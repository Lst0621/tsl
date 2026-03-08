#!/bin/bash
# Build script for WASM module using Emscripten

cd "$(dirname "$0")" || exit 1

# Configure with explicit Emscripten toolchain
cmake -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_C_COMPILER=/usr/bin/emcc \
  -DCMAKE_CXX_COMPILER=/usr/bin/em++ \
  -S . \
  -B wasm_out_v1

# Build using Emscripten make wrapper
cd wasm_out_v1 || exit 1
emmake make