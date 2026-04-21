import wasmSample from "./wasm/wasm_out_v1/wasm_sample";
import type {WasmSampleModule} from "./wasm/wasm_out_v1/wasm_sample";

let moduleInstance: WasmSampleModule | null = null;

function isNodeRuntime(): boolean {
    const g = typeof globalThis !== "undefined" ? globalThis : undefined;
    const proc = g && (g as { process?: { versions?: { node?: string } } }).process;
    return typeof proc?.versions?.node === "string";
}

/**
 * Node path: `fetch()` cannot load `file://` URLs, so read `wasm_sample.wasm`
 * from disk and hand the bytes to Emscripten via `{ wasmBinary }`.
 *
 * The Emscripten loader is obtained via a string-variable dynamic import so
 * Node's strict ESM resolver can append the required `.js` extension. esbuild
 * does not analyze the dynamic specifier, so the browser bundle continues to
 * rely on the static import at the top of this file (which the
 * `wasm-external` build plugin rewrites to an external URL).
 */
async function loadWasmSampleInNode(): Promise<WasmSampleModule> {
    const nodeFs = "node:fs";
    const nodeUrl = "node:url";
    const nodePath = "node:path";
    const fs = (await import(nodeFs)) as { readFileSync: (p: string) => Uint8Array };
    const url = (await import(nodeUrl)) as { fileURLToPath: (u: URL | string) => string };
    const path = (await import(nodePath)) as { dirname: (p: string) => string; join: (...p: string[]) => string };
    const nodeLoaderPath = "./wasm/wasm_out_v1/wasm_sample.js";
    const { default: wasmSampleNode } = (await import(nodeLoaderPath)) as {
        default: typeof wasmSample;
    };
    const __filename = url.fileURLToPath(import.meta.url);
    const __dirname = path.dirname(__filename);
    const wasmPath = path.join(__dirname, "wasm", "wasm_out_v1", "wasm_sample.wasm");
    const wasmBinary = fs.readFileSync(wasmPath);
    return wasmSampleNode({ wasmBinary });
}

/** Browser path: Emscripten fetches `wasm_sample.wasm` itself, relative to `wasm_sample.js`. */
async function loadWasmSampleInBrowser(): Promise<WasmSampleModule> {
    return wasmSample();
}

async function createModulePromise(): Promise<WasmSampleModule> {
    const mod = isNodeRuntime()
        ? await loadWasmSampleInNode()
        : await loadWasmSampleInBrowser();
    moduleInstance = mod;
    return mod;
}

export const modulePromise: Promise<WasmSampleModule> = createModulePromise();

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

