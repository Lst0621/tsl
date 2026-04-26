import wasmSequences from "../wasm_out_v1/wasm_sequences";
import type { WasmSequencesModule } from "../wasm_out_v1/wasm_sequences";
import { loadEmscriptenModule } from "./wasm_loader";

let moduleInstance: WasmSequencesModule | null = null;

export const modulePromise: Promise<WasmSequencesModule> = (async () => {
    const mod = await loadEmscriptenModule<WasmSequencesModule>(
        wasmSequences as any,
        "../wasm_out_v1/wasm_sequences.js",
        "../wasm_out_v1/wasm_sequences.wasm",
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

function allocateMemory(
    HEAP32: Int32Array,
    arr32: Int32Array,
    seq32: Int32Array,
    extraSize: number = 0,
): { arrOffsetInInts: number; seqOffsetInInts: number } {
    const arrOffsetInInts = 1024;
    const seqOffsetInInts = arrOffsetInInts + arr32.length;
    if (HEAP32.length < seqOffsetInInts + seq32.length + extraSize) {
        throw new Error("WASM memory exhausted");
    }
    return { arrOffsetInInts, seqOffsetInInts };
}

function build_multi_dimensional_array(flatData: Int32Array, dimensions: Int32Array): any {
    if (dimensions.length === 0) {
        return null;
    }
    if (dimensions.length === 1) {
        return Array.from(flatData);
    }
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

export function wasm_number_of_sequences_all(
    arr: readonly number[] | Int32Array,
    seq: readonly number[] | Int32Array,
): any {
    const HEAP32 = getHeap32();
    const arr32 = toInt32Array(arr);
    const seq32 = toInt32Array(seq);

    if (arr32.length === 0 || seq32.length === 0) {
        return [];
    }

    let totalSize = 1;
    for (let i = 0; i < seq32.length; i++) {
        totalSize *= (seq32[i] + 1);
    }

    const { arrOffsetInInts, seqOffsetInInts } = allocateMemory(HEAP32, arr32, seq32, totalSize);
    copyToHeap(HEAP32, arr32, arrOffsetInInts);
    copyToHeap(HEAP32, seq32, seqOffsetInInts);

    const arrPtr = arrOffsetInInts * 4;
    const seqPtr = seqOffsetInInts * 4;
    const outputPtr = moduleInstance!._wasm_number_of_sequences_all(arrPtr, arr32.length, seqPtr, seq32.length);

    const outputOffsetFromWasm = outputPtr / 4;
    const flatData = new Int32Array(totalSize);
    for (let i = 0; i < totalSize; i++) {
        flatData[i] = HEAP32[outputOffsetFromWasm + i];
    }

    const dimensions = new Int32Array(seq32);
    return build_multi_dimensional_array(flatData, dimensions);
}

export function wasm_number_of_sequences(
    arr: readonly number[] | Int32Array,
    seq: readonly number[] | Int32Array,
): number {
    const HEAP32 = getHeap32();
    const arr32 = toInt32Array(arr);
    const seq32 = toInt32Array(seq);
    if (arr32.length === 0 || seq32.length === 0) {
        return 0;
    }
    const { arrOffsetInInts, seqOffsetInInts } = allocateMemory(HEAP32, arr32, seq32, 0);
    copyToHeap(HEAP32, arr32, arrOffsetInInts);
    copyToHeap(HEAP32, seq32, seqOffsetInInts);
    return moduleInstance!._wasm_number_of_sequences(
        arrOffsetInInts * 4,
        arr32.length,
        seqOffsetInInts * 4,
        seq32.length,
    );
}

