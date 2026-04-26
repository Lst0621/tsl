import wasmLinearRecur from "../wasm_out_v1/wasm_linear_recur";
import type { WasmLinearRecurModule } from "../wasm_out_v1/wasm_linear_recur";
import { loadEmscriptenModule } from "./wasm_loader";

let moduleInstance: WasmLinearRecurModule | null = null;

export const modulePromise: Promise<WasmLinearRecurModule> = (async () => {
    const mod = await loadEmscriptenModule<WasmLinearRecurModule>(
        wasmLinearRecur as any,
        "../wasm_out_v1/wasm_linear_recur.js",
        "../wasm_out_v1/wasm_linear_recur.wasm",
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
    const neg = x < 0;
    let abs = Math.abs(x);
    const loU = abs % U32;
    let hiU = (abs - loU) / U32;

    let lo = (loU >>> 0);
    if (neg) {
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
    throw new Error(`${context}: int64 value outside supported JS safe range.`);
}

function requireIntegerRational(r: RationalI64Parts, context: string): number {
    if (!(r.n.hi === 0 && (r.n.lo >>> 0) === 1)) {
        throw new Error(`${context}: expected integer (denominator 1).`);
    }
    return i64ToSafeNumber(r.m, context);
}

export function lrCreate(coeffs: Array<number | RationalI64Parts>, recursiveThreshold: number = LR_DEFAULT_THRESHOLD): void {
    if (!moduleInstance) {
        throw new Error("WASM module not initialized.");
    }
    if (lrHandle !== null) {
        moduleInstance._lr_destroy(lrHandle);
    }
    const HEAP32 = getHeap32();
    const base = 2048;
    const len = coeffs.length;
    const words = len * 4;
    if (HEAP32.length < base + words) {
        throw new Error("WASM memory exhausted");
    }
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

export function lrEvaluate(initialValues: Array<number | RationalI64Parts>, n: number): number {
    if (!moduleInstance || lrHandle === null) {
        throw new Error("Linear recurrence not created. Call lrCreate first.");
    }
    const HEAP32 = getHeap32();
    const initBase = 2048;
    const resultBase = 4096;
    const len = initialValues.length;
    const initWords = len * 4;
    if (HEAP32.length < resultBase + 4) {
        throw new Error("WASM memory exhausted");
    }
    if (HEAP32.length < initBase + initWords) {
        throw new Error("WASM memory exhausted");
    }
    for (let i = 0; i < len; i++) {
        writeRationalToHeap32(HEAP32, initBase, i, toRationalParts(initialValues[i]));
    }
    moduleInstance._lr_evaluate(lrHandle, initBase * 4, len, n, resultBase * 4);
    const r = readRationalFromHeap32(HEAP32, resultBase, 0);
    return requireIntegerRational(r, "lrEvaluate");
}

export function lrGetCharacteristicPolynomial(): number[] {
    if (!moduleInstance || lrHandle === null) {
        throw new Error("Linear recurrence not created. Call lrCreate first.");
    }
    const order = moduleInstance._lr_order(lrHandle);
    const maxLen = order + 1;
    const HEAP32 = getHeap32();
    const base = 2048;
    const words = maxLen * 4;
    if (HEAP32.length < base + words) {
        throw new Error("WASM memory exhausted");
    }
    const len = moduleInstance._lr_characteristic_polynomial(lrHandle, base * 4, maxLen);
    const out: number[] = [];
    for (let i = 0; i < len; i++) {
        out.push(requireIntegerRational(readRationalFromHeap32(HEAP32, base, i), "lrGetCharacteristicPolynomial"));
    }
    return out;
}

export function lrGetTransitionMatrixSize(): number {
    if (!moduleInstance || lrHandle === null) {
        throw new Error("Linear recurrence not created. Call lrCreate first.");
    }
    return moduleInstance._lr_transition_matrix_size(lrHandle);
}

export function lrGetTransitionMatrixData(): number[] {
    if (!moduleInstance || lrHandle === null) {
        throw new Error("Linear recurrence not created. Call lrCreate first.");
    }
    const n = moduleInstance._lr_transition_matrix_size(lrHandle);
    const size = n * n;
    const HEAP32 = getHeap32();
    const base = 2048;
    const words = size * 4;
    if (HEAP32.length < base + words) {
        throw new Error("WASM memory exhausted");
    }
    moduleInstance._lr_transition_matrix_data(lrHandle, base * 4, size);
    const out: number[] = [];
    for (let i = 0; i < size; i++) {
        out.push(requireIntegerRational(readRationalFromHeap32(HEAP32, base, i), "lrGetTransitionMatrixData"));
    }
    return out;
}

export function wasmMatrixPower(data: number[], n: number, exponent: number): number[] {
    const HEAP32 = getHeap32();
    const size = n * n;
    if (data.length !== size) {
        throw new Error(`Expected ${size} elements for ${n}×${n} matrix.`);
    }
    const inBase = 2048;
    const outBase = 2048 + size;
    if (HEAP32.length < outBase + size) {
        throw new Error("WASM memory exhausted");
    }
    copyToHeap(HEAP32, toInt32Array(data), inBase);
    moduleInstance!._wasm_matrix_power(inBase * 4, n, exponent, outBase * 4);
    const out: number[] = [];
    for (let i = 0; i < size; i++) {
        out.push(HEAP32[outBase + i]);
    }
    return out;
}

export function wasmMatrixTimesConst(data: number[], n: number, scalar: number): number[] {
    const HEAP32 = getHeap32();
    const size = n * n;
    if (data.length !== size) {
        throw new Error(`Expected ${size} elements for ${n}×${n} matrix.`);
    }
    const inBase = 2048;
    const outBase = 2048 + size;
    if (HEAP32.length < outBase + size) {
        throw new Error("WASM memory exhausted");
    }
    copyToHeap(HEAP32, toInt32Array(data), inBase);
    moduleInstance!._wasm_matrix_times_const(inBase * 4, n, scalar, outBase * 4);
    const out: number[] = [];
    for (let i = 0; i < size; i++) {
        out.push(HEAP32[outBase + i]);
    }
    return out;
}

export function wasmMatrixAdd(dataA: number[], dataB: number[], n: number): number[] {
    const HEAP32 = getHeap32();
    const size = n * n;
    if (dataA.length !== size || dataB.length !== size) {
        throw new Error(`Expected ${size} elements for ${n}×${n} matrix.`);
    }
    const baseA = 2048;
    const baseB = 2048 + size;
    const baseOut = 2048 + size * 2;
    if (HEAP32.length < baseOut + size) {
        throw new Error("WASM memory exhausted");
    }
    copyToHeap(HEAP32, toInt32Array(dataA), baseA);
    copyToHeap(HEAP32, toInt32Array(dataB), baseB);
    moduleInstance!._wasm_matrix_add(baseA * 4, baseB * 4, n, baseOut * 4);
    const out: number[] = [];
    for (let i = 0; i < size; i++) {
        out.push(HEAP32[baseOut + i]);
    }
    return out;
}

