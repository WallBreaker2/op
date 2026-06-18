# Bootstrap native dependencies once before building all wheels.
$ErrorActionPreference = "Stop"

Write-Host "Bootstrapping native dependencies for wheel builds..."
python build.py -g vs2022 -t Release -a x64 --deps-arch both --deps-only

$stateFile = Join-Path (Join-Path "build" "_deps") ".deps-bootstrap-state.json"
if (-not (Test-Path $stateFile)) {
    Write-Error "Bootstrap state not found: $stateFile"
}

$state = Get-Content $stateFile -Raw | ConvertFrom-Json
if ($state.vcpkg_root) {
    "VCPKG_ROOT=$($state.vcpkg_root)" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
    Write-Host "VCPKG_ROOT=$($state.vcpkg_root)"
}

$blackboneRoot = (Resolve-Path (Join-Path (Join-Path "build" "_deps") "BlackBone")).Path
"BLACKBONE_ROOT=$blackboneRoot" | Out-File -FilePath $env:GITHUB_ENV -Append -Encoding utf8
Write-Host "BLACKBONE_ROOT=$blackboneRoot"

Write-Host "=== Bootstrap state ==="
Write-Host "vcpkg_root=$($state.vcpkg_root)"
Write-Host "blackbone_arches=$($state.blackbone_arches -join ',')"
Write-Host "opencv_configured_arches=$($state.opencv_configured_arches -join ',')"
Write-Host "opencv_installed_configs=$($state.opencv_installed_configs -join ',')"

Write-Host "=== BlackBone libs ==="
Get-ChildItem (Join-Path $blackboneRoot "build") -Recurse -Filter BlackBone.lib -ErrorAction SilentlyContinue | ForEach-Object {
    Write-Host $_.FullName
}

$opencvInstallRoot = Join-Path (Join-Path (Join-Path "build" "_deps") "opencv") "install"
Write-Host "=== OpenCV installs ==="
Get-ChildItem $opencvInstallRoot -Directory -ErrorAction SilentlyContinue | ForEach-Object {
    Write-Host $_.FullName
}
