import wasmBarsGame from "../wasm_out_ci/wasm_bars_game";
import { loadEmscriptenModule } from "./wasm_loader";
import type { WasmBarsGameModule } from "./wasm_types";

let moduleInstance: WasmBarsGameModule | null = null;

export const modulePromise: Promise<WasmBarsGameModule> = (async () => {
    const mod = await loadEmscriptenModule<WasmBarsGameModule>(
        wasmBarsGame as any,
        "../wasm_out_ci/wasm_bars_game.js",
        "../wasm_out_ci/wasm_bars_game.wasm",
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

// --- Bars game (opaque handle) ---
const BARS_GAME_BASE_INTS = 1280;
let barsGameHandle: number | null = null;

export function barsGameCreate(): void {
    if (!moduleInstance) {
        throw new Error("WASM module not initialized.");
    }
    if (barsGameHandle !== null) {
        moduleInstance._bars_game_destroy(barsGameHandle);
    }
    barsGameHandle = moduleInstance._bars_game_create();
}

export function barsGameDestroy(): void {
    if (moduleInstance && barsGameHandle !== null) {
        moduleInstance._bars_game_destroy(barsGameHandle);
        barsGameHandle = null;
    }
}

export function barsGameSetSeed(seed: number): void {
    if (!moduleInstance || barsGameHandle === null) {
        throw new Error("Bars game not created. Call barsGameCreate first.");
    }
    moduleInstance._bars_game_set_seed(barsGameHandle, seed);
}

export function barsGameInit(): void {
    if (!moduleInstance || barsGameHandle === null) {
        throw new Error("Bars game not created. Call barsGameCreate first.");
    }
    moduleInstance._bars_game_init(barsGameHandle);
}

export function barsGameGetState(): number[] {
    if (!moduleInstance || barsGameHandle === null) {
        throw new Error("Bars game not created. Call barsGameCreate first.");
    }
    const HEAP32 = getHeap32();
    const size = moduleInstance._bars_game_state_size(barsGameHandle);
    if (HEAP32.length < BARS_GAME_BASE_INTS + size) {
        throw new Error("WASM memory exhausted");
    }
    moduleInstance._bars_game_get_state(barsGameHandle, BARS_GAME_BASE_INTS * 4);
    const out: number[] = [];
    for (let i = 0; i < size; i++) {
        out.push(HEAP32[BARS_GAME_BASE_INTS + i]);
    }
    return out;
}

export function barsGameGetFutureState(choiceIndex: number): number[] {
    if (!moduleInstance || barsGameHandle === null) {
        throw new Error("Bars game not created. Call barsGameCreate first.");
    }
    const HEAP32 = getHeap32();
    const size = moduleInstance._bars_game_state_size(barsGameHandle);
    if (HEAP32.length < BARS_GAME_BASE_INTS + size) {
        throw new Error("WASM memory exhausted");
    }
    moduleInstance._bars_game_get_future_state(barsGameHandle, choiceIndex, BARS_GAME_BASE_INTS * 4);
    const out: number[] = [];
    for (let i = 0; i < size; i++) {
        out.push(HEAP32[BARS_GAME_BASE_INTS + i]);
    }
    return out;
}

export function barsGameApplyChoice(index: number): void {
    if (!moduleInstance || barsGameHandle === null) {
        throw new Error("Bars game not created. Call barsGameCreate first.");
    }
    moduleInstance._bars_game_apply_choice(barsGameHandle, index);
}

export function barsGameIsEnded(): boolean {
    if (!moduleInstance || barsGameHandle === null) {
        return true;
    }
    return moduleInstance._bars_game_is_ended(barsGameHandle) !== 0;
}

export function barsGameStateSize(): number {
    if (!moduleInstance || barsGameHandle === null) {
        return 0;
    }
    return moduleInstance._bars_game_state_size(barsGameHandle);
}

export function barsGameMaxVal(): number {
    if (!moduleInstance || barsGameHandle === null) {
        return 2000;
    }
    return moduleInstance._bars_game_max_val(barsGameHandle);
}

