$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$platformioHome = Join-Path $repoRoot ".platformio-home"
$wingetMingwBin = Join-Path $env:LOCALAPPDATA "Microsoft\WinGet\Packages\BrechtSanders.WinLibs.POSIX.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\mingw64\bin"

# 将 PlatformIO 的包、缓存和临时产物固定到仓库内，避免挤占系统盘。
$env:PLATFORMIO_CORE_DIR = $platformioHome
if (Test-Path $wingetMingwBin) {
    $env:Path = "$wingetMingwBin;$env:Path"
}

& python -m platformio @args
