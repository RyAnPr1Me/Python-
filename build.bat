@echo off
REM Python++ Build Script for Windows
REM This script builds the complete Python++ AOT compiler system

setlocal enabledelayedexpansion

REM Default configuration
set BUILD_TYPE=Release
set BUILD_DIR=build
set INSTALL_PREFIX=C:\PythonPlusPlus
set ENABLE_TESTS=ON
set ENABLE_EXAMPLES=ON
set ENABLE_AOT=ON
set ENABLE_JIT=ON
set ENABLE_DEBUG=OFF
set CLEAN_BUILD=OFF
set INSTALL_AFTER_BUILD=OFF
set CREATE_PACKAGE=OFF

REM Function to print status
echo [INFO] Python++ Build Script for Windows
echo.

REM Parse command line arguments
:parse_args
if "%~1"=="" goto :args_done
if "%~1"=="-t" (
    set BUILD_TYPE=%~2
    shift
    shift
    goto :parse_args
)
if "%~1"=="--type" (
    set BUILD_TYPE=%~2
    shift
    shift
    goto :parse_args
)
if "%~1"=="-d" (
    set BUILD_DIR=%~2
    shift
    shift
    goto :parse_args
)
if "%~1"=="--dir" (
    set BUILD_DIR=%~2
    shift
    shift
    goto :parse_args
)
if "%~1"=="-p" (
    set INSTALL_PREFIX=%~2
    shift
    shift
    goto :parse_args
)
if "%~1"=="--prefix" (
    set INSTALL_PREFIX=%~2
    shift
    shift
    goto :parse_args
)
if "%~1"=="--enable-tests" (
    set ENABLE_TESTS=ON
    shift
    goto :parse_args
)
if "%~1"=="--disable-tests" (
    set ENABLE_TESTS=OFF
    shift
    goto :parse_args
)
if "%~1"=="--enable-examples" (
    set ENABLE_EXAMPLES=ON
    shift
    goto :parse_args
)
if "%~1"=="--disable-examples" (
    set ENABLE_EXAMPLES=OFF
    shift
    goto :parse_args
)
if "%~1"=="--enable-aot" (
    set ENABLE_AOT=ON
    shift
    goto :parse_args
)
if "%~1"=="--disable-aot" (
    set ENABLE_AOT=OFF
    shift
    goto :parse_args
)
if "%~1"=="--enable-jit" (
    set ENABLE_JIT=ON
    shift
    goto :parse_args
)
if "%~1"=="--disable-jit" (
    set ENABLE_JIT=OFF
    shift
    goto :parse_args
)
if "%~1"=="--enable-debug" (
    set ENABLE_DEBUG=ON
    shift
    goto :parse_args
)
if "%~1"=="--clean" (
    set CLEAN_BUILD=ON
    shift
    goto :parse_args
)
if "%~1"=="--install" (
    set INSTALL_AFTER_BUILD=ON
    shift
    goto :parse_args
)
if "%~1"=="--package" (
    set CREATE_PACKAGE=ON
    shift
    goto :parse_args
)
if "%~1"=="--help" (
    goto :show_usage
)
echo [ERROR] Unknown option: %~1
goto :error_exit

:args_done

REM Print configuration
echo [INFO] Python++ Build Configuration
echo   Build Type: %BUILD_TYPE%
echo   Build Directory: %BUILD_DIR%
echo   Install Prefix: %INSTALL_PREFIX%
echo   Enable Tests: %ENABLE_TESTS%
echo   Enable Examples: %ENABLE_EXAMPLES%
echo   Enable AOT: %ENABLE_AOT%
echo   Enable JIT: %ENABLE_JIT%
echo   Enable Debug: %ENABLE_DEBUG%
echo.

REM Check dependencies
echo [INFO] Checking dependencies...

REM Check for CMake
cmake --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] CMake is required but not installed
    goto :error_exit
)

REM Check for Visual Studio Build Tools
where cl >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Visual Studio Build Tools or Visual Studio is required
    echo [INFO] Please run this script from a Developer Command Prompt
    goto :error_exit
)

REM Check for Python
python --version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Python 3.10 or higher is required
    goto :error_exit
)

echo [SUCCESS] All dependencies satisfied

REM Clean build directory if requested
if "%CLEAN_BUILD%"=="ON" (
    echo [INFO] Cleaning build directory...
    if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"
)

