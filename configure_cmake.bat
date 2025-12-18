@echo off
call "D:\Visual Studio\VC\Auxiliary\Build\vcvars64.bat"

cmake -S . -B build -G Ninja ^
  -DCMAKE_MAKE_PROGRAM="%~dp0.venv\Scripts\ninja.exe" ^
  -DCMAKE_C_COMPILER="D:\LLVM\bin\clang-cl.exe" ^
  -DCMAKE_CXX_COMPILER="D:\LLVM\bin\clang-cl.exe" ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DPython_EXECUTABLE="%~dp0.venv\Scripts\python.exe" ^
  -DPYBIND11_FINDPYTHON=ON ^
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if %ERRORLEVEL% EQU 0 (
    echo.
    echo ========================================
    echo CMake configuration completed successfully!
    echo ========================================
) else (
    echo.
    echo ========================================
    echo CMake configuration FAILED!
    echo ========================================
    exit /b 1
)
