cmake_minimum_required(VERSION 3.16)
project(PythonPlusPlus VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build options
option(PYPP_BUILD_TESTS "Build tests" ON)
option(PYPP_BUILD_EXAMPLES "Build examples" ON)
option(PYPP_ENABLE_AOT "Enable AOT compilation" ON)
option(PYPP_ENABLE_JIT "Enable JIT compilation" ON)
option(PYPP_ENABLE_PROFILING "Enable profiling support" OFF)
option(PYPP_ENABLE_DEBUG "Enable debug features" OFF)
option(PYPP_BUILD_SHARED_LIBS "Build shared libraries" ON)

# Compiler-specific options
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(PYPP_ENABLE_DEBUG)
        add_compile_options(-g -O0 -DDEBUG)
    else()
        add_compile_options(-O3 -DNDEBUG)
    endif()
endif()

if(MSVC)
    add_compile_options(/W4)
    if(PYPP_ENABLE_DEBUG)
        add_compile_options(/Od /DEBUG)
    else()
        add_compile_options(/O2)
    endif()
endif()

# Find required packages
find_package(LLVM REQUIRED CONFIG)
find_package(PkgConfig REQUIRED)

# Python development libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(PYTHON3 REQUIRED python3>=3.10)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${PYTHON3_INCLUDE_DIRS})

# Link directories
link_directories(${LLVM_LIBRARY_DIR})
link_directories(${PYTHON3_LIBRARY_DIRS})

# Source files (exclude runner.cpp)
file(GLOB_RECURSE SOURCES 
    "src/*.cpp"
    "src/*.c"
)
list(FILTER SOURCES EXCLUDE REGEX ".*runner\\.cpp$")

# Core library
add_library(py++-core ${PYPP_CORE_SOURCES})
target_include_directories(py++-core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(py++-core ${LLVM_LIBRARIES})
target_compile_definitions(py++-core PUBLIC ${LLVM_DEFINITIONS})
target_compile_options(py++-core PUBLIC ${LLVM_COMPILE_OPTIONS})

# AOT library (if enabled)
if(PYPP_ENABLE_AOT)
    add_library(py++-aot ${PYPP_AOT_SOURCES})
    target_link_libraries(py++-aot py++-core ${PYTHON3_LIBRARIES} ${LLVM_LIBRARIES})
    target_include_directories(py++-aot PUBLIC ${PYTHON3_INCLUDE_DIRS})
    
    # Platform-specific linking
    if(UNIX AND NOT APPLE)
        target_link_libraries(py++-aot dl pthread)
    endif()
    
    if(APPLE)
        target_link_libraries(py++-aot dl pthread)
    endif()
endif()

# Runner executable (p++)
add_executable(p++ src/runner.cpp)

# Link Windows libraries for p++ runner (needed for admin checks)
if(WIN32)
    target_link_libraries(p++ advapi32)
endif()

# Include directories
target_include_directories(py++c PRIVATE include)

if(PYPP_ENABLE_AOT)
    target_link_libraries(py++c py++-aot)
endif()

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
