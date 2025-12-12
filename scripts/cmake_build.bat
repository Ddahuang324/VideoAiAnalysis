@echo off
REM 获取项目根目录
set "SCRIPT_DIR=%~dp0"
set "PROJECT_ROOT=%SCRIPT_DIR%.."

call "D:\Visual Studio\VC\Auxiliary\Build\vcvars64.bat"
cmake --build "%PROJECT_ROOT%\build" --config Debug --parallel 4
