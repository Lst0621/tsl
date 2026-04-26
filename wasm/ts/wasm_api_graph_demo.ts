import wasmGraph from "../wasm_out_v1/wasm_graph";
import type { WasmGraphModule } from "../wasm_out_v1/wasm_graph";
import { loadEmscriptenModule } from "./wasm_loader";

let moduleInstance: WasmGraphModule | null = null;

export const modulePromise: Promise<WasmGraphModule> = (async () => {
    const mod = await loadEmscriptenModule<WasmGraphModule>(
        wasmGraph as any,
        "../wasm_out_v1/wasm_graph.js",
        "../wasm_out_v1/wasm_graph.wasm",
    );
    moduleInstance = mod;
    return mod;
})();

await modulePromise;

function getHeap32(): Int32Array {
    if (!moduleInstance) {
        throw new Error("WASM module not initialized.");
    }
    const m = moduleInstance as any;
    if (!m.HEAP32) {
        throw new Error("Cannot access WASM module HEAP32");
    }
    return m.HEAP32 as Int32Array;
}

function toInt32Array(input: readonly number[] | Int32Array): Int32Array {
    return input instanceof Int32Array ? input : new Int32Array(input);
}

function copyToHeap(HEAP32: Int32Array, data: Int32Array, offsetInInts: number): void {
    for (let i = 0; i < data.length; i++) {
        HEAP32[offsetInInts + i] = data[i];
    }
}

function requireModuleFunctions(...names: string[]): void {
    if (!moduleInstance) {
        throw new Error("WASM module not initialized.");
    }
    const m = moduleInstance as any;
    for (const name of names) {
        if (typeof m[name] !== "function") {
            throw new Error(`WASM function not available: ${name}`);
        }
    }
}

export function wasmGraphEdgeCount(adj01: readonly number[] | Int32Array, n: number, directed: boolean): number {
    requireModuleFunctions("_wasm_graph_edge_count", "_malloc", "_free");
    const m = moduleInstance as any;
    const HEAP32 = getHeap32();
    const a32 = toInt32Array(adj01);
    const needed = n * n;
    if (a32.length !== needed) {
        throw new Error(`wasmGraphEdgeCount: expected ${needed} entries for ${n}×${n} adjacency, got ${a32.length}`);
    }
    const bytes = needed * 4;
    const ptr = m._malloc(bytes);
    if (ptr === 0) {
        throw new Error("wasmGraphEdgeCount: malloc failed");
    }
    try {
        copyToHeap(HEAP32, a32, ptr / 4);
        return m._wasm_graph_edge_count(n, directed ? 1 : 0, ptr);
    } finally {
        m._free(ptr);
    }
}

export function wasmGraphRandomizeUndirectedAdj01(
    n: number,
    seed: number,
): { adj01: number[]; edgeCount: number; threshold: number } {
    requireModuleFunctions("_wasm_graph_randomize_undirected_adj01", "_malloc", "_free");
    const m = moduleInstance as any;
    const HEAP32 = getHeap32();
    const needed = n * n;
    const bytesAdj = needed * 4;
    const adjPtr = m._malloc(bytesAdj);
    const edgeCountPtr = m._malloc(4);
    const thresholdMilliPtr = m._malloc(4);
    if (adjPtr === 0 || edgeCountPtr === 0 || thresholdMilliPtr === 0) {
        if (adjPtr) {
            m._free(adjPtr);
        }
        if (edgeCountPtr) {
            m._free(edgeCountPtr);
        }
        if (thresholdMilliPtr) {
            m._free(thresholdMilliPtr);
        }
        throw new Error("wasmGraphRandomizeUndirectedAdj01: malloc failed");
    }
    try {
        m._wasm_graph_randomize_undirected_adj01(
            n,
            (seed >>> 0),
            adjPtr,
            edgeCountPtr,
            thresholdMilliPtr,
        );
        const out: number[] = [];
        const base = adjPtr / 4;
        for (let i = 0; i < needed; i++) {
            out.push(HEAP32[base + i]);
        }
        const edgeCount = HEAP32[edgeCountPtr / 4];
        const thresholdMilli = HEAP32[thresholdMilliPtr / 4];
        return { adj01: out, edgeCount, threshold: thresholdMilli / 1000.0 };
    } finally {
        m._free(adjPtr);
        m._free(edgeCountPtr);
        m._free(thresholdMilliPtr);
    }
}

