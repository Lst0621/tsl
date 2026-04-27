import wasmGol from "../wasm_out_ci/wasm_gol";
import { loadEmscriptenModule } from "./wasm_loader";
import type { WasmGolModule } from "./wasm_types";

let moduleInstance: WasmGolModule | null = null;

export const modulePromise: Promise<WasmGolModule> = (async () => {
    const mod = await loadEmscriptenModule<WasmGolModule>(
        wasmGol as any,
        "../wasm_out_ci/wasm_gol.js",
        "../wasm_out_ci/wasm_gol.wasm",
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

// --- Game of Life (C++ WASM) ---
const GOL_DEFAULT_SIZE = 40;
let golHandle: number | null = null;
let golSize: number = GOL_DEFAULT_SIZE;
let golLiveCellsPtr: number | null = null;
let golLiveCellsCap: number = 0;

export type GolTopologyMode = 0 | 1 | 2; // Finite2D, Torus2D, Cylinder2D

export function golCreate(size: number): void {
    if (!moduleInstance) {
        throw new Error("WASM module not initialized.");
    }
    if (golHandle !== null) {
        moduleInstance._gol_destroy(golHandle);
    }
    if (golLiveCellsPtr !== null) {
        moduleInstance._free(golLiveCellsPtr);
        golLiveCellsPtr = null;
        golLiveCellsCap = 0;
    }
    golHandle = moduleInstance._gol_create(size);
    golSize = size;
}

export function golDestroy(): void {
    if (moduleInstance && golHandle !== null) {
        moduleInstance._gol_destroy(golHandle);
        golHandle = null;
    }
    if (moduleInstance && golLiveCellsPtr !== null) {
        moduleInstance._free(golLiveCellsPtr);
        golLiveCellsPtr = null;
        golLiveCellsCap = 0;
    }
}

export function golInit(): void {
    if (!moduleInstance || golHandle === null) {
        throw new Error("GoL not created. Call golCreate first.");
    }
    moduleInstance._gol_init(golHandle);
}

export function golRandomInitWithSeed(liveProb: number, seed: number): void {
    if (!moduleInstance || golHandle === null) {
        throw new Error("GoL not created. Call golCreate first.");
    }
    moduleInstance._gol_random_init_seed(golHandle, liveProb, seed);
}

export function golGetSeed(): number {
    if (!moduleInstance || golHandle === null) {
        return 0;
    }
    return moduleInstance._gol_get_seed(golHandle);
}

export function golEvolve(): void {
    if (!moduleInstance || golHandle === null) {
        throw new Error("GoL not created. Call golCreate first.");
    }
    moduleInstance._gol_evolve(golHandle);
}

export function golSetTopology(mode: GolTopologyMode): void {
    if (!moduleInstance || golHandle === null) {
        throw new Error("GoL not created. Call golCreate first.");
    }
    moduleInstance._gol_set_topology(golHandle, mode);
}

export function golSetWormholeSeed(seed: number): void {
    if (!moduleInstance || golHandle === null) {
        throw new Error("GoL not created. Call golCreate first.");
    }
    moduleInstance._gol_set_wormhole_seed(golHandle, seed >>> 0);
}

export function golSetWormholeCount(count: number): void {
    if (!moduleInstance || golHandle === null) {
        throw new Error("GoL not created. Call golCreate first.");
    }
    moduleInstance._gol_set_wormhole_count(golHandle, count | 0);
}

export function golGetWormholeEdges(): number {
    if (!moduleInstance || golHandle === null) {
        throw new Error("GoL not created. Call golCreate first.");
    }
    return moduleInstance._gol_get_wormhole_edges(golHandle);
}

export function golSetCutSeed(seed: number): void {
    if (!moduleInstance || golHandle === null) {
        throw new Error("GoL not created. Call golCreate first.");
    }
    moduleInstance._gol_set_cut_seed(golHandle, seed >>> 0);
}

export function golSetCutCount(count: number): void {
    if (!moduleInstance || golHandle === null) {
        throw new Error("GoL not created. Call golCreate first.");
    }
    moduleInstance._gol_set_cut_count(golHandle, count | 0);
}

export function golGetCutEdges(): number {
    if (!moduleInstance || golHandle === null) {
        throw new Error("GoL not created. Call golCreate first.");
    }
    return moduleInstance._gol_get_cut_edges(golHandle);
}

export function golGetLiveCells(): { x: number; y: number }[] {
    if (!moduleInstance || golHandle === null) {
        throw new Error("GoL not created. Call golCreate first.");
    }
    const HEAP32 = getHeap32();
    const maxCount = golSize * golSize;
    const bytes = 2 * maxCount * 4;
    if (golLiveCellsPtr === null || golLiveCellsCap < bytes) {
        if (golLiveCellsPtr !== null) {
            moduleInstance._free(golLiveCellsPtr);
        }
        const ptr = moduleInstance._malloc(bytes);
        if (ptr === 0) {
            throw new Error("WASM malloc failed for GoL live cells.");
        }
        golLiveCellsPtr = ptr;
        golLiveCellsCap = bytes;
    }
    const count = moduleInstance._gol_get_live_cells(golHandle, golLiveCellsPtr, maxCount);
    const out: { x: number; y: number }[] = [];
    const base = golLiveCellsPtr / 4;
    for (let i = 0; i < count; i++) {
        out.push({ x: HEAP32[base + 2 * i], y: HEAP32[base + 2 * i + 1] });
    }
    return out;
}

export function golGetSize(): number {
    return golSize;
}

export function golIsCreated(): boolean {
    return golHandle !== null;
}