function requireModuleFunctions<K extends string>(...names: K[]): void {
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
export function wasm_number_of_sequences(arr: readonly number[] | Int32Array, seq: readonly number[] | Int32Array): number {
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
function build_multi_dimensional_array(flatData: Int32Array, dimensions: Int32Array): any {
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
export function wasm_number_of_sequences_all(arr: readonly number[] | Int32Array, seq: readonly number[] | Int32Array): any {
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
export function wasm_get_gl_n_zm_size(n: number, m: number): number {
    if (!moduleInstance) {
        throw new Error(
            "WASM module not initialized. Call and await initWasm() before using WASM functions."
        );
    }
    return moduleInstance._wasm_get_gl_n_zm_size(n, m);
}

export interface WasmMatrixSet {
    ptr: number;
    count: number;
    n: number;
    modulus: number;
    toFlatArray(): number[];
    free(): void;
}

/**
 * Generate all invertible n×n matrices over Z_m (the general linear group).
 * Returns a handle whose .ptr lives on the WASM heap and can be passed
 * directly to other WASM functions (zero copy).
 * Call .free() when done to release WASM memory.
 */
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
            const result: number[] = [];
            for (let i = 0; i < total; i++) {
                result.push(H[startIdx + i]);
            }
            return result;
        },
        free(): void {
            moduleInstance!._free(bufPtr);
        },
    };
}

/**
 * Check whether a set of n×n matrices forms a group under multiplication.
 *
 * Accepts either:
 *   - A WasmMatrixSet (zero-copy, pointer reuse)
 *   - A flat JS array + count + n + modulus (copies to WASM heap)
 */
export function wasm_is_matrix_group(input: WasmMatrixSet): boolean;
export function wasm_is_matrix_group(
    data: readonly number[] | Int32Array,
    count: number,
    n: number,
    modulus: number,
): boolean;
export function wasm_is_matrix_group(
    dataOrSet: WasmMatrixSet | readonly number[] | Int32Array,
    count?: number,
    n?: number,
    modulus?: number,
): boolean {
    if (typeof (dataOrSet as WasmMatrixSet).ptr === "number" &&
        typeof (dataOrSet as WasmMatrixSet).count === "number") {
        const set = dataOrSet as WasmMatrixSet;
        return moduleInstance!._wasm_is_matrix_group(
            set.ptr, set.count, set.n, set.modulus) === 1;
    }

    const data = dataOrSet as readonly number[] | Int32Array;
    const HEAP32 = getHeap32();
    const data32 = toInt32Array(data);

    const totalSize = count! * n! * n!;
    if (data32.length !== totalSize) {
        throw new Error(
            `Expected ${totalSize} elements (${count} matrices of ${n}×${n}), got ${data32.length}`);
    }

    const dataOffsetInInts = 1024;
    if (HEAP32.length < dataOffsetInInts + totalSize) {
        throw new Error("WASM memory exhausted");
    }

    copyToHeap(HEAP32, data32, dataOffsetInInts);
    const dataPtr = dataOffsetInInts * 4;

    return moduleInstance!._wasm_is_matrix_group(dataPtr, count!, n!, modulus!) === 1;
}

/**
 * Calculate the determinant of a square matrix.
 *
 * @param data Array of integers representing the matrix in row-major order
 * @param n Size of the square matrix (n×n). Total elements should be n²
 * @return Determinant of the matrix
 */
export function wasm_matrix_det(data: readonly number[] | Int32Array, n: number): number {
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
    return moduleInstance!._wasm_matrix_det(dataPtr, n);
}

/**
 * Compute the inverse of an n×n matrix over Z_m (modular arithmetic).
 * Uses the adjugate/determinant method in C++.
 * Requires m to be prime and det(A) != 0 mod m.
 *
 * @param data Row-major n×n matrix entries (integers in [0, m))
 * @param n    Matrix dimension
 * @param m    Prime modulus
 * @return     Row-major n×n inverse matrix entries
 */
export function wasm_matrix_inverse_mod(
    data: readonly number[] | Int32Array,
    n: number,
    m: number,
): number[] {
    const HEAP32 = getHeap32();
    const data32 = toInt32Array(data);

    const totalSize = n * n;
    if (data32.length !== totalSize) {
        throw new Error(`Expected ${totalSize} elements for a ${n}×${n} matrix, got ${data32.length}`);
    }

    const dataOffsetInInts = 1024;
    const outOffsetInInts = dataOffsetInInts + totalSize;
    if (HEAP32.length < outOffsetInInts + totalSize) {
        throw new Error("WASM memory exhausted");
    }

    copyToHeap(HEAP32, data32, dataOffsetInInts);

    const dataPtr = dataOffsetInInts * 4;
    const outPtr = outOffsetInInts * 4;

    moduleInstance!._wasm_matrix_inverse_mod(dataPtr, n, m, outPtr);

    const result: number[] = [];
    for (let i = 0; i < totalSize; i++) {
        result.push(HEAP32[outOffsetInInts + i]);
    }
    return result;
}

// --- Game of Life (C++ WASM) ---
const GOL_DEFAULT_SIZE = 40;
let golHandle: number | null = null;
let golSize: number = GOL_DEFAULT_SIZE;

export type GolTopologyMode = 0 | 1 | 2; // Finite2D, Torus2D, Cylinder2D

export function golCreate(size: number): void {
    if (!moduleInstance) throw new Error("WASM module not initialized.");
    if (golHandle !== null) {
        moduleInstance._gol_destroy(golHandle);
    }
    golHandle = moduleInstance._gol_create(size);
    golSize = size;
}

export function golDestroy(): void {
    if (moduleInstance && golHandle !== null) {
        moduleInstance._gol_destroy(golHandle);
        golHandle = null;
    }
}

export function golInit(): void {
    if (!moduleInstance || golHandle === null) throw new Error("GoL not created. Call golCreate first.");
    moduleInstance._gol_init(golHandle);
}

export function golRandomInit(liveProb: number): void {
    if (!moduleInstance || golHandle === null) throw new Error("GoL not created. Call golCreate first.");
    moduleInstance._gol_random_init(golHandle, liveProb);
}

export function golRandomInitWithSeed(liveProb: number, seed: number): void {
    if (!moduleInstance || golHandle === null) throw new Error("GoL not created. Call golCreate first.");
    moduleInstance._gol_random_init_seed(golHandle, liveProb, seed);
}

export function golGetSeed(): number {
    if (!moduleInstance || golHandle === null) return 0;
    return moduleInstance._gol_get_seed(golHandle);
}

export function golEvolve(): void {
    if (!moduleInstance || golHandle === null) throw new Error("GoL not created. Call golCreate first.");
    moduleInstance._gol_evolve(golHandle);
}

export function golSetTopology(mode: GolTopologyMode): void {
    if (!moduleInstance || golHandle === null) throw new Error("GoL not created. Call golCreate first.");
    moduleInstance._gol_set_topology(golHandle, mode);
}

export function golGetLiveCells(): { x: number; y: number }[] {
    if (!moduleInstance || golHandle === null) throw new Error("GoL not created. Call golCreate first.");
    const HEAP32 = getHeap32();
    const maxCount = golSize * golSize;
    const bytes = 2 * maxCount * 4;
    const ptr = moduleInstance._malloc(bytes);
    if (ptr === 0) throw new Error("WASM malloc failed for GoL live cells.");
    try {
        const count = moduleInstance._gol_get_live_cells(golHandle, ptr, maxCount);
        const out: { x: number; y: number }[] = [];
        const base = ptr / 4;
        for (let i = 0; i < count; i++) {
            out.push({ x: HEAP32[base + 2 * i], y: HEAP32[base + 2 * i + 1] });
        }
        return out;
    } finally {
        moduleInstance._free(ptr);
    }
}

export function golGetSize(): number {
    return golSize;
}

export function golIsCreated(): boolean {
    return golHandle !== null;
}

// --- Bars game (opaque handle) ---
const BARS_GAME_BASE_INTS = 1280;
let barsGameHandle: number | null = null;

const m = (): { _bars_game_create: () => number; _bars_game_destroy: (h: number) => void; _bars_game_set_seed: (h: number, s: number) => void; _bars_game_init: (h: number) => void; _bars_game_get_state: (h: number, ptr: number) => void; _bars_game_get_future_state: (h: number, i: number, ptr: number) => void; _bars_game_apply_choice: (h: number, i: number) => void; _bars_game_is_ended: (h: number) => number; _bars_game_state_size: (h: number) => number; _bars_game_max_val: (h: number) => number } => moduleInstance as any;

export function barsGameCreate(): void {
    if (!moduleInstance) throw new Error("WASM module not initialized.");
    if (barsGameHandle !== null) {
        m()._bars_game_destroy(barsGameHandle);
    }
    barsGameHandle = m()._bars_game_create();
}

export function barsGameDestroy(): void {
    if (moduleInstance && barsGameHandle !== null) {
        m()._bars_game_destroy(barsGameHandle);
        barsGameHandle = null;
    }
}

export function barsGameSetSeed(seed: number): void {
    if (!moduleInstance || barsGameHandle === null) throw new Error("Bars game not created. Call barsGameCreate first.");
    m()._bars_game_set_seed(barsGameHandle, seed);
}

export function barsGameInit(): void {
    if (!moduleInstance || barsGameHandle === null) throw new Error("Bars game not created. Call barsGameCreate first.");
    m()._bars_game_init(barsGameHandle);
}

export function barsGameGetState(): number[] {
    if (!moduleInstance || barsGameHandle === null) throw new Error("Bars game not created. Call barsGameCreate first.");
    const HEAP32 = getHeap32();
    const size = m()._bars_game_state_size(barsGameHandle);
    if (HEAP32.length < BARS_GAME_BASE_INTS + size) throw new Error("WASM memory exhausted");
    m()._bars_game_get_state(barsGameHandle, BARS_GAME_BASE_INTS * 4);
    const out: number[] = [];
    for (let i = 0; i < size; i++) out.push(HEAP32[BARS_GAME_BASE_INTS + i]);
    return out;
}

export function barsGameGetFutureState(choiceIndex: number): number[] {
    if (!moduleInstance || barsGameHandle === null) throw new Error("Bars game not created. Call barsGameCreate first.");
    const HEAP32 = getHeap32();
    const size = m()._bars_game_state_size(barsGameHandle);
    if (HEAP32.length < BARS_GAME_BASE_INTS + size) throw new Error("WASM memory exhausted");
    m()._bars_game_get_future_state(barsGameHandle, choiceIndex, BARS_GAME_BASE_INTS * 4);
    const out: number[] = [];
    for (let i = 0; i < size; i++) out.push(HEAP32[BARS_GAME_BASE_INTS + i]);
    return out;
}

export function barsGameApplyChoice(index: number): void {
    if (!moduleInstance || barsGameHandle === null) throw new Error("Bars game not created. Call barsGameCreate first.");
    m()._bars_game_apply_choice(barsGameHandle, index);
}

export function barsGameIsEnded(): boolean {
    if (!moduleInstance || barsGameHandle === null) return true;
    return m()._bars_game_is_ended(barsGameHandle) !== 0;
}

export function barsGameStateSize(): number {
    if (!moduleInstance || barsGameHandle === null) return 0;
    return m()._bars_game_state_size(barsGameHandle);
}

export function barsGameMaxVal(): number {
    if (!moduleInstance || barsGameHandle === null) return 2000;
    return m()._bars_game_max_val(barsGameHandle);
}

// --- Linear recurrence (opaque handle) ---
const LR_DEFAULT_THRESHOLD = 20;
let lrHandle: number | null = null;

export type I64Parts = { lo: number; hi: number };
export type RationalI64Parts = { m: I64Parts; n: I64Parts };

const U32 = 0x100000000;

function i64FromNumber(x: number): I64Parts {
    if (!Number.isFinite(x) || !Number.isSafeInteger(x)) {
        throw new Error("Expected a safe integer for int64 conversion.");
    }
    // Pack into two's complement i64 split into (lo, hi).
    const neg = x < 0;
    let abs = Math.abs(x);
    const loU = abs % U32;
    let hiU = (abs - loU) / U32;

    let lo = (loU >>> 0);
    if (neg) {
        // Two's complement: (~u + 1)
        lo = ((~lo + 1) >>> 0);
        hiU = (~hiU) >>> 0;
        if (lo === 0) {
            hiU = (hiU + 1) >>> 0;
        }
    }
    const hi = (hiU | 0);
    return { lo: lo | 0, hi };
}

function rationalFromInt(x: number): RationalI64Parts {
    return { m: i64FromNumber(x), n: { lo: 1, hi: 0 } };
}

function toRationalParts(x: number | RationalI64Parts): RationalI64Parts {
    if (typeof x === "number") {
        return rationalFromInt(x);
    }
    return x;
}

function writeI64ToHeap32(HEAP32: Int32Array, base: number, idxWords: number, v: I64Parts): void {
    HEAP32[base + idxWords] = v.lo;
    HEAP32[base + idxWords + 1] = v.hi;
}

function writeRationalToHeap32(HEAP32: Int32Array, base: number, idxRational: number, r: RationalI64Parts): void {
    const off = idxRational * 4;
    writeI64ToHeap32(HEAP32, base, off, r.m);
    writeI64ToHeap32(HEAP32, base, off + 2, r.n);
}

function readI64FromHeap32(HEAP32: Int32Array, base: number, idxWords: number): I64Parts {
    return { lo: HEAP32[base + idxWords], hi: HEAP32[base + idxWords + 1] };
}

function readRationalFromHeap32(HEAP32: Int32Array, base: number, idxRational: number): RationalI64Parts {
    const off = idxRational * 4;
    return { m: readI64FromHeap32(HEAP32, base, off), n: readI64FromHeap32(HEAP32, base, off + 2) };
}

function i64ToSafeNumber(p: I64Parts, context: string): number {
    // Only convert when value fits in safe integer range.
    // Fast-path: i32
    if (p.hi === 0) {
        const v = (p.lo >>> 0);
        if (!Number.isSafeInteger(v)) {
            throw new Error(`${context}: value not a safe integer.`);
        }
        return v;
    }
    if (p.hi === -1) {
        const v = (p.lo >>> 0) - U32;
        if (!Number.isSafeInteger(v)) {
            throw new Error(`${context}: value not a safe integer.`);
        }
        return v;
    }
    // General case: may still be safe, but we avoid bigint in this project.
    throw new Error(`${context}: int64 value outside supported JS safe range.`);
}

function requireIntegerRational(r: RationalI64Parts, context: string): number {
    if (!(r.n.hi === 0 && (r.n.lo >>> 0) === 1)) {
        throw new Error(`${context}: expected integer (denominator 1).`);
    }
    return i64ToSafeNumber(r.m, context);
}

export function lrCreate(coeffs: Array<number | RationalI64Parts>, recursiveThreshold: number = LR_DEFAULT_THRESHOLD): void {
    if (!moduleInstance) throw new Error("WASM module not initialized.");
    if (lrHandle !== null) {
        moduleInstance._lr_destroy(lrHandle);
    }
    const HEAP32 = getHeap32();
    const base = 2048;
    const len = coeffs.length;
    const words = len * 4;
    if (HEAP32.length < base + words) throw new Error("WASM memory exhausted");
    for (let i = 0; i < len; i++) {
        writeRationalToHeap32(HEAP32, base, i, toRationalParts(coeffs[i]));
    }
    lrHandle = moduleInstance._lr_create(base * 4, len, recursiveThreshold);
}

export function lrDestroy(): void {
    if (moduleInstance && lrHandle !== null) {
        moduleInstance._lr_destroy(lrHandle);
        lrHandle = null;
    }
}

export function lrOrder(): number {
    if (!moduleInstance || lrHandle === null) throw new Error("Linear recurrence not created. Call lrCreate first.");
    return moduleInstance._lr_order(lrHandle);
}

/** Returns a(n) as number (full int64 range via low + high*2^32; fits in number for |value| < 2^53). */
export function lrEvaluateRational(initialValues: Array<number | RationalI64Parts>, n: number): RationalI64Parts {
    if (!moduleInstance || lrHandle === null) throw new Error("Linear recurrence not created. Call lrCreate first.");
    const HEAP32 = getHeap32();
    const initBase = 2048;
    const resultBase = 4096;
    const len = initialValues.length;
    const initWords = len * 4;
    if (HEAP32.length < resultBase + 4) throw new Error("WASM memory exhausted");
    if (HEAP32.length < initBase + initWords) throw new Error("WASM memory exhausted");
    for (let i = 0; i < len; i++) {
        writeRationalToHeap32(HEAP32, initBase, i, toRationalParts(initialValues[i]));
    }
    moduleInstance._lr_evaluate(lrHandle, initBase * 4, len, n, resultBase * 4);
    return readRationalFromHeap32(HEAP32, resultBase, 0);
}

/** Back-compat: returns integer a(n) as number (only if denominator == 1). */
export function lrEvaluate(initialValues: Array<number | RationalI64Parts>, n: number): number {
    const r = lrEvaluateRational(initialValues, n);
    return requireIntegerRational(r, "lrEvaluate");
}

export function lrGetCharacteristicPolynomialRational(): RationalI64Parts[] {
    if (!moduleInstance || lrHandle === null) throw new Error("Linear recurrence not created. Call lrCreate first.");
    const order = moduleInstance._lr_order(lrHandle);
    const maxLen = order + 1;
    const HEAP32 = getHeap32();
    const base = 2048;
    const words = maxLen * 4;
    if (HEAP32.length < base + words) throw new Error("WASM memory exhausted");
    const len = moduleInstance._lr_characteristic_polynomial(lrHandle, base * 4, maxLen);
    const out: RationalI64Parts[] = [];
    for (let i = 0; i < len; i++) out.push(readRationalFromHeap32(HEAP32, base, i));
    return out;
}

/** Back-compat: returns integer coefficients (only if all denominators == 1). */
export function lrGetCharacteristicPolynomial(): number[] {
    const rs = lrGetCharacteristicPolynomialRational();
    const out: number[] = [];
    for (let i = 0; i < rs.length; i++) {
        out.push(requireIntegerRational(rs[i], "lrGetCharacteristicPolynomial"));
    }
    return out;
}

export function lrGetTransitionMatrixSize(): number {
    if (!moduleInstance || lrHandle === null) throw new Error("Linear recurrence not created. Call lrCreate first.");
    return moduleInstance._lr_transition_matrix_size(lrHandle);
}

export function lrGetTransitionMatrixDataRational(): RationalI64Parts[] {
    if (!moduleInstance || lrHandle === null) throw new Error("Linear recurrence not created. Call lrCreate first.");
    const n = moduleInstance._lr_transition_matrix_size(lrHandle);
    const size = n * n;
    const HEAP32 = getHeap32();
    const base = 2048;
    const words = size * 4;
    if (HEAP32.length < base + words) throw new Error("WASM memory exhausted");
    moduleInstance._lr_transition_matrix_data(lrHandle, base * 4, size);
    const out: RationalI64Parts[] = [];
    for (let i = 0; i < size; i++) out.push(readRationalFromHeap32(HEAP32, base, i));
    return out;
}

/** Back-compat: returns integer matrix entries (only if all denominators == 1). */
export function lrGetTransitionMatrixData(): number[] {
    const rs = lrGetTransitionMatrixDataRational();
    const out: number[] = [];
    for (let i = 0; i < rs.length; i++) {
        out.push(requireIntegerRational(rs[i], "lrGetTransitionMatrixData"));
    }
    return out;
}

export function lrEvaluatePolyAtMatrixRational(): RationalI64Parts[] {
    if (!moduleInstance || lrHandle === null) throw new Error("Linear recurrence not created. Call lrCreate first.");
    const n = moduleInstance._lr_transition_matrix_size(lrHandle);
    const size = n * n;
    const HEAP32 = getHeap32();
    const base = 2048;
    const words = size * 4;
    if (HEAP32.length < base + words) throw new Error("WASM memory exhausted");
    moduleInstance._lr_evaluate_poly_at_matrix(lrHandle, base * 4, size);
    const out: RationalI64Parts[] = [];
    for (let i = 0; i < size; i++) out.push(readRationalFromHeap32(HEAP32, base, i));
    return out;
}

/** Back-compat: returns integer matrix entries (only if all denominators == 1). */
export function lrEvaluatePolyAtMatrix(): number[] {
    const rs = lrEvaluatePolyAtMatrixRational();
    const out: number[] = [];
    for (let i = 0; i < rs.length; i++) {
        out.push(requireIntegerRational(rs[i], "lrEvaluatePolyAtMatrix"));
    }
    return out;
}

export function wasmMatrixPower(data: number[], n: number, exponent: number): number[] {
    const HEAP32 = getHeap32();
    const size = n * n;
    if (data.length !== size) throw new Error(`Expected ${size} elements for ${n}×${n} matrix.`);
    const inBase = 2048;
    const outBase = 2048 + size;
    if (HEAP32.length < outBase + size) throw new Error("WASM memory exhausted");
    copyToHeap(HEAP32, toInt32Array(data), inBase);
    moduleInstance!._wasm_matrix_power(inBase * 4, n, exponent, outBase * 4);
    const out: number[] = [];
    for (let i = 0; i < size; i++) out.push(HEAP32[outBase + i]);
    return out;
}

export function wasmMatrixTimesConst(data: number[], n: number, scalar: number): number[] {
    const HEAP32 = getHeap32();
    const size = n * n;
    if (data.length !== size) throw new Error(`Expected ${size} elements for ${n}×${n} matrix.`);
    const inBase = 2048;
    const outBase = 2048 + size;
    if (HEAP32.length < outBase + size) throw new Error("WASM memory exhausted");
    copyToHeap(HEAP32, toInt32Array(data), inBase);
    moduleInstance!._wasm_matrix_times_const(inBase * 4, n, scalar, outBase * 4);
    const out: number[] = [];
    for (let i = 0; i < size; i++) out.push(HEAP32[outBase + i]);
    return out;
}

export function wasmMatrixAdd(dataA: number[], dataB: number[], n: number): number[] {
    const HEAP32 = getHeap32();
    const size = n * n;
    if (dataA.length !== size || dataB.length !== size) throw new Error(`Expected ${size} elements for ${n}×${n} matrix.`);
    const baseA = 2048;
    const baseB = 2048 + size;
    const baseOut = 2048 + size * 2;
    if (HEAP32.length < baseOut + size) throw new Error("WASM memory exhausted");
    copyToHeap(HEAP32, toInt32Array(dataA), baseA);
    copyToHeap(HEAP32, toInt32Array(dataB), baseB);
    moduleInstance!._wasm_matrix_add(baseA * 4, baseB * 4, n, baseOut * 4);
    const out: number[] = [];
    for (let i = 0; i < size; i++) out.push(HEAP32[baseOut + i]);
    return out;
}

// --- Graph demo (adjacency-matrix input) ---

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
        const base = ptr / 4;
        copyToHeap(HEAP32, a32, base);
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
    const bytesEdgeCount = 4;
    const bytesThresholdMilli = 4;
    const adjPtr = m._malloc(bytesAdj);
    const edgeCountPtr = m._malloc(bytesEdgeCount);
    const thresholdMilliPtr = m._malloc(bytesThresholdMilli);
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

export function wasmGraphMetricDimension(adj01: readonly number[] | Int32Array, n: number): { dimension: number; basis: number[] } {
    requireModuleFunctions("_wasm_graph_metric_dimension", "_malloc", "_free");
    const m = moduleInstance as any;
    const HEAP32 = getHeap32();
    const a32 = toInt32Array(adj01);
    const needed = n * n;
    if (a32.length !== needed) {
        throw new Error(`wasmGraphMetricDimension: expected ${needed} entries for ${n}×${n} adjacency, got ${a32.length}`);
    }
    const bytesAdj = needed * 4;
    const bytesDim = 4;
    const bytesBasis = n * 4;
    const adjPtr = m._malloc(bytesAdj);
    const dimPtr = m._malloc(bytesDim);
    const basisPtr = m._malloc(bytesBasis);
    if (adjPtr === 0 || dimPtr === 0 || basisPtr === 0) {
        if (adjPtr) {
            m._free(adjPtr);
        }
        if (dimPtr) {
            m._free(dimPtr);
        }
        if (basisPtr) {
            m._free(basisPtr);
        }
        throw new Error("wasmGraphMetricDimension: malloc failed");
    }
    try {
        copyToHeap(HEAP32, a32, adjPtr / 4);
        // basis_max = n is enough for all cases.
        m._wasm_graph_metric_dimension(n, adjPtr, dimPtr, basisPtr, n);
        const dim = HEAP32[dimPtr / 4];
        const basis: number[] = [];
        const base = basisPtr / 4;
        for (let i = 0; i < dim; i++) {
            basis.push(HEAP32[base + i]);
        }
        return { dimension: dim, basis };
    } finally {
        m._free(adjPtr);
        m._free(dimPtr);
        m._free(basisPtr);
    }
}

export type GraphResolveMode = 0 | 1 | 2; // 0=Nodes, 1=Edges, 2=NodesAndEdges (strict union)
const GRAPH_RESOLVE_LIST_MAX_INTS_PER_MODE = 100_000;

function nCk(n: number, k: number): number {
    if (k < 0 || k > n) {
        return 0;
    }
    if (k === 0 || k === n) {
        return 1;
    }
    let kk = k;
    if (kk > n - kk) {
        kk = n - kk;
    }
    let num = 1;
    let den = 1;
    for (let i = 1; i <= kk; i++) {
        num *= (n - (kk - i));
        den *= i;
    }
    return Math.floor(num / den);
}

function maxSubsetListInts(n: number, maxK: number): number {
    let total = 0;
    for (let k = 1; k <= maxK; k++) {
        const count = nCk(n, k);
        total += count * (1 + k);
    }
    return total;
}

function clampListMaxKForCapacity(
    n: number,
    requestedListMaxK: number,
    maxIntsPerMode: number,
): { effectiveListMaxK: number; listFlatMaxInts: number; rangeLimited: boolean } {
    let effectiveListMaxK = requestedListMaxK;
    if (effectiveListMaxK < 0) {
        effectiveListMaxK = n;
    }
    if (effectiveListMaxK > n) {
        effectiveListMaxK = n;
    }

    let rangeLimited = false;
    let listFlatMaxInts = maxSubsetListInts(n, effectiveListMaxK);
    if (!Number.isFinite(listFlatMaxInts) || listFlatMaxInts > maxIntsPerMode) {
        rangeLimited = true;
        while (effectiveListMaxK > 0) {
            const candidate = maxSubsetListInts(n, effectiveListMaxK);
            if (Number.isFinite(candidate) && candidate <= maxIntsPerMode) {
                listFlatMaxInts = candidate;
                break;
            }
            effectiveListMaxK--;
        }
        if (effectiveListMaxK === 0) {
            listFlatMaxInts = 0;
        }
    }
    return { effectiveListMaxK, listFlatMaxInts, rangeLimited };
}

function wasmGraphResolvingSubsetsFromDistInternal(
    wasmFunctionName:
        "_wasm_graph_resolving_subsets_from_dist"
        | "_wasm_graph_resolving_subsets_with_non_resolving_size_minus_one_from_dist",
    adj01: readonly number[] | Int32Array,
    distFlat: readonly number[] | Int32Array,
    n: number,
    mode: GraphResolveMode,
    listMaxK: number = 3,
): { minDimension: number; smallestBasis: number[]; subsets: number[][]; truncated: boolean } {
    requireModuleFunctions(wasmFunctionName, "_malloc", "_free");
    const m = moduleInstance as any;
    const HEAP32 = getHeap32();
    const a32 = toInt32Array(adj01);
    const d32 = toInt32Array(distFlat);
    const needed = n * n;
    if (a32.length !== needed) {
        throw new Error(`wasmGraphResolvingSubsetsFromDist: expected ${needed} entries for ${n}×${n} adjacency, got ${a32.length}`);
    }
    if (d32.length !== needed) {
        throw new Error(`wasmGraphResolvingSubsetsFromDist: expected ${needed} entries for ${n}×${n} distances, got ${d32.length}`);
    }
    const clamped = clampListMaxKForCapacity(
        n,
        listMaxK,
        GRAPH_RESOLVE_LIST_MAX_INTS_PER_MODE,
    );
    listMaxK = clamped.effectiveListMaxK;

    const bytesAdj = needed * 4;
    const bytesDist = needed * 4;
    const bytesMinDim = 4;
    const bytesSmallest = n * 4;
    const bytesCount = 4;
    const bytesUsed = 4;
    const bytesTrunc = 4;
    const listFlatMaxInts = clamped.listFlatMaxInts;
    const listFlatIntsForAlloc = listFlatMaxInts > 0 ? listFlatMaxInts : 1;
    const bytesListFlat = listFlatIntsForAlloc * 4;

    const adjPtr = m._malloc(bytesAdj);
    const distPtr = m._malloc(bytesDist);
    const minDimPtr = m._malloc(bytesMinDim);
    const smallestPtr = m._malloc(bytesSmallest);
    const countPtr = m._malloc(bytesCount);
    const usedPtr = m._malloc(bytesUsed);
    const truncPtr = m._malloc(bytesTrunc);
    const listFlatPtr = m._malloc(bytesListFlat);

    if (adjPtr === 0 || distPtr === 0 || minDimPtr === 0 || smallestPtr === 0 ||
        countPtr === 0 || usedPtr === 0 || truncPtr === 0 || listFlatPtr === 0) {
        if (adjPtr) { m._free(adjPtr); }
        if (distPtr) { m._free(distPtr); }
        if (minDimPtr) { m._free(minDimPtr); }
        if (smallestPtr) { m._free(smallestPtr); }
        if (countPtr) { m._free(countPtr); }
        if (usedPtr) { m._free(usedPtr); }
        if (truncPtr) { m._free(truncPtr); }
        if (listFlatPtr) { m._free(listFlatPtr); }
        throw new Error("wasmGraphResolvingSubsetsFromDist: malloc failed");
    }

    try {
        copyToHeap(HEAP32, a32, adjPtr / 4);
        copyToHeap(HEAP32, d32, distPtr / 4);

        const ok = m[wasmFunctionName](
            n,
            adjPtr,
            distPtr,
            mode,
            listMaxK,
            minDimPtr,
            smallestPtr,
            n,
            countPtr,
            usedPtr,
            listFlatPtr,
            listFlatMaxInts,
            truncPtr,
        );
        if (ok === 0) {
            throw new Error("wasmGraphResolvingSubsetsFromDist: wasm call failed");
        }

        const minDim = HEAP32[minDimPtr / 4];
        const smallestBasis: number[] = [];
        const smallestBase = smallestPtr / 4;
        for (let i = 0; i < minDim; i++) {
            smallestBasis.push(HEAP32[smallestBase + i]);
        }

        const usedInts = HEAP32[usedPtr / 4];
        const truncated = HEAP32[truncPtr / 4] !== 0 || clamped.rangeLimited;
        const subsets: number[][] = [];
        const flatBase = listFlatPtr / 4;
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

        return { minDimension: minDim, smallestBasis, subsets, truncated };
    } finally {
        m._free(adjPtr);
        m._free(distPtr);
        m._free(minDimPtr);
        m._free(smallestPtr);
        m._free(countPtr);
        m._free(usedPtr);
        m._free(truncPtr);
        m._free(listFlatPtr);
    }
}

export function wasmGraphResolvingSubsetsFromDist(
    adj01: readonly number[] | Int32Array,
    distFlat: readonly number[] | Int32Array,
    n: number,
    mode: GraphResolveMode,
    listMaxK: number = 3,
): { minDimension: number; smallestBasis: number[]; subsets: number[][]; truncated: boolean } {
    return wasmGraphResolvingSubsetsFromDistInternal(
        "_wasm_graph_resolving_subsets_from_dist",
        adj01,
        distFlat,
        n,
        mode,
        listMaxK,
    );
}

export function wasmGraphResolvingSubsetsWithNonResolvingSizeMinusOneFromDist(
    adj01: readonly number[] | Int32Array,
    distFlat: readonly number[] | Int32Array,
    n: number,
    mode: GraphResolveMode,
    listMaxK: number = 3,
): { minDimension: number; smallestBasis: number[]; subsets: number[][]; truncated: boolean } {
    return wasmGraphResolvingSubsetsFromDistInternal(
        "_wasm_graph_resolving_subsets_with_non_resolving_size_minus_one_from_dist",
        adj01,
        distFlat,
        n,
        mode,
        listMaxK,
    );
}

export function wasmGraphResolvingSubsetsAllModesWithNonResolvingSizeMinusOneFromDist(
    adj01: readonly number[] | Int32Array,
    distFlat: readonly number[] | Int32Array,
    n: number,
    listMaxK: number = 3,
): {
    node: { minDimension: number; smallestBasis: number[]; subsets: number[][]; truncated: boolean };
    edge: { minDimension: number; smallestBasis: number[]; subsets: number[][]; truncated: boolean };
    mixed: { minDimension: number; smallestBasis: number[]; subsets: number[][]; truncated: boolean };
} {
    requireModuleFunctions(
        "_wasm_graph_resolving_subsets_all_modes_with_non_resolving_size_minus_one_from_dist",
        "_malloc",
        "_free",
    );
    const m = moduleInstance as any;
    const HEAP32 = getHeap32();
    const a32 = toInt32Array(adj01);
    const d32 = toInt32Array(distFlat);
    const needed = n * n;
    if (a32.length !== needed) {
        throw new Error(`wasmGraphResolvingSubsetsAllModesWithNonResolvingSizeMinusOneFromDist: expected ${needed} entries for ${n}×${n} adjacency, got ${a32.length}`);
    }
    if (d32.length !== needed) {
        throw new Error(`wasmGraphResolvingSubsetsAllModesWithNonResolvingSizeMinusOneFromDist: expected ${needed} entries for ${n}×${n} distances, got ${d32.length}`);
    }
    const clamped = clampListMaxKForCapacity(
        n,
        listMaxK,
        GRAPH_RESOLVE_LIST_MAX_INTS_PER_MODE,
    );
    listMaxK = clamped.effectiveListMaxK;

    const modeCount = 3;
    const bytesAdj = needed * 4;
    const bytesDist = needed * 4;
    const bytesMinDim3 = modeCount * 4;
    const bytesSmallest3 = modeCount * n * 4;
    const bytesCount3 = modeCount * 4;
    const bytesUsed3 = modeCount * 4;
    const bytesTrunc3 = modeCount * 4;
    const listFlatMaxIntsPerMode = clamped.listFlatMaxInts;
    const listFlatIntsForAllocPerMode =
        listFlatMaxIntsPerMode > 0 ? listFlatMaxIntsPerMode : 1;
    const bytesListFlat3 = modeCount * listFlatIntsForAllocPerMode * 4;

    const adjPtr = m._malloc(bytesAdj);
    const distPtr = m._malloc(bytesDist);
    const minDim3Ptr = m._malloc(bytesMinDim3);
    const smallest3Ptr = m._malloc(bytesSmallest3);
    const count3Ptr = m._malloc(bytesCount3);
    const used3Ptr = m._malloc(bytesUsed3);
    const trunc3Ptr = m._malloc(bytesTrunc3);
    const listFlat3Ptr = m._malloc(bytesListFlat3);
    if (adjPtr === 0 || distPtr === 0 || minDim3Ptr === 0 || smallest3Ptr === 0 ||
        count3Ptr === 0 || used3Ptr === 0 || trunc3Ptr === 0 || listFlat3Ptr === 0) {
        if (adjPtr) {
            m._free(adjPtr);
        }
        if (distPtr) {
            m._free(distPtr);
        }
        if (minDim3Ptr) {
            m._free(minDim3Ptr);
        }
        if (smallest3Ptr) {
            m._free(smallest3Ptr);
        }
        if (count3Ptr) {
            m._free(count3Ptr);
        }
        if (used3Ptr) {
            m._free(used3Ptr);
        }
        if (trunc3Ptr) {
            m._free(trunc3Ptr);
        }
        if (listFlat3Ptr) {
            m._free(listFlat3Ptr);
        }
        throw new Error("wasmGraphResolvingSubsetsAllModesWithNonResolvingSizeMinusOneFromDist: malloc failed");
    }

    try {
        copyToHeap(HEAP32, a32, adjPtr / 4);
        copyToHeap(HEAP32, d32, distPtr / 4);
        const ok = m._wasm_graph_resolving_subsets_all_modes_with_non_resolving_size_minus_one_from_dist(
            n,
            adjPtr,
            distPtr,
            listMaxK,
            minDim3Ptr,
            smallest3Ptr,
            n,
            count3Ptr,
            used3Ptr,
            listFlat3Ptr,
            listFlatMaxIntsPerMode,
            trunc3Ptr,
        );
        if (ok === 0) {
            throw new Error("wasmGraphResolvingSubsetsAllModesWithNonResolvingSizeMinusOneFromDist: wasm call failed");
        }

        const parseModeResult = (modeIdx: number): { minDimension: number; smallestBasis: number[]; subsets: number[][]; truncated: boolean } => {
            const minDim = HEAP32[minDim3Ptr / 4 + modeIdx];
            const smallestBasis: number[] = [];
            const smallestBase = smallest3Ptr / 4 + modeIdx * n;
            for (let i = 0; i < minDim; i++) {
                smallestBasis.push(HEAP32[smallestBase + i]);
            }
            const usedInts = HEAP32[used3Ptr / 4 + modeIdx];
            const truncated =
                HEAP32[trunc3Ptr / 4 + modeIdx] !== 0 || clamped.rangeLimited;
            const subsets: number[][] = [];
            const flatBase = listFlat3Ptr / 4 + modeIdx * listFlatMaxIntsPerMode;
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
            return { minDimension: minDim, smallestBasis, subsets, truncated };
        };

        return {
            node: parseModeResult(0),
            edge: parseModeResult(1),
            mixed: parseModeResult(2),
        };
    } finally {
        m._free(adjPtr);
        m._free(distPtr);
        m._free(minDim3Ptr);
        m._free(smallest3Ptr);
        m._free(count3Ptr);
        m._free(used3Ptr);
        m._free(trunc3Ptr);
        m._free(listFlat3Ptr);
    }
}

export function wasmGraphResolvingSubsetsAllModesPaginatedWithNonResolvingSizeMinusOneFromDist(
    adj01: readonly number[] | Int32Array,
    distFlat: readonly number[] | Int32Array,
    n: number,
    pageSize: number,
    pageIndexByMode: readonly [number, number, number],
): {
    node: { minDimension: number; smallestBasis: number[]; totalCount: number; pageCount: number; subsets: number[][]; truncated: boolean };
    edge: { minDimension: number; smallestBasis: number[]; totalCount: number; pageCount: number; subsets: number[][]; truncated: boolean };
    mixed: { minDimension: number; smallestBasis: number[]; totalCount: number; pageCount: number; subsets: number[][]; truncated: boolean };
} {
    requireModuleFunctions(
        "_wasm_graph_resolving_subsets_all_modes_paginated_with_non_resolving_size_minus_one_from_dist",
        "_malloc",
        "_free",
    );
    const m = moduleInstance as any;
    const HEAP32 = getHeap32();
    const a32 = toInt32Array(adj01);
    const d32 = toInt32Array(distFlat);
    const needed = n * n;
    if (a32.length !== needed) {
        throw new Error(`wasmGraphResolvingSubsetsAllModesPaginatedWithNonResolvingSizeMinusOneFromDist: expected ${needed} entries for ${n}×${n} adjacency, got ${a32.length}`);
    }
    if (d32.length !== needed) {
        throw new Error(`wasmGraphResolvingSubsetsAllModesPaginatedWithNonResolvingSizeMinusOneFromDist: expected ${needed} entries for ${n}×${n} distances, got ${d32.length}`);
    }
    if (pageSize < 0) {
        pageSize = 0;
    }

    const modeCount = 3;
    const bytesAdj = needed * 4;
    const bytesDist = needed * 4;
    const bytesPageIndex3 = modeCount * 4;
    const bytesMinDim3 = modeCount * 4;
    const bytesSmallest3 = modeCount * n * 4;
    const bytesTotalCount3 = modeCount * 4;
    const bytesPageCount3 = modeCount * 4;
    const bytesPageListCount3 = modeCount * 4;
    const bytesPageUsed3 = modeCount * 4;
    const bytesPageTrunc3 = modeCount * 4;
    const pageListFlatMaxIntsPerMode = Math.max(1, pageSize * (n + 1));
    const bytesPageFlat3 = modeCount * pageListFlatMaxIntsPerMode * 4;

    const adjPtr = m._malloc(bytesAdj);
    const distPtr = m._malloc(bytesDist);
    const pageIndex3Ptr = m._malloc(bytesPageIndex3);
    const minDim3Ptr = m._malloc(bytesMinDim3);
    const smallest3Ptr = m._malloc(bytesSmallest3);
    const totalCount3Ptr = m._malloc(bytesTotalCount3);
    const pageCount3Ptr = m._malloc(bytesPageCount3);
    const pageListCount3Ptr = m._malloc(bytesPageListCount3);
    const pageUsed3Ptr = m._malloc(bytesPageUsed3);
    const pageTrunc3Ptr = m._malloc(bytesPageTrunc3);
    const pageFlat3Ptr = m._malloc(bytesPageFlat3);

    if (adjPtr === 0 || distPtr === 0 || pageIndex3Ptr === 0 || minDim3Ptr === 0 ||
        smallest3Ptr === 0 || totalCount3Ptr === 0 || pageCount3Ptr === 0 ||
        pageListCount3Ptr === 0 || pageUsed3Ptr === 0 || pageTrunc3Ptr === 0 ||
        pageFlat3Ptr === 0) {
        if (adjPtr) {
            m._free(adjPtr);
        }
        if (distPtr) {
            m._free(distPtr);
        }
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
        throw new Error("wasmGraphResolvingSubsetsAllModesPaginatedWithNonResolvingSizeMinusOneFromDist: malloc failed");
    }

    try {
        copyToHeap(HEAP32, a32, adjPtr / 4);
        copyToHeap(HEAP32, d32, distPtr / 4);
        const pageIndexBase = pageIndex3Ptr / 4;
        HEAP32[pageIndexBase] = Math.max(0, pageIndexByMode[0] | 0);
        HEAP32[pageIndexBase + 1] = Math.max(0, pageIndexByMode[1] | 0);
        HEAP32[pageIndexBase + 2] = Math.max(0, pageIndexByMode[2] | 0);

        const ok = m._wasm_graph_resolving_subsets_all_modes_paginated_with_non_resolving_size_minus_one_from_dist(
            n,
            adjPtr,
            distPtr,
            pageSize,
            pageIndex3Ptr,
            minDim3Ptr,
            smallest3Ptr,
            n,
            totalCount3Ptr,
            pageCount3Ptr,
            pageListCount3Ptr,
            pageUsed3Ptr,
            pageFlat3Ptr,
            pageListFlatMaxIntsPerMode,
            pageTrunc3Ptr,
        );
        if (ok === 0) {
            throw new Error("wasmGraphResolvingSubsetsAllModesPaginatedWithNonResolvingSizeMinusOneFromDist: wasm call failed");
        }

        const parseMode = (modeIdx: number): {
            minDimension: number;
            smallestBasis: number[];
            totalCount: number;
            pageCount: number;
            subsets: number[][];
            truncated: boolean;
        } => {
            const minDim = HEAP32[minDim3Ptr / 4 + modeIdx];
            const smallestBasis: number[] = [];
            const smallestBase = smallest3Ptr / 4 + modeIdx * n;
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
            return { minDimension: minDim, smallestBasis, totalCount, pageCount, subsets, truncated };
        };

        return {
            node: parseMode(0),
            edge: parseMode(1),
            mixed: parseMode(2),
        };
    } finally {
        m._free(adjPtr);
        m._free(distPtr);
        m._free(pageIndex3Ptr);
        m._free(minDim3Ptr);
        m._free(smallest3Ptr);
        m._free(totalCount3Ptr);
        m._free(pageCount3Ptr);
        m._free(pageListCount3Ptr);
        m._free(pageUsed3Ptr);
        m._free(pageTrunc3Ptr);
        m._free(pageFlat3Ptr);
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
                    const minSizeTruncated = HEAP32[minTrunc3Ptr / 4 + modeIdx] !== 0;
                    const minSizeSubsets: number[][] = [];
                    const minFlatBase = minFlat3Ptr / 4 + modeIdx * minListFlatMaxIntsPerMode;
                    let minIdx = 0;
                    while (minIdx < minUsedInts) {
                        const k = HEAP32[minFlatBase + minIdx];
                        minIdx++;
                        const subset: number[] = [];
                        for (let i = 0; i < k; i++) {
                            subset.push(HEAP32[minFlatBase + minIdx]);
                            minIdx++;
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
                        minSizeTruncated,
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
            m._wasm_graph_resolving_subsets_cache_destroy(handle);
            released = true;
            graphN = 0;
        },
    };
}
