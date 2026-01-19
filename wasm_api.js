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
/** Synchronous; requires `initWasm()` to have completed successfully first. */
export function wasmMinus(a, b) {
    if (!moduleInstance) {
        throw new Error("WASM module not initialized. Call and await initWasm() before using wasmMinus().");
    }
    return moduleInstance._wasm_minus(a, b);
}
