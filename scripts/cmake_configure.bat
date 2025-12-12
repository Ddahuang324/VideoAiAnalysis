@echo off
REM 获取脚本所在目录的父目录（项目根目录）
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."

call "D:\Visual Studio\VC\Auxiliary\Build\vcvars64.bat"
cmake -S "%PROJECT_ROOT%" -B "%PROJECT_ROOT%\build" -G Ninja ^
  -DCMAKE_MAKE_PROGRAM="%PROJECT_ROOT%\.venv\Scripts\ninja.exe" ^
  -DCMAKE_C_COMPILER="D:/LLVM/bin/clang-cl.exe" ^
  -DCMAKE_CXX_COMPILER="D:/LLVM/bin/clang-cl.exe" ^
  -DCMAKE_BUILD_TYPE=Debug ^
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ^
  -DPython_EXECUTABLE="%PROJECT_ROOT%\.venv\Scripts\python.exe" ^
  -DPYBIND11_FINDPYTHON=ON
