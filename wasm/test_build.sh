#!/bin/bash
# Native C++ test build script for Polynomial tests using Google Test
# This is for testing the polynomial library natively (not WASM)
#
# RULE: Always use build.sh for WASM builds.
#       Use this script (test_build.sh) for native C++ tests with Google Test.

cd "$(dirname "$0")" || exit 1

# Create native build directory (separate from WASM build)
mkdir -p native_build

# Configure with native C++ compiler (not Emscripten)
# This allows Google Test to work properly
cmake -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -S . \
  -B native_build

# Build the native test executable with parallel jobs
cd native_build || exit 1
make -j

# Run the tests
echo ""
echo "=========================================="
echo "Running Polynomial Tests with Google Test"
echo "=========================================="
echo ""

./polynomial_test_gtest

exit_code=$?

echo ""
echo "=========================================="
if [ $exit_code -eq 0 ]; then
    echo "✓ All tests passed!"
else
    echo "✗ Some tests failed (exit code: $exit_code)"
fi
echo "=========================================="
echo ""

exit $exit_code
