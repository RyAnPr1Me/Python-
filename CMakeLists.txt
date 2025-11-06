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

# Source files
set(PYPP_CORE_SOURCES
    src/lexer.cpp
    src/parser.cpp
    src/ast.cpp
    src/type_inference.cpp
    src/types.cpp
    src/ir.cpp
    src/codegen.cpp
    src/runtime.cpp
    src/refcount.cpp
    src/stdlib.cpp
    src/advanced_types.cpp
    src/optimizations.cpp
    src/python_stdlib.cpp
)

set(PYPP_AOT_SOURCES
    src/bytecode.cpp
    src/import_system.cpp
    src/c_api.cpp
    src/aot.cpp
    src/jit.cpp
)

set(PYPP_MAIN_SOURCES
    src/main.cpp
)

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

# Main compiler executable
add_executable(py++c ${PYPP_MAIN_SOURCES})
target_link_libraries(py++c py++-core)

if(PYPP_ENABLE_AOT)
    target_link_libraries(py++c py++-aot)
endif()

# Runtime library
add_library(py++-runtime
    src/runtime.cpp
    src/refcount.cpp
    src/c_api.cpp
)
target_include_directories(py++-runtime PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(py++-runtime ${PYTHON3_LIBRARIES})

# Examples
if(PYPP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
if(PYPP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Installation
include(GNUInstallDirs)

install(TARGETS py++-core py++-runtime py++c
    EXPORT PythonPlusPlusTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT PythonPlusPlusTargets
    FILE PythonPlusPlusTargets.cmake
    NAMESPACE PythonPlusPlus::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PythonPlusPlus
)

# Create package configuration
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PythonPlusPlusConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/PythonPlusPlusConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PythonPlusPlus
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/PythonPlusPlusConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/PythonPlusPlusConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/PythonPlusPlusConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/PythonPlusPlus
)

# Custom targets for AOT compilation
if(PYPP_ENABLE_AOT)
    # AOT compilation of Python files
    function(add_aot_target source_file output_name)
        add_custom_command(
            OUTPUT ${output_name}
            COMMAND py++c -a -O3 ${source_file} -o ${output_name}
            DEPENDS ${source_file} py++c
            COMMENT "AOT compiling ${source_file} to ${output_name}"
        )
        add_custom_target(aot_${output_name}
            DEPENDS ${output_name}
            COMMENT "AOT compilation target for ${output_name}"
        )
    endfunction()
    
    # Batch AOT compilation
    function(add_aot_batch target_name)
        add_custom_target(${target_name}
            COMMAND ${CMAKE_COMMAND} -E echo "Starting batch AOT compilation..."
            COMMENT "Batch AOT compilation for ${target_name}"
        )
    endfunction()
endif()

# Documentation generation
find_package(Doxygen QUIET)
if(DOXYGEN_FOUND)
    set(DOXYGEN_INPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
    set(DOXYGEN_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/docs)
    
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in
                   ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    
    add_custom_target(docs
        ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM
    )
endif()

# Packaging
set(CPACK_PACKAGE_NAME "PythonPlusPlus")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AOT-Compiled Python++ Compiler")
set(CPACK_PACKAGE_VENDOR "Python++ Project")
set(CPACK_PACKAGE_CONTACT "ryan.j.manzo@gmail.com")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
else()
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
endif()

include(CPack)

# Configuration summary
message(STATUS "")
message(STATUS "Python++ Configuration Summary:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "")
message(STATUS "Features:")
message(STATUS "  AOT compilation: ${PYPP_ENABLE_AOT}")
message(STATUS "  JIT compilation: ${PYPP_ENABLE_JIT}")
message(STATUS "  Profiling: ${PYPP_ENABLE_PROFILING}")
message(STATUS "  Debug: ${PYPP_ENABLE_DEBUG}")
message(STATUS "  Shared libraries: ${PYPP_BUILD_SHARED_LIBS}")
message(STATUS "")
message(STATUS "Dependencies:")
message(STATUS "  LLVM version: ${LLVM_PACKAGE_VERSION}")
message(STATUS "  Python version: ${PYTHON3_VERSION}")
message(STATUS "")
message(STATUS "Build directories:")
message(STATUS "  Source: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "  Binary: ${CMAKE_CURRENT_BINARY_DIR}")
message(STATUS "  Install: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")