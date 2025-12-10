$vcvars = "D:\Visual Studio\VC\Auxiliary\Build\vcvars64.bat"
$venv_python = "$PSScriptRoot\.venv\Scripts\python.exe"
$venv_ninja = "$PSScriptRoot\.venv\Scripts\ninja.exe"
$clang = "D:\LLVM\bin\clang-cl.exe"

Write-Host "Configuring CMake..." -ForegroundColor Green
cmd.exe /c "call `"$vcvars`" && cmake -S . -B build -G Ninja -DCMAKE_MAKE_PROGRAM=`"$venv_ninja`" -DCMAKE_C_COMPILER=`"$clang`" -DCMAKE_CXX_COMPILER=`"$clang`" -DCMAKE_BUILD_TYPE=Debug -DPython_EXECUTABLE=`"$venv_python`" -DPYBIND11_FINDPYTHON=ON"

if ($LASTEXITCODE -eq 0) {
    Write-Host "Building project..." -ForegroundColor Green
    cmd.exe /c "call `"$vcvars`" && cmake --build build --parallel 4"
} else {
    Write-Host "Configuration failed." -ForegroundColor Red
}
