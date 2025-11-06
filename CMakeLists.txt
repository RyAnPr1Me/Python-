cmake_minimum_required(VERSION 3.16)
project(PythonPlusPlus VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Include LLVM headers
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# LLVM libraries
llvm_map_components_to_libnames(llvm_libs
    core
    executionengine
    interpreter
    mc
    support
    nativecodegen
    object
)

# Source files (exclude runner.cpp)
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.c"
)
list(FILTER SOURCES EXCLUDE REGEX ".*runner\\.cpp$")

# Header files
file(GLOB_RECURSE HEADERS
    "include/*.h"
    "include/*.hpp"
)

# Compiler executable
add_executable(py++c ${SOURCES} ${HEADERS})

# Runner executable (p++)
add_executable(p++ src/runner.cpp)

# Include directories
target_include_directories(py++c PRIVATE include)

# Link LLVM libraries
target_link_libraries(py++c ${llvm_libs})

# Runtime library (conditionally built if runtime directory exists)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/runtime")
    add_library(py++-runtime STATIC
        runtime/object.cpp
        runtime/memory.cpp
        runtime/exceptions.cpp
        runtime/builtins.cpp
    )
    target_include_directories(py++-runtime PUBLIC include)
    install(TARGETS py++-runtime DESTINATION lib)
endif()

# Tests (optional - only if GTest is available and BUILD_TESTING is ON)
if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    find_package(GTest QUIET)
    if(GTest_FOUND)
        enable_testing()
        add_subdirectory(tests)
    else()
        message(STATUS "GTest not found, skipping tests")
    endif()
endif()

# Installation
install(TARGETS py++c p++ DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)