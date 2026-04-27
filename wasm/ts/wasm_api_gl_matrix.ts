import wasmMatrix from "../wasm_out_ci/wasm_matrix";
import { loadEmscriptenModule } from "./wasm_loader";
import type { WasmMatrixModule } from "./wasm_types";

let moduleInstance: WasmMatrixModule | null = null;

export const modulePromise: Promise<WasmMatrixModule> = (async () => {
    const mod = await loadEmscriptenModule<WasmMatrixModule>(
        wasmMatrix as any,
        "../wasm_out_ci/wasm_matrix.js",
        "../wasm_out_ci/wasm_matrix.wasm",
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

export function wasm_get_gl_n_zm_size(n: number, m: number): number {
    return moduleInstance!._wasm_get_gl_n_zm_size(n, m);
}

export interface WasmMatrixSet {
    ptr: number;
    count: number;
    n: number;
    modulus: number;
    toFlatArray(): number[];
    free(): void;
}

export function wasm_get_gl_n_zm(n: number, m: number): WasmMatrixSet {
    const HEAP32 = getHeap32();

    const countSlot = 256;
    const countPtr = countSlot * 4;

    const bufPtr = moduleInstance!._wasm_get_gl_n_zm(n, m, countPtr);
    const count = HEAP32[countSlot];

    return {
        ptr: bufPtr,
        count,
        n,
        modulus: m,
        toFlatArray(): number[] {
            const H = getHeap32();
            const startIdx = bufPtr / 4;
            const total = count * n * n;
            const out: number[] = [];
            for (let i = 0; i < total; i++) {
                out.push(H[startIdx + i]);
            }
            return out;
        },
        free(): void {
            moduleInstance!._free(bufPtr);
        },
    };
}

export function wasm_is_matrix_group(
    dataOrPtrOrHandle: number[] | WasmMatrixSet | number,
    count?: number,
    n?: number,
    modulus?: number,
): boolean {
    const m = moduleInstance as any;
    const HEAP32 = getHeap32();

    // Zero-copy handle path: wasm_is_matrix_group(glHandle)
    if (typeof dataOrPtrOrHandle === "object" && "ptr" in dataOrPtrOrHandle) {
        const h = dataOrPtrOrHandle as WasmMatrixSet;
        return m._wasm_is_matrix_group(h.ptr, h.count, h.n, h.modulus) !== 0;
    }

    // Pointer path: wasm_is_matrix_group(ptr, count, n, modulus)
    if (typeof dataOrPtrOrHandle === "number") {
        if (count === undefined || n === undefined || modulus === undefined) {
            throw new Error("wasm_is_matrix_group(ptr,...): missing args");
        }
        return m._wasm_is_matrix_group(dataOrPtrOrHandle, count, n, modulus) !== 0;
    }

    // JS array path
    if (count === undefined || n === undefined || modulus === undefined) {
        throw new Error("wasm_is_matrix_group(array,...): missing args");
    }
    const a32 = toInt32Array(dataOrPtrOrHandle);
    const bytes = a32.length * 4;
    const ptr = m._malloc(bytes);
    if (ptr === 0) {
        throw new Error("wasm_is_matrix_group: malloc failed");
    }
    try {
        copyToHeap(HEAP32, a32, ptr / 4);
        return m._wasm_is_matrix_group(ptr, count, n, modulus) !== 0;
    } finally {
        m._free(ptr);
    }
}

export function wasm_matrix_det(data: number[], n: number): number {
    if (data.length !== n * n) {
        throw new Error("wasm_matrix_det: dimension mismatch");
    }
    const m = moduleInstance as any;
    const HEAP32 = getHeap32();
    const a32 = toInt32Array(data);
    const bytes = a32.length * 4;
    const ptr = m._malloc(bytes);
    if (ptr === 0) {
        throw new Error("wasm_matrix_det: malloc failed");
    }
    try {
        copyToHeap(HEAP32, a32, ptr / 4);
        return m._wasm_matrix_det(ptr, n);
    } finally {
        m._free(ptr);
    }
}

export function wasm_matrix_inverse_mod(data: number[], n: number, mod: number): number[] {
    if (data.length !== n * n) {
        throw new Error("wasm_matrix_inverse_mod: dimension mismatch");
    }
    const m = moduleInstance as any;
    const HEAP32 = getHeap32();
    const a32 = toInt32Array(data);
    const size = n * n;
    const bytesIn = size * 4;
    const bytesOut = size * 4;
    const inPtr = m._malloc(bytesIn);
    const outPtr = m._malloc(bytesOut);
    if (inPtr === 0 || outPtr === 0) {
        if (inPtr) {
            m._free(inPtr);
        }
        if (outPtr) {
            m._free(outPtr);
        }
        throw new Error("wasm_matrix_inverse_mod: malloc failed");
    }
    try {
        copyToHeap(HEAP32, a32, inPtr / 4);
        m._wasm_matrix_inverse_mod(inPtr, n, mod, outPtr);
        const out: number[] = [];
        const base = outPtr / 4;
        for (let i = 0; i < size; i++) {
            out.push(HEAP32[base + i]);
        }
        return out;
    } finally {
        m._free(inPtr);
        m._free(outPtr);
    }
}

