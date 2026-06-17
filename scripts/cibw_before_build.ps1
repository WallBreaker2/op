# Configure Python roots and CMake args for each cibuildwheel matrix entry.
$ErrorActionPreference = "Stop"

$pythonRoot = Split-Path (Get-Command python).Source -Parent
$arch = $env:CIBW_ARCHS
$generatorKey = "vs2026"

Write-Host "cibuildwheel arch: $arch"
Write-Host "Python root: $pythonRoot"

if ($arch -eq "x86") {
    $env:PYTHON32_ROOT = $pythonRoot
    $cmakePlatform = "Win32"
    $depArch = "x86"
    $vsArch = "Win32"
    $blackboneLib = Join-Path $env:BLACKBONE_ROOT "build/Win32/Release/BlackBone.lib"
} else {
    $env:PYTHON64_ROOT = $pythonRoot
    $cmakePlatform = "x64"
    $depArch = "x64"
    $vsArch = "x64"
    $blackboneLib = Join-Path $env:BLACKBONE_ROOT "build/X64/Release/BlackBone.lib"
}

if (-not (Test-Path $blackboneLib)) {
    $blackboneLib = Join-Path $env:BLACKBONE_ROOT "build/$vsArch/Release/BlackBone.lib"
}

if (-not (Test-Path $blackboneLib)) {
    Write-Error "BlackBone import library not found for $arch at $blackboneLib"
}

$opencvRoot = Join-Path "build" "_deps" "opencv" "install" "$generatorKey-$vsArch"
$opencvArgs = @()
if (Test-Path $opencvRoot) {
    $opencvResolved = (Resolve-Path $opencvRoot).Path
    $env:OPENCV_ROOT = $opencvResolved
    $opencvArgs = @(
        "-DOPENCV_ROOT=$opencvResolved",
        "-DOPENCV_LIB_SUFFIX=500"
    )
    Write-Host "OPENCV_ROOT=$opencvResolved"
}

$blackboneInclude = Join-Path $env:BLACKBONE_ROOT "src"
$cmakeArgs = @(
    "-DCMAKE_GENERATOR_PLATFORM=$cmakePlatform",
    "-DBLACKBONE_ROOT=$env:BLACKBONE_ROOT",
    "-DBLACKBONE_INCLUDE_DIR=$blackboneInclude",
    "-DBLACKBONE_LIBRARY=$blackboneLib",
    "-DOP_PYTHON_WHEEL=ON",
    "-DOP_BUILD_TESTS=OFF",
    "-Dbuild_swig_py=ON"
) + $opencvArgs

$env:CMAKE_ARGS = ($cmakeArgs -join " ")
Write-Host "CMAKE_ARGS=$env:CMAKE_ARGS"