REM Create build directory
echo [INFO] Creating build directory...
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd "%BUILD_DIR%"

REM Configure with CMake
echo [INFO] Configuring with CMake...
cmake .. ^
    -G "Visual Studio 16 2019" ^
    -A x64 ^
    -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%" ^
    -DPYPP_BUILD_TESTS=%ENABLE_TESTS% ^
    -DPYPP_BUILD_EXAMPLES=%ENABLE_EXAMPLES% ^
    -DPYPP_ENABLE_AOT=%ENABLE_AOT% ^
    -DPYPP_ENABLE_JIT=%ENABLE_JIT% ^
    -DPYPP_ENABLE_DEBUG=%ENABLE_DEBUG% ^
    -DPYPP_BUILD_SHARED_LIBS=ON

if errorlevel 1 (
    echo [ERROR] CMake configuration failed
    goto :error_exit
)

REM Build
echo [INFO] Building...
cmake --build . --config %BUILD_TYPE% --parallel

if errorlevel 1 (
    echo [ERROR] Build failed
    goto :error_exit
)

echo [SUCCESS] Build completed successfully

REM Run tests if enabled
if "%ENABLE_TESTS%"=="ON" (
    echo [INFO] Running tests...
    ctest --output-on-failure --parallel 4 --config %BUILD_TYPE%
    
    if errorlevel 1 (
        echo [WARNING] Some tests failed
    ) else (
        echo [SUCCESS] All tests passed
    )
)

REM Install if requested
if "%INSTALL_AFTER_BUILD%"=="ON" (
    echo [INFO] Installing...
    cmake --build . --config %BUILD_TYPE% --target install
    
    if errorlevel 1 (
        echo [ERROR] Installation failed
        goto :error_exit
    )
    echo [SUCCESS] Installation completed
)

REM Create package if requested
if "%CREATE_PACKAGE%"=="ON" (
    echo [INFO] Creating distribution package...
    cpack --config CPackConfig.cmake
    
    if errorlevel 1 (
        echo [ERROR] Package creation failed
        goto :error_exit
    )
    echo [SUCCESS] Package created successfully
)

REM Print summary
echo.
echo [INFO] Build Summary
echo   Executable: %BUILD_DIR%\%BUILD_TYPE%\py++c.exe
echo   Libraries: %BUILD_DIR%\%BUILD_TYPE%\
echo   Examples: %BUILD_DIR%\examples\

if "%ENABLE_TESTS%"=="ON" (
    echo   Tests: %BUILD_DIR%\tests\
)

if "%INSTALL_AFTER_BUILD%"=="ON" (
    echo   Installation: %INSTALL_PREFIX%\
)

echo.
echo [SUCCESS] Python++ build completed successfully!
echo [INFO] You can now run the compiler with: %BUILD_DIR%\%BUILD_TYPE%\py++c.exe --help
goto :end

:show_usage
echo Python++ Build Script for Windows
echo.
echo Usage: build.bat [OPTIONS]
echo.
echo Options:
echo     -t, --type TYPE         Build type (Debug^|Release^|RelWithDebInfo) [default: Release]
echo     -d, --dir DIR           Build directory [default: build]
echo     -p, --prefix PREFIX     Install prefix [default: C:\PythonPlusPlus]
echo     --enable-tests          Enable building tests [default: ON]
echo     --disable-tests         Disable building tests
echo     --enable-examples       Enable building examples [default: ON]
echo     --disable-examples      Disable building examples
echo     --enable-aot            Enable AOT compilation [default: ON]
echo     --disable-aot           Disable AOT compilation
echo     --enable-jit            Enable JIT compilation [default: ON]
echo     --disable-jit           Disable JIT compilation
echo     --enable-debug          Enable debug features [default: OFF]
echo     --clean                 Clean build directory before building
echo     --install               Install after building
echo     --package               Create distribution package
echo     --help                  Show this help message
echo.
echo Examples:
echo     build.bat                                    # Default release build
echo     build.bat -t Debug                           # Debug build
echo     build.bat --clean --install                   # Clean, build, and install
echo     build.bat --enable-debug --enable-tests       # Debug build with tests
echo.
goto :end

:error_exit
echo.
echo [ERROR] Build failed!
exit /b 1

:end
endlocal