export function wasmGraphAllPairsDistances(adj01: readonly number[] | Int32Array, n: number, directed: boolean): number[] {
    requireModuleFunctions("_wasm_graph_all_pairs_bfs_distances", "_malloc", "_free");
    const m = moduleInstance as any;
    const HEAP32 = getHeap32();
    const a32 = toInt32Array(adj01);
    const needed = n * n;
    if (a32.length !== needed) {
        throw new Error(`wasmGraphAllPairsDistances: expected ${needed} entries for ${n}×${n} adjacency, got ${a32.length}`);
    }
    const bytesAdj = needed * 4;
    const bytesOut = needed * 4;
    const adjPtr = m._malloc(bytesAdj);
    const outPtr = m._malloc(bytesOut);
    if (adjPtr === 0 || outPtr === 0) {
        if (adjPtr) {
            m._free(adjPtr);
        }
        if (outPtr) {
            m._free(outPtr);
        }
        throw new Error("wasmGraphAllPairsDistances: malloc failed");
    }
    try {
        copyToHeap(HEAP32, a32, adjPtr / 4);
        m._wasm_graph_all_pairs_bfs_distances(n, directed ? 1 : 0, adjPtr, outPtr);
        const out: number[] = [];
        const base = outPtr / 4;
        for (let i = 0; i < needed; i++) {
            out.push(HEAP32[base + i]);
        }
        return out;
    } finally {
        m._free(adjPtr);
        m._free(outPtr);
    }
}

export type WasmGraphResolvingSubsetsPageModeResult = {
    minDimension: number;
    smallestBasis: number[];
    totalCount: number;
    pageCount: number;
    subsets: number[][];
    truncated: boolean;
    minSizeSubsets: number[][];
    minSizeTruncated: boolean;
};

export type WasmGraphResolvingSubsetsPageAllModesResult = {
    node: WasmGraphResolvingSubsetsPageModeResult;
    edge: WasmGraphResolvingSubsetsPageModeResult;
    mixed: WasmGraphResolvingSubsetsPageModeResult;
};

export interface WasmGraphResolvingSubsetsCacheHandle {
    setGraph(
        adj01: readonly number[] | Int32Array,
        distFlat: readonly number[] | Int32Array,
        n: number,
    ): void;
    getPage(
        pageSize: number,
        pageIndexByMode: readonly [number, number, number],
    ): WasmGraphResolvingSubsetsPageAllModesResult;
    free(): void;
}

