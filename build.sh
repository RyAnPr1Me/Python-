#!/bin/bash

# Python++ Build Script for Linux/macOS
# This script builds the complete Python++ AOT compiler system

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default configuration
BUILD_TYPE="Release"
BUILD_DIR="build"
INSTALL_PREFIX="/usr/local"
ENABLE_TESTS="ON"
ENABLE_EXAMPLES="ON"
ENABLE_AOT="ON"
ENABLE_JIT="ON"
ENABLE_DEBUG="OFF"
PARALLEL_JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
show_usage() {
    cat << EOF
Python++ Build Script

Usage: $0 [OPTIONS]

Options:
    -t, --type TYPE         Build type (Debug|Release|RelWithDebInfo) [default: Release]
    -d, --dir DIR           Build directory [default: build]
    -p, --prefix PREFIX     Install prefix [default: /usr/local]
    -j, --jobs N            Number of parallel jobs [default: $PARALLEL_JOBS]
    --enable-tests          Enable building tests [default: ON]
    --disable-tests         Disable building tests
    --enable-examples       Enable building examples [default: ON]
    --disable-examples      Disable building examples
    --enable-aot            Enable AOT compilation [default: ON]
    --disable-aot           Disable AOT compilation
    --enable-jit            Enable JIT compilation [default: ON]
    --disable-jit           Disable JIT compilation
    --enable-debug          Enable debug features [default: OFF]
    --clean                 Clean build directory before building
    --install               Install after building
    --package               Create distribution package
    --help                  Show this help message

Examples:
    $0                                    # Default release build
    $0 -t Debug -j 8                      # Debug build with 8 jobs
    $0 --clean --install                   # Clean, build, and install
    $0 --enable-debug --enable-tests       # Debug build with tests

EOF
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -d|--dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -p|--prefix)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        -j|--jobs)
            PARALLEL_JOBS="$2"
            shift 2
            ;;
        --enable-tests)
            ENABLE_TESTS="ON"
            shift
            ;;
        --disable-tests)
            ENABLE_TESTS="OFF"
            shift
            ;;
        --enable-examples)
            ENABLE_EXAMPLES="ON"
            shift
            ;;
        --disable-examples)
            ENABLE_EXAMPLES="OFF"
            shift
            ;;
        --enable-aot)
            ENABLE_AOT="ON"
            shift
            ;;
        --disable-aot)
            ENABLE_AOT="OFF"
            shift
            ;;
        --enable-jit)
            ENABLE_JIT="ON"
            shift
            ;;
        --disable-jit)
            ENABLE_JIT="OFF"
            shift
            ;;
        --enable-debug)
            ENABLE_DEBUG="ON"
            shift
            ;;
        --clean)
            CLEAN_BUILD="ON"
            shift
            ;;
        --install)
            INSTALL_AFTER_BUILD="ON"
            shift
            ;;
        --package)
            CREATE_PACKAGE="ON"
            shift
            ;;
        --help)
            show_usage
            exit 0
            ;;
        *)
            print_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Validate build type
case $BUILD_TYPE in
    Debug|Release|RelWithDebInfo|MinSizeRel)
        ;;
    *)
        print_error "Invalid build type: $BUILD_TYPE"
        exit 1
        ;;
esac

# Print configuration
print_status "Python++ Build Configuration"
echo "  Build Type: $BUILD_TYPE"
echo "  Build Directory: $BUILD_DIR"
echo "  Install Prefix: $INSTALL_PREFIX"
echo "  Parallel Jobs: $PARALLEL_JOBS"
echo "  Enable Tests: $ENABLE_TESTS"
echo "  Enable Examples: $ENABLE_EXAMPLES"
echo "  Enable AOT: $ENABLE_AOT"
echo "  Enable JIT: $ENABLE_JIT"
echo "  Enable Debug: $ENABLE_DEBUG"
echo ""

# Check dependencies
print_status "Checking dependencies..."

# Check for CMake
if ! command -v cmake &> /dev/null; then
    print_error "CMake is required but not installed"
    exit 1
fi

# Check for LLVM
if ! pkg-config --exists llvm; then
    print_warning "LLVM not found via pkg-config, trying cmake find_package..."
fi

# Check for Python development headers
if ! pkg-config --exists python3; then
    print_error "Python 3 development headers not found"
    exit 1
fi

# Get Python version
PYTHON_VERSION=$(pkg-config --modversion python3)
print_status "Found Python version: $PYTHON_VERSION"

# Check if Python version is >= 3.10
if ! pkg-config --atleast-version=3.10 python3; then
    print_error "Python 3.10 or higher is required, found $PYTHON_VERSION"
    exit 1
fi

print_success "All dependencies satisfied"

# Clean build directory if requested
if [[ "$CLEAN_BUILD" == "ON" ]]; then
    print_status "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
fi

# Create build directory
print_status "Creating build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
print_status "Configuring with CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
    -DPYPP_BUILD_TESTS="$ENABLE_TESTS" \
    -DPYPP_BUILD_EXAMPLES="$ENABLE_EXAMPLES" \
    -DPYPP_ENABLE_AOT="$ENABLE_AOT" \
    -DPYPP_ENABLE_JIT="$ENABLE_JIT" \
    -DPYPP_ENABLE_DEBUG="$ENABLE_DEBUG" \
    -DPYPP_BUILD_SHARED_LIBS=ON

if [[ $? -ne 0 ]]; then
    print_error "CMake configuration failed"
    exit 1
fi

# Build
print_status "Building with $PARALLEL_JOBS parallel jobs..."
make -j"$PARALLEL_JOBS"

if [[ $? -ne 0 ]]; then
    print_error "Build failed"
    exit 1
fi

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
