import wasmSample from "./wasm/wasm_out_v1/wasm_sample.js";
let moduleInstance = null;
/** In Node, fetch() cannot load file:// URLs, so we pass the WASM binary directly. */
async function createModulePromise() {
    var _a;
    const g = typeof globalThis !== "undefined" ? globalThis : undefined;
    const proc = g && g.process;
    const inNode = typeof ((_a = proc === null || proc === void 0 ? void 0 : proc.versions) === null || _a === void 0 ? void 0 : _a.node) === "string";
    if (inNode) {
        const nodeFs = "node:fs";
        const nodeUrl = "node:url";
        const nodePath = "node:path";
        const fs = (await import(nodeFs));
        const url = (await import(nodeUrl));
        const path = (await import(nodePath));
        const __filename = url.fileURLToPath(import.meta.url);
        const __dirname = path.dirname(__filename);
        const wasmPath = path.join(__dirname, "wasm", "wasm_out_v1", "wasm_sample.wasm");
        const wasmBinary = fs.readFileSync(wasmPath);
        const mod = await wasmSample({ wasmBinary });
        moduleInstance = mod;
        return mod;
    }
    const mod = await wasmSample();
    moduleInstance = mod;
    return mod;
}
export const modulePromise = createModulePromise();
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
    if (!moduleInstance || golHandle === null)
        return 0;
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
// --- Bars game (opaque handle) ---
const BARS_GAME_BASE_INTS = 1280;
let barsGameHandle = null;
const m = () => moduleInstance;
export function barsGameCreate() {
    if (!moduleInstance)
        throw new Error("WASM module not initialized.");
    if (barsGameHandle !== null) {
        m()._bars_game_destroy(barsGameHandle);
    }
    barsGameHandle = m()._bars_game_create();
}
export function barsGameDestroy() {
    if (moduleInstance && barsGameHandle !== null) {
        m()._bars_game_destroy(barsGameHandle);
        barsGameHandle = null;
    }
}
export function barsGameSetSeed(seed) {
    if (!moduleInstance || barsGameHandle === null)
        throw new Error("Bars game not created. Call barsGameCreate first.");
    m()._bars_game_set_seed(barsGameHandle, seed);
}
export function barsGameInit() {
    if (!moduleInstance || barsGameHandle === null)
        throw new Error("Bars game not created. Call barsGameCreate first.");
    m()._bars_game_init(barsGameHandle);
}
export function barsGameGetState() {
    if (!moduleInstance || barsGameHandle === null)
        throw new Error("Bars game not created. Call barsGameCreate first.");
    const HEAP32 = getHeap32();
    const size = m()._bars_game_state_size(barsGameHandle);
    if (HEAP32.length < BARS_GAME_BASE_INTS + size)
        throw new Error("WASM memory exhausted");
    m()._bars_game_get_state(barsGameHandle, BARS_GAME_BASE_INTS * 4);
    const out = [];
    for (let i = 0; i < size; i++)
        out.push(HEAP32[BARS_GAME_BASE_INTS + i]);
    return out;
}
export function barsGameGetFutureState(choiceIndex) {
    if (!moduleInstance || barsGameHandle === null)
        throw new Error("Bars game not created. Call barsGameCreate first.");
    const HEAP32 = getHeap32();
    const size = m()._bars_game_state_size(barsGameHandle);
    if (HEAP32.length < BARS_GAME_BASE_INTS + size)
        throw new Error("WASM memory exhausted");
    m()._bars_game_get_future_state(barsGameHandle, choiceIndex, BARS_GAME_BASE_INTS * 4);
    const out = [];
    for (let i = 0; i < size; i++)
        out.push(HEAP32[BARS_GAME_BASE_INTS + i]);
    return out;
}
export function barsGameApplyChoice(index) {
    if (!moduleInstance || barsGameHandle === null)
        throw new Error("Bars game not created. Call barsGameCreate first.");
    m()._bars_game_apply_choice(barsGameHandle, index);
}
export function barsGameIsEnded() {
    if (!moduleInstance || barsGameHandle === null)
        return true;
    return m()._bars_game_is_ended(barsGameHandle) !== 0;
}
export function barsGameStateSize() {
    if (!moduleInstance || barsGameHandle === null)
        return 0;
    return m()._bars_game_state_size(barsGameHandle);
}
export function barsGameMaxVal() {
    if (!moduleInstance || barsGameHandle === null)
        return 2000;
    return m()._bars_game_max_val(barsGameHandle);
}
// --- Linear recurrence (opaque handle) ---
const LR_DEFAULT_THRESHOLD = 20;
let lrHandle = null;
export function lrCreate(coeffs, recursiveThreshold = LR_DEFAULT_THRESHOLD) {
    if (!moduleInstance)
        throw new Error("WASM module not initialized.");
    if (lrHandle !== null) {
        moduleInstance._lr_destroy(lrHandle);
    }
    const HEAP32 = getHeap32();
    const arr32 = toInt32Array(coeffs);
    const base = 2048;
    if (HEAP32.length < base + arr32.length)
        throw new Error("WASM memory exhausted");
    copyToHeap(HEAP32, arr32, base);
    lrHandle = moduleInstance._lr_create(base * 4, arr32.length, recursiveThreshold);
}
export function lrDestroy() {
    if (moduleInstance && lrHandle !== null) {
        moduleInstance._lr_destroy(lrHandle);
        lrHandle = null;
    }
}
export function lrOrder() {
    if (!moduleInstance || lrHandle === null)
        throw new Error("Linear recurrence not created. Call lrCreate first.");
    return moduleInstance._lr_order(lrHandle);
}
/** Returns a(n) as number (full int64 range via low + high*2^32; fits in number for |value| < 2^53). */
export function lrEvaluate(initialValues, n) {
    if (!moduleInstance || lrHandle === null)
        throw new Error("Linear recurrence not created. Call lrCreate first.");
    const HEAP32 = getHeap32();
    const init32 = toInt32Array(initialValues);
    const initBase = 2048;
    const resultBase = 4096;
    if (HEAP32.length < resultBase + 2)
        throw new Error("WASM memory exhausted");
    copyToHeap(HEAP32, init32, initBase);
    moduleInstance._lr_evaluate(lrHandle, initBase * 4, init32.length, n, resultBase * 4);
    const low = HEAP32[resultBase];
    const high = HEAP32[resultBase + 1];
    return low + high * 0x100000000;
}
export function lrGetCharacteristicPolynomial() {
    if (!moduleInstance || lrHandle === null)
        throw new Error("Linear recurrence not created. Call lrCreate first.");
    const order = moduleInstance._lr_order(lrHandle);
    const maxLen = order + 1;
    const HEAP32 = getHeap32();
    const base = 2048;
    if (HEAP32.length < base + maxLen)
        throw new Error("WASM memory exhausted");
    const len = moduleInstance._lr_characteristic_polynomial(lrHandle, base * 4, maxLen);
    const out = [];
    for (let i = 0; i < len; i++)
        out.push(HEAP32[base + i]);
    return out;
}
export function lrGetTransitionMatrixSize() {
    if (!moduleInstance || lrHandle === null)
        throw new Error("Linear recurrence not created. Call lrCreate first.");
    return moduleInstance._lr_transition_matrix_size(lrHandle);
}
export function lrGetTransitionMatrixData() {
    if (!moduleInstance || lrHandle === null)
        throw new Error("Linear recurrence not created. Call lrCreate first.");
    const n = moduleInstance._lr_transition_matrix_size(lrHandle);
    const size = n * n;
    const HEAP32 = getHeap32();
    const base = 2048;
    if (HEAP32.length < base + size)
        throw new Error("WASM memory exhausted");
    moduleInstance._lr_transition_matrix_data(lrHandle, base * 4, size);
    const out = [];
    for (let i = 0; i < size; i++)
        out.push(HEAP32[base + i]);
    return out;
}
export function lrEvaluatePolyAtMatrix() {
    if (!moduleInstance || lrHandle === null)
        throw new Error("Linear recurrence not created. Call lrCreate first.");
    const n = moduleInstance._lr_transition_matrix_size(lrHandle);
    const size = n * n;
    const HEAP32 = getHeap32();
    const base = 2048;
    if (HEAP32.length < base + size)
        throw new Error("WASM memory exhausted");
    moduleInstance._lr_evaluate_poly_at_matrix(lrHandle, base * 4, size);
    const out = [];
    for (let i = 0; i < size; i++)
        out.push(HEAP32[base + i]);
    return out;
}
export function wasmMatrixPower(data, n, exponent) {
    const HEAP32 = getHeap32();
    const size = n * n;
    if (data.length !== size)
        throw new Error(`Expected ${size} elements for ${n}×${n} matrix.`);
    const inBase = 2048;
    const outBase = 2048 + size;
    if (HEAP32.length < outBase + size)
        throw new Error("WASM memory exhausted");
    copyToHeap(HEAP32, toInt32Array(data), inBase);
    moduleInstance._wasm_matrix_power(inBase * 4, n, exponent, outBase * 4);
    const out = [];
    for (let i = 0; i < size; i++)
        out.push(HEAP32[outBase + i]);
    return out;
}
export function wasmMatrixTimesConst(data, n, scalar) {
    const HEAP32 = getHeap32();
    const size = n * n;
    if (data.length !== size)
        throw new Error(`Expected ${size} elements for ${n}×${n} matrix.`);
    const inBase = 2048;
    const outBase = 2048 + size;
    if (HEAP32.length < outBase + size)
        throw new Error("WASM memory exhausted");
    copyToHeap(HEAP32, toInt32Array(data), inBase);
    moduleInstance._wasm_matrix_times_const(inBase * 4, n, scalar, outBase * 4);
    const out = [];
    for (let i = 0; i < size; i++)
        out.push(HEAP32[outBase + i]);
    return out;
}
export function wasmMatrixAdd(dataA, dataB, n) {
    const HEAP32 = getHeap32();
    const size = n * n;
    if (dataA.length !== size || dataB.length !== size)
        throw new Error(`Expected ${size} elements for ${n}×${n} matrix.`);
    const baseA = 2048;
    const baseB = 2048 + size;
    const baseOut = 2048 + size * 2;
    if (HEAP32.length < baseOut + size)
        throw new Error("WASM memory exhausted");
    copyToHeap(HEAP32, toInt32Array(dataA), baseA);
    copyToHeap(HEAP32, toInt32Array(dataB), baseB);
    moduleInstance._wasm_matrix_add(baseA * 4, baseB * 4, n, baseOut * 4);
    const out = [];
    for (let i = 0; i < size; i++)
        out.push(HEAP32[baseOut + i]);
    return out;
}
