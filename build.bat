@echo off
echo Building Python++ compiler...

REM Create build directory
if not exist build mkdir build
cd build

REM Configure with CMake
echo Configuring with CMake...
cmake .. -DCMAKE_BUILD_TYPE=Release

REM Build
echo Building...
cmake --build . --config Release

REM Run tests
echo Running tests...
ctest --config Release

echo Build completed successfully!
echo Compiler executable: build\Release\py++c.exe
echo Runner executable: build\Release\p++.exe
echo.
echo Usage examples (Compiler):
echo   py++c.exe examples\fibonacci.py -o fibonacci
echo   py++c.exe examples\basic_operations.py -o basic_ops -v
echo   py++c.exe examples\functions.py -o functions -O3 -S
echo.
echo Usage examples (Runner):
echo   p++.exe examples\hello_world.py+
echo   p++.exe examples\fibonacci.py+

pause