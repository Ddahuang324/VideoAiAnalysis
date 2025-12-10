$env:PYTHONPATH = "$PSScriptRoot"
$python = "$PSScriptRoot\.venv\Scripts\python.exe"
$script = "$PSScriptRoot\python\main.py"

Write-Host "Starting AI Video Analysis System..." -ForegroundColor Green
& $python $script