export function wasmGraphResolvingSubsetsCacheCreate(): WasmGraphResolvingSubsetsCacheHandle {
    requireModuleFunctions(
        "_wasm_graph_resolving_subsets_cache_create",
        "_wasm_graph_resolving_subsets_cache_destroy",
        "_wasm_graph_resolving_subsets_cache_set_graph",
        "_wasm_graph_resolving_subsets_cache_get_page",
        "_malloc",
        "_free",
    );
    const m = moduleInstance as any;
    const handle = m._wasm_graph_resolving_subsets_cache_create();
    if (handle === 0) {
        throw new Error("wasmGraphResolvingSubsetsCacheCreate: create failed");
    }

    let released = false;
    let graphN = 0;

    return {
        setGraph(
            adj01: readonly number[] | Int32Array,
            distFlat: readonly number[] | Int32Array,
            n: number,
        ): void {
            if (released) {
                throw new Error("wasmGraphResolvingSubsetsCacheHandle: cache already freed");
            }
            const HEAP32 = getHeap32();
            const a32 = toInt32Array(adj01);
            const d32 = toInt32Array(distFlat);
            const needed = n * n;
            if (a32.length !== needed) {
                throw new Error(`wasmGraphResolvingSubsetsCacheHandle.setGraph: expected ${needed} adjacency entries, got ${a32.length}`);
            }
            if (d32.length !== needed) {
                throw new Error(`wasmGraphResolvingSubsetsCacheHandle.setGraph: expected ${needed} distance entries, got ${d32.length}`);
            }

            const bytes = needed * 4;
            const adjPtr = m._malloc(bytes);
            const distPtr = m._malloc(bytes);
            if (adjPtr === 0 || distPtr === 0) {
                if (adjPtr) {
                    m._free(adjPtr);
                }
                if (distPtr) {
                    m._free(distPtr);
                }
                throw new Error("wasmGraphResolvingSubsetsCacheHandle.setGraph: malloc failed");
            }

            try {
                copyToHeap(HEAP32, a32, adjPtr / 4);
                copyToHeap(HEAP32, d32, distPtr / 4);
                const ok = m._wasm_graph_resolving_subsets_cache_set_graph(
                    handle,
                    n,
                    adjPtr,
                    distPtr,
                );
                if (ok === 0) {
                    throw new Error("wasmGraphResolvingSubsetsCacheHandle.setGraph: wasm call failed");
                }
                graphN = n;
            } finally {
                m._free(adjPtr);
                m._free(distPtr);
            }
        },

        getPage(
            pageSize: number,
            pageIndexByMode: readonly [number, number, number],
        ): WasmGraphResolvingSubsetsPageAllModesResult {
            if (released) {
                throw new Error("wasmGraphResolvingSubsetsCacheHandle: cache already freed");
            }
            if (graphN <= 0) {
                throw new Error("wasmGraphResolvingSubsetsCacheHandle.getPage: setGraph must be called first");
            }
            if (pageSize < 0) {
                pageSize = 0;
            }

            const HEAP32 = getHeap32();
            const modeCount = 3;
            const bytesPageIndex3 = modeCount * 4;
            const bytesMinDim3 = modeCount * 4;
            const bytesSmallest3 = modeCount * graphN * 4;
            const bytesTotalCount3 = modeCount * 4;
            const bytesPageCount3 = modeCount * 4;
            const bytesPageListCount3 = modeCount * 4;
            const bytesPageUsed3 = modeCount * 4;
            const bytesPageTrunc3 = modeCount * 4;
            const pageListFlatMaxIntsPerMode = Math.max(1, pageSize * (graphN + 1));
            const bytesPageFlat3 = modeCount * pageListFlatMaxIntsPerMode * 4;
            const minListFlatMaxIntsPerMode = Math.max(1, graphN * graphN * (graphN + 1));
            const bytesMinListCount3 = modeCount * 4;
            const bytesMinUsed3 = modeCount * 4;
            const bytesMinTrunc3 = modeCount * 4;
            const bytesMinFlat3 = modeCount * minListFlatMaxIntsPerMode * 4;

            const pageIndex3Ptr = m._malloc(bytesPageIndex3);
            const minDim3Ptr = m._malloc(bytesMinDim3);
            const smallest3Ptr = m._malloc(bytesSmallest3);
            const totalCount3Ptr = m._malloc(bytesTotalCount3);
            const pageCount3Ptr = m._malloc(bytesPageCount3);
            const pageListCount3Ptr = m._malloc(bytesPageListCount3);
            const pageUsed3Ptr = m._malloc(bytesPageUsed3);
            const pageTrunc3Ptr = m._malloc(bytesPageTrunc3);
            const pageFlat3Ptr = m._malloc(bytesPageFlat3);
            const minListCount3Ptr = m._malloc(bytesMinListCount3);
            const minUsed3Ptr = m._malloc(bytesMinUsed3);
            const minTrunc3Ptr = m._malloc(bytesMinTrunc3);
            const minFlat3Ptr = m._malloc(bytesMinFlat3);

            if (pageIndex3Ptr === 0 || minDim3Ptr === 0 || smallest3Ptr === 0 ||
                totalCount3Ptr === 0 || pageCount3Ptr === 0 ||
                pageListCount3Ptr === 0 || pageUsed3Ptr === 0 ||
                pageTrunc3Ptr === 0 || pageFlat3Ptr === 0 ||
                minListCount3Ptr === 0 || minUsed3Ptr === 0 ||
                minTrunc3Ptr === 0 || minFlat3Ptr === 0) {
                if (pageIndex3Ptr) {
                    m._free(pageIndex3Ptr);
                }
                if (minDim3Ptr) {
                    m._free(minDim3Ptr);
                }
                if (smallest3Ptr) {
                    m._free(smallest3Ptr);
                }
                if (totalCount3Ptr) {
                    m._free(totalCount3Ptr);
                }
                if (pageCount3Ptr) {
                    m._free(pageCount3Ptr);
                }
                if (pageListCount3Ptr) {
                    m._free(pageListCount3Ptr);
                }
                if (pageUsed3Ptr) {
                    m._free(pageUsed3Ptr);
                }
                if (pageTrunc3Ptr) {
                    m._free(pageTrunc3Ptr);
                }
                if (pageFlat3Ptr) {
                    m._free(pageFlat3Ptr);
                }
                if (minListCount3Ptr) {
                    m._free(minListCount3Ptr);
                }
                if (minUsed3Ptr) {
                    m._free(minUsed3Ptr);
                }
                if (minTrunc3Ptr) {
                    m._free(minTrunc3Ptr);
                }
                if (minFlat3Ptr) {
                    m._free(minFlat3Ptr);
                }
                throw new Error("wasmGraphResolvingSubsetsCacheHandle.getPage: malloc failed");
            }

            try {
                const pageIndexBase = pageIndex3Ptr / 4;
                HEAP32[pageIndexBase] = Math.max(0, pageIndexByMode[0] | 0);
                HEAP32[pageIndexBase + 1] = Math.max(0, pageIndexByMode[1] | 0);
                HEAP32[pageIndexBase + 2] = Math.max(0, pageIndexByMode[2] | 0);

                const ok = m._wasm_graph_resolving_subsets_cache_get_page(
                    handle,
                    pageSize,
                    pageIndex3Ptr,
                    minDim3Ptr,
                    smallest3Ptr,
                    graphN,
                    totalCount3Ptr,
                    pageCount3Ptr,
                    pageListCount3Ptr,
                    pageUsed3Ptr,
                    pageFlat3Ptr,
                    pageListFlatMaxIntsPerMode,
                    pageTrunc3Ptr,
                    minListCount3Ptr,
                    minUsed3Ptr,
                    minFlat3Ptr,
                    minListFlatMaxIntsPerMode,
                    minTrunc3Ptr,
                );
                if (ok === 0) {
                    throw new Error("wasmGraphResolvingSubsetsCacheHandle.getPage: wasm call failed");
                }

                const parseMode = (modeIdx: number): WasmGraphResolvingSubsetsPageModeResult => {
                    const minDim = HEAP32[minDim3Ptr / 4 + modeIdx];
                    const smallestBasis: number[] = [];
                    const smallestBase = smallest3Ptr / 4 + modeIdx * graphN;
                    for (let i = 0; i < minDim; i++) {
                        smallestBasis.push(HEAP32[smallestBase + i]);
                    }
                    const totalCount = HEAP32[totalCount3Ptr / 4 + modeIdx];
                    const pageCount = HEAP32[pageCount3Ptr / 4 + modeIdx];
                    const usedInts = HEAP32[pageUsed3Ptr / 4 + modeIdx];
                    const truncated = HEAP32[pageTrunc3Ptr / 4 + modeIdx] !== 0;

                    const subsets: number[][] = [];
                    const flatBase = pageFlat3Ptr / 4 + modeIdx * pageListFlatMaxIntsPerMode;
                    let idx = 0;
                    while (idx < usedInts) {
                        const k = HEAP32[flatBase + idx];
                        idx++;
                        const subset: number[] = [];
                        for (let i = 0; i < k; i++) {
                            subset.push(HEAP32[flatBase + idx]);
                            idx++;
                        }
                        subsets.push(subset);
                    }

                    const minUsedInts = HEAP32[minUsed3Ptr / 4 + modeIdx];
                    const minTruncated = HEAP32[minTrunc3Ptr / 4 + modeIdx] !== 0;
                    const minSizeSubsets: number[][] = [];
                    const minFlatBase = minFlat3Ptr / 4 + modeIdx * minListFlatMaxIntsPerMode;
                    let midx = 0;
                    while (midx < minUsedInts) {
                        const k = HEAP32[minFlatBase + midx];
                        midx++;
                        const subset: number[] = [];
                        for (let i = 0; i < k; i++) {
                            subset.push(HEAP32[minFlatBase + midx]);
                            midx++;
                        }
                        minSizeSubsets.push(subset);
                    }

                    return {
                        minDimension: minDim,
                        smallestBasis,
                        totalCount,
                        pageCount,
                        subsets,
                        truncated,
                        minSizeSubsets,
                        minSizeTruncated: minTruncated,
                    };
                };

                return {
                    node: parseMode(0),
                    edge: parseMode(1),
                    mixed: parseMode(2),
                };
            } finally {
                m._free(pageIndex3Ptr);
                m._free(minDim3Ptr);
                m._free(smallest3Ptr);
                m._free(totalCount3Ptr);
                m._free(pageCount3Ptr);
                m._free(pageListCount3Ptr);
                m._free(pageUsed3Ptr);
                m._free(pageTrunc3Ptr);
                m._free(pageFlat3Ptr);
                m._free(minListCount3Ptr);
                m._free(minUsed3Ptr);
                m._free(minTrunc3Ptr);
                m._free(minFlat3Ptr);
            }
        },

        free(): void {
            if (released) {
                return;
            }
            released = true;
            m._wasm_graph_resolving_subsets_cache_destroy(handle);
        },
    };
}

