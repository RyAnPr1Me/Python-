cmake_minimum_required(VERSION 3.16)

# Find Google Test
find_package(GTest REQUIRED)

# Create test executable
add_executable(run_tests
    test_compiler.cpp
    ../src/lexer.cpp
    ../src/parser.cpp
    ../src/ast.cpp
    ../src/types.cpp
    ../src/type_inference.cpp
    ../src/ir.cpp
    ../src/runtime.cpp
    ../src/stdlib.cpp
)

# Include directories
target_include_directories(run_tests PRIVATE 
    ../include
    ${GTEST_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(run_tests
    ${GTEST_LIBRARIES}
    pthread
)

# Enable testing
enable_testing()

# Add test
add_test(NAME CompilerTests COMMAND run_tests)