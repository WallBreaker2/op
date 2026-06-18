# Validate the per-wheel build environment used by cibuildwheel.
$ErrorActionPreference = "Stop"

$pythonCommand = Get-Command python
$pythonRoot = Split-Path $pythonCommand.Source -Parent
$pythonBits = python -c "import struct; print(struct.calcsize('P') * 8)"
$identifier = $env:CIBUILDWHEEL_BUILD_IDENTIFIER
$arch = $env:CIBW_ARCHS
$generatorKey = "vs2022"

Write-Host "CIBUILDWHEEL_BUILD_IDENTIFIER=$identifier"
Write-Host "CIBW_ARCHS=$arch"
Write-Host "python=$($pythonCommand.Source)"
Write-Host "python_root=$pythonRoot"
Write-Host "python_bits=$pythonBits"
Write-Host "CMAKE_GENERATOR=$env:CMAKE_GENERATOR"
Write-Host "CMAKE_ARGS=$env:CMAKE_ARGS"
Write-Host "VCPKG_ROOT=$env:VCPKG_ROOT"
Write-Host "BLACKBONE_ROOT=$env:BLACKBONE_ROOT"

$isX86 = ($identifier -like "*-win32") -or ($arch -eq "x86") -or ($env:CMAKE_ARGS -match "(^| )-A Win32( |$)")
$isX64 = ($identifier -like "*-win_amd64") -or ($arch -in @("AMD64", "x64")) -or ($env:CMAKE_ARGS -match "(^| )-A x64( |$)")

if (-not $isX86 -and -not $isX64) {
    $isX86 = ($pythonBits -eq "32")
    $isX64 = ($pythonBits -eq "64")
}

if (-not $isX86 -and -not $isX64) {
    Write-Error "Unsupported cibuildwheel target: identifier=$identifier arch=$arch"
}

if ($isX86 -and $pythonBits -ne "32") {
    Write-Error "Expected 32-bit Python for win32 wheel, got $pythonBits-bit: $($pythonCommand.Source)"
}

if ($isX64 -and $pythonBits -ne "64") {
    Write-Error "Expected 64-bit Python for win_amd64 wheel, got $pythonBits-bit: $($pythonCommand.Source)"
}

if (-not $env:BLACKBONE_ROOT) {
    Write-Error "BLACKBONE_ROOT is not set"
}

if (-not $env:VCPKG_ROOT) {
    Write-Error "VCPKG_ROOT is not set"
}

if ($isX86) {
    $vsArch = "Win32"
    $blackboneCandidates = @(
        (Join-Path $env:BLACKBONE_ROOT "build/$generatorKey-Win32/BlackBone/Release/BlackBone.lib"),
        (Join-Path $env:BLACKBONE_ROOT "build/$generatorKey-Win32/Release/BlackBone.lib"),
        (Join-Path $env:BLACKBONE_ROOT "build/Win32/BlackBone/Release/BlackBone.lib"),
        (Join-Path $env:BLACKBONE_ROOT "build/Win32/Release/BlackBone.lib"),
        (Join-Path $env:BLACKBONE_ROOT "build/x86/Release/BlackBone.lib")
    )
} else {
    $vsArch = "x64"
    $blackboneCandidates = @(
        (Join-Path $env:BLACKBONE_ROOT "build/$generatorKey-x64/BlackBone/Release/BlackBone.lib"),
        (Join-Path $env:BLACKBONE_ROOT "build/$generatorKey-x64/Release/BlackBone.lib"),
        (Join-Path $env:BLACKBONE_ROOT "build/x64/BlackBone/Release/BlackBone.lib"),
        (Join-Path $env:BLACKBONE_ROOT "build/x64/Release/BlackBone.lib"),
        (Join-Path $env:BLACKBONE_ROOT "build/X64/Release/BlackBone.lib")
    )
}

$blackboneLib = $null
foreach ($candidate in $blackboneCandidates) {
    if (Test-Path $candidate) {
        $blackboneLib = (Resolve-Path $candidate).Path
        break
    }
}

if (-not $blackboneLib) {
    Write-Host "=== Available BlackBone libs ==="
    Get-ChildItem (Join-Path $env:BLACKBONE_ROOT "build") -Recurse -Filter BlackBone.lib -ErrorAction SilentlyContinue | ForEach-Object {
        Write-Host $_.FullName
    }
    Write-Error "BlackBone import library not found for $identifier"
}

$opencvRoot = Join-Path (Join-Path (Join-Path (Join-Path "build" "_deps") "opencv") "install") "$generatorKey-$vsArch"
if (-not (Test-Path $opencvRoot)) {
    Write-Host "=== Available OpenCV install roots ==="
    Get-ChildItem (Join-Path (Join-Path (Join-Path "build" "_deps") "opencv") "install") -Directory -ErrorAction SilentlyContinue | ForEach-Object {
        Write-Host $_.FullName
    }
    Write-Error "OpenCV install root not found for $identifier`: $opencvRoot"
}

Write-Host "BLACKBONE_LIBRARY=$blackboneLib"
Write-Host "OPENCV_ROOT=$((Resolve-Path $opencvRoot).Path)"
