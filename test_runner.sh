#!/bin/bash

# Python++ Test Runner
# Comprehensive test suite for AOT compilation system

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Test configuration
TEST_DIR="tests"
BUILD_DIR="build"
TEST_RESULTS_DIR="test_results"
COVERAGE_DIR="coverage"
PARALLEL_TESTS=4

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[PASS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[FAIL]${NC} $1"
}

# Function to show usage
show_usage() {
    cat << EOF
Python++ Test Runner

Usage: $0 [OPTIONS]

Options:
    -d, --dir DIR         Test directory [default: tests]
    -b, --build DIR       Build directory [default: build]
    -j, --parallel N      Number of parallel tests [default: 4]
    --coverage            Enable code coverage
    --benchmark           Run performance benchmarks
    --integration         Run integration tests only
    --unit                Run unit tests only
    --aot                 Run AOT compilation tests only
    --runtime             Run runtime tests only
    --verbose             Verbose output
    --clean               Clean test results before running
    --help                Show this help message

Examples:
    $0                                    # Run all tests
    $0 --coverage --benchmark              # Run tests with coverage and benchmarks
    $0 --integration --verbose             # Run integration tests with verbose output
    $0 --aot --parallel 8                 # Run AOT tests with 8 parallel jobs

EOF
}

# Parse command line arguments
RUN_UNIT_TESTS=true
RUN_INTEGRATION_TESTS=true
RUN_AOT_TESTS=true
RUN_RUNTIME_TESTS=true
ENABLE_COVERAGE=false
ENABLE_BENCHMARK=false
VERBOSE=false
CLEAN_RESULTS=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--dir)
            TEST_DIR="$2"
            shift 2
            ;;
        -b|--build)
            BUILD_DIR="$2"
            shift 2
            ;;
        -j|--parallel)
            PARALLEL_TESTS="$2"
            shift 2
            ;;
        --coverage)
            ENABLE_COVERAGE=true
            shift
            ;;
        --benchmark)
            ENABLE_BENCHMARK=true
            shift
            ;;
        --integration)
            RUN_UNIT_TESTS=false
            RUN_AOT_TESTS=false
            RUN_RUNTIME_TESTS=false
            shift
            ;;
        --unit)
            RUN_INTEGRATION_TESTS=false
            RUN_AOT_TESTS=false
            RUN_RUNTIME_TESTS=false
            shift
            ;;
        --aot)
            RUN_UNIT_TESTS=false
            RUN_INTEGRATION_TESTS=false
            RUN_RUNTIME_TESTS=false
            shift
            ;;
        --runtime)
            RUN_UNIT_TESTS=false
            RUN_INTEGRATION_TESTS=false
            RUN_AOT_TESTS=false
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --clean)
            CLEAN_RESULTS=true
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

# Print configuration
print_status "Python++ Test Runner Configuration"
echo "  Test Directory: $TEST_DIR"
echo "  Build Directory: $BUILD_DIR"
echo "  Parallel Tests: $PARALLEL_TESTS"
echo "  Coverage: $ENABLE_COVERAGE"
echo "  Benchmark: $ENABLE_BENCHMARK"
echo "  Verbose: $VERBOSE"
echo ""

# Check dependencies
print_status "Checking test dependencies..."

# Check for build directory
if [[ ! -d "$BUILD_DIR" ]]; then
    print_error "Build directory not found: $BUILD_DIR"
    print_status "Please run the build script first"
    exit 1
fi

# Check for test executable
TEST_EXECUTABLE="$BUILD_DIR/tests/test_aot_compilation"
if [[ ! -f "$TEST_EXECUTABLE" ]]; then
    print_error "Test executable not found: $TEST_EXECUTABLE"
    print_status "Please build the tests first"
    exit 1
fi

# Check for required tools
if ! command -v python3 &> /dev/null; then
    print_error "Python 3 is required for some tests"
    exit 1
fi

print_success "All dependencies satisfied"

# Clean test results if requested
if [[ "$CLEAN_RESULTS" == "true" ]]; then
    print_status "Cleaning test results..."
    rm -rf "$TEST_RESULTS_DIR"
    rm -rf "$COVERAGE_DIR"
fi

# Create test results directory
mkdir -p "$TEST_RESULTS_DIR"
mkdir -p "$COVERAGE_DIR"

# Set up environment
export PYTHONPATH="$PYTHONPATH:$(pwd)/examples:$(pwd)/tests"

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# Function to run a test category
run_test_category() {
    local category_name="$1"
    local test_filter="$2"
    local test_executable="$3"
    
    if [[ "$VERBOSE" == "true" ]]; then
        print_status "Running $category_name tests..."
    fi
    
    local test_output="$TEST_RESULTS_DIR/${category_name,,}_results.xml"
    local log_file="$TEST_RESULTS_DIR/${category_name,,}_tests.log"
    
    # Run tests
    local test_args=""
    if [[ "$test_filter" != "" ]]; then
        test_args="--gtest_filter=$test_filter"
    fi
    
    if [[ "$ENABLE_COVERAGE" == "true" ]]; then
        # Run with coverage
        local coverage_file="$COVERAGE_DIR/${category_name,,}_coverage.info"
        llvm-cov gcov "$test_executable" -o "$coverage_file" -- "$test_executable" $test_args 2>&1 | tee "$log_file"
    else
        # Run normally
        "$test_executable" $test_args --gtest_output=xml:"$test_output" 2>&1 | tee "$log_file"
    fi
    
    local exit_code=$?
    
    # Parse results
    if [[ -f "$test_output" ]]; then
        local category_total=$(grep -o 'tests="[0-9]*"' "$test_output" | grep -o '[0-9]*' | head -1)
        local category_failed=$(grep -o 'failures="[0-9]*"' "$test_output" | grep -o '[0-9]*' | head -1)
        local category_disabled=$(grep -o 'disabled="[0-9]*"' "$test_output" | grep -o '[0-9]*' | head -1)
        
        category_total=${category_total:-0}
        category_failed=${category_failed:-0}
        category_disabled=${category_disabled:-0}
        local category_passed=$((category_total - category_failed - category_disabled))
        
        TOTAL_TESTS=$((TOTAL_TESTS + category_total))
        PASSED_TESTS=$((PASSED_TESTS + category_passed))
        FAILED_TESTS=$((FAILED_TESTS + category_failed))
        SKIPPED_TESTS=$((SKIPPED_TESTS + category_disabled))
        
        if [[ "$VERBOSE" == "true" ]]; then
            echo "  Total: $category_total, Passed: $category_passed, Failed: $category_failed, Disabled: $category_disabled"
        fi
        
        if [[ $category_failed -eq 0 ]]; then
            print_success "$category_name tests passed"
        else
            print_error "$category_name tests failed"
        fi
    else
        print_error "Failed to parse $category_name test results"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
}

