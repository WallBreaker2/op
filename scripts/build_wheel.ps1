# Build a local pip wheel with bootstrapped native dependencies.
param(
    [ValidateSet("x86", "x64")]
    [string]$Arch = "x64",
    [string]$Generator = "",
    [string]$OutputDir = "wheelhouse"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path $PSScriptRoot -Parent
Set-Location $repoRoot

if ($Generator -eq "") {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $vsMajor = & $vswhere -latest -property catalog_buildVersion 2>$null
        if ($vsMajor -match "^18\.") {
            $Generator = "vs2026"
        } elseif ($vsMajor -match "^17\.") {
            $Generator = "vs2022"
        } else {
            $Generator = "vs2022"
        }
    } else {
        $Generator = "vs2022"
    }
}

Write-Host "Bootstrapping native dependencies (generator=$Generator, arch=$Arch)..."
python build.py -g $Generator -t Release -a $Arch --deps-arch $Arch

$stateFile = Join-Path (Join-Path "build" "_deps") ".deps-bootstrap-state.json"
if (-not (Test-Path $stateFile)) {
    Write-Error "Bootstrap state not found: $stateFile"
}

$state = Get-Content $stateFile -Raw | ConvertFrom-Json
if ($state.vcpkg_root) {
    $env:VCPKG_ROOT = $state.vcpkg_root
}

$env:BLACKBONE_ROOT = (Resolve-Path (Join-Path (Join-Path "build" "_deps") "BlackBone")).Path
$blackboneInclude = Join-Path $env:BLACKBONE_ROOT "src"

if ($Arch -eq "x86") {
    $cmakePlatform = "Win32"
    $vsArch = "Win32"
    $env:PYTHON32_ROOT = Split-Path (Get-Command python).Source -Parent
    $blackboneCandidates = @(
        (Join-Path $env:BLACKBONE_ROOT "build/$Generator-Win32/BlackBone/Release/BlackBone.lib"),
        (Join-Path $env:BLACKBONE_ROOT "build/Win32/BlackBone/Release/BlackBone.lib"),
        (Join-Path $env:BLACKBONE_ROOT "build/Win32/Release/BlackBone.lib"),
        (Join-Path $env:BLACKBONE_ROOT "build/x86/Release/BlackBone.lib")
    )
} else {
    $cmakePlatform = "x64"
    $vsArch = "x64"
    $env:PYTHON64_ROOT = Split-Path (Get-Command python).Source -Parent
    $blackboneCandidates = @(
        (Join-Path $env:BLACKBONE_ROOT "build/$Generator-x64/BlackBone/Release/BlackBone.lib"),
        (Join-Path $env:BLACKBONE_ROOT "build/x64/BlackBone/Release/BlackBone.lib"),
        (Join-Path $env:BLACKBONE_ROOT "build/X64/Release/BlackBone.lib"),
        (Join-Path $env:BLACKBONE_ROOT "build/x64/Release/BlackBone.lib")
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
    Write-Error "BlackBone import library not found under $($env:BLACKBONE_ROOT)/build"
}

$opencvRoot = Join-Path (Join-Path (Join-Path (Join-Path "build" "_deps") "opencv") "install") "$Generator-$vsArch"
$opencvArgs = @()
if (Test-Path $opencvRoot) {
    $opencvResolved = (Resolve-Path $opencvRoot).Path
    $env:OPENCV_ROOT = $opencvResolved
    $opencvArgs = @(
        "-DOPENCV_ROOT=$($opencvResolved -replace '\\', '/')",
        "-DOPENCV_LIB_SUFFIX=500"
    )
}

$cmakeArgs = @(
    "-DCMAKE_GENERATOR_PLATFORM=$cmakePlatform",
    "-DBLACKBONE_ROOT=$($env:BLACKBONE_ROOT -replace '\\', '/')",
    "-DBLACKBONE_INCLUDE_DIR=$($blackboneInclude -replace '\\', '/')",
    "-DBLACKBONE_LIBRARY=$($blackboneLib -replace '\\', '/')",
    "-DOP_PYTHON_WHEEL=ON",
    "-DOP_BUILD_TESTS=OFF",
    "-Dbuild_swig_py=ON"
) + $opencvArgs

$env:CMAKE_ARGS = ($cmakeArgs -join " ")
Write-Host "CMAKE_ARGS=$env:CMAKE_ARGS"

New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null
pip wheel . --no-deps -w $OutputDir
