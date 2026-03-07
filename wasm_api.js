import wasmSample from "./wasm/wasm_out_v1/wasm_sample.js";
let moduleInstance = null;
export const modulePromise = wasmSample().then((m) => {
    moduleInstance = m;
    return m;
});
/** Call this early (e.g. app startup) to ensure sync APIs are usable later. */
function initWasm() {
    return modulePromise;
}
await initWasm();
/**
 * Helper: Verify module is initialized and return HEAP32
 */
function getHeap32() {
    if (!moduleInstance) {
        throw new Error("WASM module not initialized. Call and await initWasm() before using WASM functions.");
    }
    const m = moduleInstance;
    const HEAP32 = m.HEAP32;
    if (!HEAP32) {
        throw new Error("Cannot access WASM module HEAP32");
    }
    return HEAP32;
}
/**
 * Helper: Convert input to Int32Array
 */
function toInt32Array(input) {
    return input instanceof Int32Array ? input : new Int32Array(input);
}
/**
 * Helper: Copy array data into HEAP32 at given offset
 */
function copyToHeap(HEAP32, data, offsetInInts) {
    for (let i = 0; i < data.length; i++) {
        HEAP32[offsetInInts + i] = data[i];
    }
}
/**
 * Helper: Calculate safe memory offsets and verify bounds
 */
function allocateMemory(HEAP32, arr32, seq32, extraSize = 0) {
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
export function wasm_number_of_sequences(arr, seq) {
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
    return moduleInstance._wasm_number_of_sequences(arrPtr, arr32.length, seqPtr, seq32.length);
}
/**
 * Helper: Build a multi-dimensional array from flat output.
 * Dimensions correspond to (seq[i] + 1) for each sequence value.
 * Data is in lexicographic order as produced by C++.
 */
function build_multi_dimensional_array(flatData, dimensions) {
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
        result.push(build_multi_dimensional_array(subData, remainingDims));
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
export function wasm_number_of_sequences_all(arr, seq) {
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
    const outputPtr = moduleInstance._wasm_number_of_sequences_all(arrPtr, arr32.length, seqPtr, seq32.length);
    // Convert byte pointer back to int32 index
    const outputOffsetFromWasm = outputPtr / 4;
    // Copy flat data from WASM memory
    const flatData = new Int32Array(totalSize);
    for (let i = 0; i < totalSize; i++) {
        flatData[i] = HEAP32[outputOffsetFromWasm + i];
    }
    // Build multi-dimensional array with dimensions matching seq values
    const dimensions = new Int32Array(seq32);
    return build_multi_dimensional_array(flatData, dimensions);
}
/**
 * Get the size of the general linear group GL(n, m).
 * Returns the count of all invertible n×n matrices with elements in Z_m.
 *
 * @param n Size of the matrix (n×n)
 * @param m Modulus for elements (elements will be 0 to m-1)
 * @return Number of invertible matrices in GL(n, m)
 */
export function wasm_get_gl_n_zm_size(n, m) {
    if (!moduleInstance) {
        throw new Error("WASM module not initialized. Call and await initWasm() before using WASM functions.");
    }
    return moduleInstance._wasm_get_gl_n_zm_size(n, m);
}
/**
 * Calculate the determinant of a square matrix.
 *
 * @param data Array of integers representing the matrix in row-major order
 * @param n Size of the square matrix (n×n). Total elements should be n²
 * @return Determinant of the matrix
 */
export function wasm_matrix_det(data, n) {
    const HEAP32 = getHeap32();
    const data32 = toInt32Array(data);
    // Validate that we have exactly n² elements
    const totalSize = n * n;
    if (data32.length !== totalSize) {
        throw new Error(`Expected ${totalSize} elements for a ${n}×${n} matrix, got ${data32.length}`);
    }
    // Allocate memory in WASM heap
    const dataOffsetInInts = 1024;
    if (HEAP32.length < dataOffsetInInts + totalSize) {
        throw new Error("WASM memory exhausted");
    }
    // Copy data to HEAP32
    copyToHeap(HEAP32, data32, dataOffsetInInts);
    // Convert offset from int32 indices to byte offset
    const dataPtr = dataOffsetInInts * 4;
    // Call the WASM function
    return moduleInstance._wasm_matrix_det(dataPtr, n);
}
// --- Game of Life (C++ WASM) ---
const GOL_DEFAULT_SIZE = 40;
let golHandle = null;
let golSize = GOL_DEFAULT_SIZE;
export function golCreate(size) {
    if (!moduleInstance)
        throw new Error("WASM module not initialized.");
    if (golHandle !== null) {
        moduleInstance._gol_destroy(golHandle);
    }
    golHandle = moduleInstance._gol_create(size);
    golSize = size;
}
export function golDestroy() {
    if (moduleInstance && golHandle !== null) {
        moduleInstance._gol_destroy(golHandle);
        golHandle = null;
    }
}
export function golInit() {
    if (!moduleInstance || golHandle === null)
        throw new Error("GoL not created. Call golCreate first.");
    moduleInstance._gol_init(golHandle);
}
export function golRandomInit(liveProb) {
    if (!moduleInstance || golHandle === null)
        throw new Error("GoL not created. Call golCreate first.");
    moduleInstance._gol_random_init(golHandle, liveProb);
}
export function golRandomInitWithSeed(liveProb, seed) {
    if (!moduleInstance || golHandle === null)
        throw new Error("GoL not created. Call golCreate first.");
    moduleInstance._gol_random_init_seed(golHandle, liveProb, seed);
}
export function golGetSeed() {
    if (!moduleInstance || golHandle === null) return 0;
    return moduleInstance._gol_get_seed(golHandle);
}
export function golEvolve() {
    if (!moduleInstance || golHandle === null)
        throw new Error("GoL not created. Call golCreate first.");
    moduleInstance._gol_evolve(golHandle);
}
export function golSetTopology(mode) {
    if (!moduleInstance || golHandle === null)
        throw new Error("GoL not created. Call golCreate first.");
    moduleInstance._gol_set_topology(golHandle, mode);
}
export function golGetLiveCells() {
    if (!moduleInstance || golHandle === null)
        throw new Error("GoL not created. Call golCreate first.");
    const HEAP32 = getHeap32();
    const maxCount = golSize * golSize;
    const bytes = 2 * maxCount * 4;
    const ptr = moduleInstance._malloc(bytes);
    if (ptr === 0)
        throw new Error("WASM malloc failed for GoL live cells.");
    try {
        const count = moduleInstance._gol_get_live_cells(golHandle, ptr, maxCount);
        const out = [];
        const base = ptr / 4;
        for (let i = 0; i < count; i++) {
            out.push({ x: HEAP32[base + 2 * i], y: HEAP32[base + 2 * i + 1] });
        }
        return out;
    }
    finally {
        moduleInstance._free(ptr);
    }
}
export function golGetSize() {
    return golSize;
}
export function golIsCreated() {
    return golHandle !== null;
}