# Run unit tests
if [[ "$RUN_UNIT_TESTS" == "true" ]]; then
    print_status "Running Unit Tests..."
    run_test_category "Unit" "*Unit*" "$TEST_EXECUTABLE"
fi

# Run integration tests
if [[ "$RUN_INTEGRATION_TESTS" == "true" ]]; then
    print_status "Running Integration Tests..."
    run_test_category "Integration" "*Integration*" "$TEST_EXECUTABLE"
fi

# Run AOT compilation tests
if [[ "$RUN_AOT_TESTS" == "true" ]]; then
    print_status "Running AOT Compilation Tests..."
    run_test_category "AOT" "*AOT*" "$TEST_EXECUTABLE"
fi

# Run runtime tests
if [[ "$RUN_RUNTIME_TESTS" == "true" ]]; then
    print_status "Running Runtime Tests..."
    run_test_category "Runtime" "*Runtime*" "$TEST_EXECUTABLE"
fi

# Run Python example tests
print_status "Running Python Example Tests..."
python_example_tests=0
python_passed=0
python_failed=0

for example in examples/*.py; do
    if [[ -f "$example" ]]; then
        example_name=$(basename "$example" .py)
        python_example_tests=$((python_example_tests + 1))
        
        if [[ "$VERBOSE" == "true" ]]; then
            echo "Testing Python example: $example_name"
        fi
        
        # Test with Python++ compiler
        if "$BUILD_DIR/py++c" -a -O2 "$example" -o "$TEST_RESULTS_DIR/${example_name}_output" 2>/dev/null; then
            python_passed=$((python_passed + 1))
            if [[ "$VERBOSE" == "true" ]]; then
                print_success "  $example_name compiled successfully"
            fi
        else
            python_failed=$((python_failed + 1))
            if [[ "$VERBOSE" == "true" ]]; then
                print_error "  $example_name compilation failed"
            fi
        fi
    fi
done

TOTAL_TESTS=$((TOTAL_TESTS + python_example_tests))
PASSED_TESTS=$((PASSED_TESTS + python_passed))
FAILED_TESTS=$((FAILED_TESTS + python_failed))

# Run performance benchmarks
if [[ "$ENABLE_BENCHMARK" == "true" ]]; then
    print_status "Running Performance Benchmarks..."
    
    benchmark_output="$TEST_RESULTS_DIR/benchmark_results.json"
    "$TEST_EXECUTABLE" --gtest_filter="*Performance*" --benchmark_out="$benchmark_output" --benchmark_out_format=json
    
    if [[ $? -eq 0 ]]; then
        print_success "Performance benchmarks completed"
    else
        print_warning "Some performance benchmarks failed"
    fi
fi

# Generate coverage report
if [[ "$ENABLE_COVERAGE" == "true" ]]; then
    print_status "Generating coverage report..."
    
    # Combine coverage data
    llvm-cov gcov "$TEST_EXECUTABLE" -o "$COVERAGE_DIR/combined.info" -- "$TEST_EXECUTABLE"
    
    # Generate HTML report
    if command -v llvm-cov &> /dev/null; then
        llvm-cov show "$TEST_EXECUTABLE" -format=html -output-dir="$COVERAGE_DIR/html"
        print_success "Coverage report generated: $COVERAGE_DIR/html/index.html"
    fi
fi

# Print summary
echo ""
print_status "Test Results Summary"
echo "  Total Tests: $TOTAL_TESTS"
echo "  Passed: $PASSED_TESTS"
echo "  Failed: $FAILED_TESTS"
echo "  Skipped: $SKIPPED_TESTS"

if [[ $FAILED_TESTS -eq 0 ]]; then
    print_success "All tests passed!"
    exit_code=0
else
    print_error "$FAILED_TESTS test(s) failed"
    exit_code=1
fi

# Print detailed results if verbose
if [[ "$VERBOSE" == "true" ]]; then
    echo ""
    print_status "Detailed Results:"
    echo "  Test results directory: $TEST_RESULTS_DIR"
    if [[ "$ENABLE_COVERAGE" == "true" ]]; then
        echo "  Coverage report: $COVERAGE_DIR/html/index.html"
    fi
    if [[ "$ENABLE_BENCHMARK" == "true" ]]; then
        echo "  Benchmark results: $TEST_RESULTS_DIR/benchmark_results.json"
    fi
fi

exit $exit_code