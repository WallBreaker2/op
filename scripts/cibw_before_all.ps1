# Bootstrap native dependencies once before building all wheels.
$ErrorActionPreference = "Stop"

Write-Host "Bootstrapping native dependencies for wheel builds..."
python build.py -g vs2026 -t Release -a x64 --deps-arch both

$stateFile = Join-Path "build" "_deps" ".deps-bootstrap-state.json"
if (-not (Test-Path $stateFile)) {
    Write-Error "Bootstrap state not found: $stateFile"
}

$state = Get-Content $stateFile -Raw | ConvertFrom-Json
if ($state.vcpkg_root) {
    "VCPKG_ROOT=$($state.vcpkg_root)" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
    Write-Host "VCPKG_ROOT=$($state.vcpkg_root)"
}

$blackboneRoot = (Resolve-Path (Join-Path "build" "_deps" "BlackBone")).Path
"BLACKBONE_ROOT=$blackboneRoot" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
Write-Host "BLACKBONE_ROOT=$blackboneRoot"
