# 设置 PYTHONPATH 包含项目根目录和 Python 绑定目录
$env:PYTHONPATH = "$PSScriptRoot;$PSScriptRoot\build\python"

# 将 FFmpeg DLL 目录和 C++ 输出目录添加到 PATH
$env:PATH = "$PSScriptRoot\build\bin;$PSScriptRoot\build\_deps\ffmpeg_prebuilt-src\bin;$env:PATH"

$python = "$PSScriptRoot\.venv\Scripts\python.exe"
$script = "$PSScriptRoot\python\main.py"

Write-Host "Starting AI Video Analysis System..." -ForegroundColor Green
& $python $script
