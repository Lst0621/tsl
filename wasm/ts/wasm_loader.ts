export function isNodeRuntime(): boolean {
    const g = typeof globalThis !== "undefined" ? globalThis : undefined;
    const proc = g && (g as { process?: { versions?: { node?: string } } }).process;
    return typeof proc?.versions?.node === "string";
}

/**
 * Load an Emscripten MODULARIZE=1 module in both Node and browser.
 *
 * - Browser: call `browserInit()`; Emscripten fetches `<name>.wasm` relative to `<name>.js`.
 * - Node: read the `.wasm` bytes and pass `{ wasmBinary }` to the loader.
 */
export async function loadEmscriptenModule<T>(
    browserInit: (moduleOverrides?: { wasmBinary?: Buffer | Uint8Array }) => Promise<T>,
    nodeLoaderPath: string,
    wasmRelativePathFromThisFile: string,
): Promise<T> {
    if (!isNodeRuntime()) {
        return browserInit();
    }

    const nodeFs = "node:fs";
    const nodeUrl = "node:url";
    const nodePath = "node:path";
    const fs = (await import(nodeFs)) as { readFileSync: (p: string) => Uint8Array };
    const url = (await import(nodeUrl)) as { fileURLToPath: (u: URL | string) => string };
    const path = (await import(nodePath)) as { dirname: (p: string) => string; join: (...p: string[]) => string };

    const { default: nodeInit } = (await import(nodeLoaderPath)) as {
        default: typeof browserInit;
    };

    const __filename = url.fileURLToPath(import.meta.url);
    const __dirname = path.dirname(__filename);
    const wasmPath = path.join(__dirname, wasmRelativePathFromThisFile);
    const wasmBinary = fs.readFileSync(wasmPath);
    return nodeInit({ wasmBinary });
}

