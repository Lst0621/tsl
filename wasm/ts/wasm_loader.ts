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
        let onRuntimeInitializedResolve: (() => void) | null = null;
        const onRuntimeInitializedPromise = new Promise<void>((resolve) => {
            onRuntimeInitializedResolve = resolve;
        });

        const mod = await browserInit({
            onRuntimeInitialized: () => {
                if (onRuntimeInitializedResolve) {
                    onRuntimeInitializedResolve();
                }
            },
        } as any);

        const maybeReady = (mod as any)?.ready;
        if (maybeReady && typeof maybeReady.then === "function") {
            await maybeReady;
        }

        const heap32 = (mod as any)?.HEAP32;
        if (!heap32) {
            await Promise.race([
                onRuntimeInitializedPromise,
                new Promise<void>((_, reject) => {
                    setTimeout(() => {
                        reject(new Error("Timed out waiting for Emscripten runtime initialization."));
                    }, 5000);
                }),
            ]);
        }

        return mod;
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
    let onRuntimeInitializedResolve: (() => void) | null = null;
    const onRuntimeInitializedPromise = new Promise<void>((resolve) => {
        onRuntimeInitializedResolve = resolve;
    });

    const mod = await nodeInit({
        wasmBinary,
        onRuntimeInitialized: () => {
            if (onRuntimeInitializedResolve) {
                onRuntimeInitializedResolve();
            }
        },
    } as any);

    const maybeReady = (mod as any)?.ready;
    if (maybeReady && typeof maybeReady.then === "function") {
        await maybeReady;
    }

    const heap32 = (mod as any)?.HEAP32;
    if (!heap32) {
        await Promise.race([
            onRuntimeInitializedPromise,
            new Promise<void>((_, reject) => {
                setTimeout(() => {
                    reject(new Error("Timed out waiting for Emscripten runtime initialization."));
                }, 5000);
            }),
        ]);
    }

    return mod;
}

