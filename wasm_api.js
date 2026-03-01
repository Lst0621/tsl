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
 * High-level wrapper: accepts plain arrays or Int32Array.
 * Allocates space in WASM memory, copies data, calls the low-level function.
 */
export function wasmNumberOfSequences(arr, seq) {
    if (!moduleInstance) {
        throw new Error("WASM module not initialized. Call and await initWasm() before using wasmNumberOfSequences().");
    }
    // Convert to Int32Array if needed
    const arr32 = arr instanceof Int32Array ? arr : new Int32Array(arr);
    const seq32 = seq instanceof Int32Array ? seq : new Int32Array(seq);
    if (arr32.length === 0 || seq32.length === 0) {
        return 0;
    }
    // Get the Emscripten module's HEAP32 view (this is exposed by Emscripten)
    const m = moduleInstance;
    const HEAP32 = m.HEAP32;
    if (!HEAP32) {
        throw new Error("Cannot access WASM module HEAP32");
    }
    // Calculate memory allocation (use a safe offset in the heap)
    const arrBytes = arr32.length * 4;
    const seqBytes = seq32.length * 4;
    // Emscripten allocates memory starting from lower addresses
    // Use a safe offset: allocate at multiples of heap size for safety
    // For small data, use a fixed safe location (after common static data)
    const arrOffsetInInts = 1024; // Safe offset in int32s
    const seqOffsetInInts = arrOffsetInInts + arr32.length;
    // Verify we're within bounds
    if (HEAP32.length < seqOffsetInInts + seq32.length) {
        throw new Error("WASM memory exhausted");
    }
    // Copy data directly into HEAP32
    for (let i = 0; i < arr32.length; i++) {
        HEAP32[arrOffsetInInts + i] = arr32[i];
    }
    for (let i = 0; i < seq32.length; i++) {
        HEAP32[seqOffsetInInts + i] = seq32[i];
    }
    // Convert offsets from int32 indices to byte offsets
    const arrPtr = arrOffsetInInts * 4;
    const seqPtr = seqOffsetInInts * 4;
    // Call the WASM function with byte pointers
    return moduleInstance._wasm_number_of_sequences(arrPtr, arr32.length, seqPtr, seq32.length);
}
