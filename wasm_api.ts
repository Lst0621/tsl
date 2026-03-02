import wasmSample from "./wasm/wasm_out_v1/wasm_sample.js";
import type {WasmSampleModule} from "./wasm/wasm_out_v1/wasm_sample.js";

let moduleInstance: WasmSampleModule | null = null;

export const modulePromise: Promise<WasmSampleModule> = wasmSample().then((m) => {
    moduleInstance = m;
    return m;
});

/** Call this early (e.g. app startup) to ensure sync APIs are usable later. */
function initWasm(): Promise<WasmSampleModule> {
    return modulePromise;
}

await initWasm();

/**
 * Helper: Verify module is initialized and return HEAP32
 */
function getHeap32(): Int32Array {
    if (!moduleInstance) {
        throw new Error(
            "WASM module not initialized. Call and await initWasm() before using WASM functions."
        );
    }
    const m = moduleInstance as any;
    const HEAP32 = m.HEAP32;
    if (!HEAP32) {
        throw new Error("Cannot access WASM module HEAP32");
    }
    return HEAP32;
}

/**
 * Helper: Convert input to Int32Array
 */
function toInt32Array(input: readonly number[] | Int32Array): Int32Array {
    return input instanceof Int32Array ? input : new Int32Array(input);
}

/**
 * Helper: Copy array data into HEAP32 at given offset
 */
function copyToHeap(HEAP32: Int32Array, data: Int32Array, offsetInInts: number): void {
    for (let i = 0; i < data.length; i++) {
        HEAP32[offsetInInts + i] = data[i];
    }
}

/**
 * Helper: Calculate safe memory offsets and verify bounds
 */
function allocateMemory(HEAP32: Int32Array, arr32: Int32Array, seq32: Int32Array, extraSize: number = 0): {
    arrOffsetInInts: number;
    seqOffsetInInts: number;
    outputOffsetInInts: number;
} {
    const arrOffsetInInts = 1024;
    const seqOffsetInInts = arrOffsetInInts + arr32.length;
    const outputOffsetInInts = seqOffsetInInts + seq32.length;

    if (HEAP32.length < outputOffsetInInts + extraSize) {
        throw new Error("WASM memory exhausted");
    }

    return { arrOffsetInInts, seqOffsetInInts, outputOffsetInInts };
}

/**
 * High-level wrapper: accepts plain arrays or Int32Array.
 * Allocates space in WASM memory, copies data, calls the low-level function.
 */
export function wasmNumberOfSequences(arr: readonly number[] | Int32Array, seq: readonly number[] | Int32Array): number {
    const HEAP32 = getHeap32();
    const arr32 = toInt32Array(arr);
    const seq32 = toInt32Array(seq);

    if (arr32.length === 0 || seq32.length === 0) {
        return 0;
    }

    const { arrOffsetInInts, seqOffsetInInts } = allocateMemory(HEAP32, arr32, seq32);

    // Copy data directly into HEAP32
    copyToHeap(HEAP32, arr32, arrOffsetInInts);
    copyToHeap(HEAP32, seq32, seqOffsetInInts);

    // Convert offsets from int32 indices to byte offsets
    const arrPtr = arrOffsetInInts * 4;
    const seqPtr = seqOffsetInInts * 4;

    // Call the WASM function with byte pointers
    return moduleInstance!._wasm_number_of_sequences(arrPtr, arr32.length, seqPtr, seq32.length);
}

/**
 * Helper: Build a multi-dimensional array from flat output.
 * Dimensions correspond to (seq[i] + 1) for each sequence value.
 * Data is in lexicographic order as produced by C++.
 */
function buildMultiDimensionalArray(flatData: Int32Array, dimensions: Int32Array): any {
    if (dimensions.length === 0) {
        return flatData[0];
    }

    if (dimensions.length === 1) {
        return Array.from(flatData);
    }

    // Recursively build nested arrays
    const currentDim = dimensions[0];
    const remainingDims = dimensions.slice(1);
    const subArraySize = remainingDims.reduce((a, b) => a * (b + 1), 1);

    const result = [];
    for (let i = 0; i <= currentDim; i++) {
        const subData = flatData.slice(i * subArraySize, (i + 1) * subArraySize);
        result.push(buildMultiDimensionalArray(subData, remainingDims));
    }

    return result;
}

/**
 * High-level wrapper for getting all number of sequences results.
 * Returns a multi-dimensional array with dimensions = (seq[i] + 1) for each i.
 * The dimensions match the length of the seq parameter.
 * Data is filled in lexicographic order (rightmost dimension changes fastest).
 * Allocates space in WASM memory, copies data, calls the low-level function.
 */
export function wasmNumberOfSequencesAll(arr: readonly number[] | Int32Array, seq: readonly number[] | Int32Array): any {
    const HEAP32 = getHeap32();
    const arr32 = toInt32Array(arr);
    const seq32 = toInt32Array(seq);

    if (arr32.length === 0 || seq32.length === 0) {
        return [];
    }

    // Calculate output array size: product of (seq[i] + 1)
    let totalSize = 1;
    for (let i = 0; i < seq32.length; i++) {
        totalSize *= (seq32[i] + 1);
    }

    const { arrOffsetInInts, seqOffsetInInts } = allocateMemory(HEAP32, arr32, seq32, totalSize);

    // Copy input data into HEAP32
    copyToHeap(HEAP32, arr32, arrOffsetInInts);
    copyToHeap(HEAP32, seq32, seqOffsetInInts);

    // Convert offsets from int32 indices to byte offsets
    const arrPtr = arrOffsetInInts * 4;
    const seqPtr = seqOffsetInInts * 4;

    // Call the WASM function, which returns a pointer to the output array
    const outputPtr = moduleInstance!._wasm_number_of_sequences_all(arrPtr, arr32.length, seqPtr, seq32.length);

    // Convert byte pointer back to int32 index
    const outputOffsetFromWasm = outputPtr / 4;

    // Copy flat data from WASM memory
    const flatData = new Int32Array(totalSize);
    for (let i = 0; i < totalSize; i++) {
        flatData[i] = HEAP32[outputOffsetFromWasm + i];
    }

    // Build multi-dimensional array with dimensions matching seq values
    const dimensions = new Int32Array(seq32);
    return buildMultiDimensionalArray(flatData, dimensions);
}