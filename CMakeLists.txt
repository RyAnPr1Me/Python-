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

# Source files
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.c"
)

# Header files
file(GLOB_RECURSE HEADERS
    "include/*.h"
    "include/*.hpp"
)

# Compiler executable
add_executable(py++c ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(py++c PRIVATE include)

# Link LLVM libraries
target_link_libraries(py++c ${llvm_libs})

# Runtime library
add_library(py++-runtime STATIC
    runtime/object.cpp
    runtime/memory.cpp
    runtime/exceptions.cpp
    runtime/builtins.cpp
)

target_include_directories(py++-runtime PUBLIC include)

# Tests
enable_testing()
add_subdirectory(tests)

# Installation
install(TARGETS py++c DESTINATION bin)
install(TARGETS py++-runtime DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)