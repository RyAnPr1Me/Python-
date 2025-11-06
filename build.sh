#!/bin/bash

# Build script for Python++ compiler

set -e

echo "Building Python++ compiler..."

# Create build directory
mkdir -p build
cd build

# Configure with CMake
echo "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo "Building..."
make -j$(nproc)

# Run tests
echo "Running tests..."
make test

echo "Build completed successfully!"
echo "Compiler executable: build/py++c"
echo "Runner executable: build/p++"
echo ""
echo "Usage examples (Compiler):"
echo "  ./py++c examples/fibonacci.py -o fibonacci"
echo "  ./py++c examples/basic_operations.py -o basic_ops -v"
echo "  ./py++c examples/functions.py -o functions -O3 -S"
echo ""
echo "Usage examples (Runner):"
echo "  ./p++ examples/hello_world.py+"
echo "  ./p++ examples/fibonacci.py+"