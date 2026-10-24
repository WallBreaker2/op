# Cross-language smoke test: call op_c_api from a managed (.NET) program.
#
# Why a separate script instead of wiring the build into CMake directly:
#   the dotnet SDK is not a build requirement of this repository, so it must not
#   be able to break `python build.py`. When dotnet (or the plugin artifacts) is
#   missing the script exits 77, which ctest maps to SKIP via SKIP_RETURN_CODE,
#   keeping the suite usable on machines without the SDK.
#
# Usage: powershell -NoProfile -ExecutionPolicy Bypass -File scripts/run_csharp_smoke.ps1 -Arch x64

param(
    [ValidateSet("x64", "x86")]
    [string]$Arch = "x64",
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"

# ctest 把 77 当作 SKIP（见 tests/CMakeLists.txt 的 SKIP_RETURN_CODE），
# 这样没装 dotnet 的机器上不会把「没跑」算成「失败」。
$SkipExitCode = 77

$root = Split-Path -Parent $PSScriptRoot
$project = Join-Path $root "tests/csharp_smoke/CSharpSmoke.csproj"
$outDir = Join-Path $root "build/csharp_smoke/$Arch"
$binDir = Join-Path $root "bin/$Arch"

if (-not (Get-Command dotnet -ErrorAction SilentlyContinue)) {
    Write-Host "SKIP: dotnet SDK not found; cross-language smoke test not run."
    exit $SkipExitCode
}
if (-not (Test-Path $project)) {
    Write-Host "SKIP: $project not found."
    exit $SkipExitCode
}

$platform = if ($Arch -eq "x86") { "x86" } else { "x64" }

Write-Host "Building $project ($platform / $Configuration) ..."
& dotnet build $project -c $Configuration -p:Platform=$platform -o $outDir --nologo
if ($LASTEXITCODE -ne 0) {
    Write-Host "FAIL: dotnet build returned $LASTEXITCODE"
    exit 1
}

# The C API entry point loads the plugin DLL from its own directory, so both
# DLLs have to sit next to the smoke executable.
#
# 构建目录优先、安装目录兜底：bin/<arch> 只有跑过 install 才会更新，
# 只认它会出现「明明改了、跑起来还是旧行为」的假象
# （tests/stage_x64_artifacts.cmake 里同样的取舍，理由一样）。
$searchDirs = @(
    (Join-Path $root "build/vs2022-$Arch-Release/libop/Release"),
    (Join-Path $root "build/vs2022-$Arch-Release/tests/Release"),
    $binDir
)

$missing = @()
foreach ($name in @("op_c_api_$Arch.dll", "op_$Arch.dll")) {
    $source = $null
    foreach ($dir in $searchDirs) {
        $candidate = Join-Path $dir $name
        if (Test-Path $candidate) {
            $source = $candidate
            break
        }
    }
    if ($null -eq $source) {
        $missing += $name
    } else {
        Write-Host "  using $source"
        Copy-Item $source $outDir -Force
    }
}
if ($missing.Count -gt 0) {
    Write-Host "FAIL: missing plugin DLL(s): $($missing -join ', ')"
    exit 1
}

$exe = Join-Path $outDir "CSharpSmoke.exe"
Write-Host "Running $exe"
& $exe
exit $LASTEXITCODE
