<#
.SYNOPSIS
    编译并运行 dx.d3d12 抓帧探针（真实 D3D12 交换链），按输出文本判定成败。

.DESCRIPTION
    探针本体是 scripts\d3d12_grab_probe.cpp + d3d12_grab_probe_stubs.cpp，
    被验证的代码是 libop\hook\D3D12Capture.cpp（同一份源码，不复制）。

    为什么按文本判定而不是退出码：D3D12 调试层开着时，进程退出期报活对象会把
    退出码改成 2173 (0x87D)，探针自己 return 的 0 传不出来（详见 doc\defect_fixes.md §70.7）。
    本脚本解析 stdout 的 "PASS:" / "FAIL" 行，自己返回 0/1。

.PARAMETER KeepArtifacts
    保留编译产物目录，便于用调试器复跑。

.PARAMETER NoDebugLayer
    关闭 D3D12 调试层。关掉后状态栅栏写错不会被报错（探针失去这一维敏感度），
    但退出码变得可信。

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\run_d3d12_probe.ps1
#>
[CmdletBinding()]
param(
    [switch]$KeepArtifacts,
    [switch]$NoDebugLayer
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$srcProbe = Join-Path $PSScriptRoot 'd3d12_grab_probe.cpp'
$srcStubs = Join-Path $PSScriptRoot 'd3d12_grab_probe_stubs.cpp'
$srcImpl  = Join-Path $repoRoot    'libop\hook\D3D12Capture.cpp'
# D3D12Capture.cpp 还用到 DxCaptureCommon 的 CopyImageData/GetImageBufferFormat 和
# SharedFrameChannelLease；这两份实现只依赖 SDK 头 + 标准库，直接编真代码，不写桩。
$srcCommon  = Join-Path $repoRoot 'libop\hook\DxCaptureCommon.cpp'
$srcChannel = Join-Path $repoRoot 'libop\hook\SharedFrameChannel.cpp'

foreach ($f in @($srcProbe, $srcStubs, $srcImpl, $srcCommon, $srcChannel)) {
    if (-not (Test-Path $f)) { Write-Host "[FAIL] 缺少源文件: $f" -ForegroundColor Red; exit 1 }
}

# ---------------------------------------------------------------- 找编译环境
$vcvarsCandidates = @(
    'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files (x86)\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files (x86)\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat',
    'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat'
)
$vcvars = $vcvarsCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $vcvars) {
    Write-Host '[FAIL] 没找到 vcvars64.bat（需要 VS2022 Build Tools 或 Community）' -ForegroundColor Red
    exit 1
}

# 探针只需要 <d3d12.h>/<dxgi1_4.h> 这类 SDK 头，C++ 标准库由 /MT 静态链。
# 不依赖 vcpkg 的 include：探针刻意不引入 libop 其余依赖（见 stubs）。
$vcpkgInc = Join-Path $repoRoot 'build\_deps\vcpkg\installed\x64-windows\include'
$extraInc = ''
if (Test-Path $vcpkgInc) { $extraInc = "/I`"$vcpkgInc`"" }

# ---------------------------------------------------------------- 编译到临时目录
$work = Join-Path $env:TEMP ("d3d12_probe_" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Path $work -Force | Out-Null

$cl = "cl /nologo /utf-8 /std:c++17 /EHsc /MT /O2 /W3 $extraInc " +
      "`"$srcProbe`" `"$srcStubs`" `"$srcImpl`" `"$srcCommon`" `"$srcChannel`" " +
      "/Fo:`"$work\\`" /Fe:`"$work\d3d12_grab_probe.exe`" " +
      "/link d3d12.lib dxgi.lib user32.lib ole32.lib"

Write-Host "[INFO] 编译探针（输出到 $work）..." -ForegroundColor Cyan
$buildLog = & cmd /c "`"$vcvars`" >nul && $cl 2>&1"
$buildExit = $LASTEXITCODE
$errors = $buildLog | Select-String -Pattern ': error|error LNK|fatal error'
if ($buildExit -ne 0 -or $errors) {
    Write-Host '[FAIL] 编译失败：' -ForegroundColor Red
    $buildLog | Select-Object -Last 25 | ForEach-Object { Write-Host "  $_" }
    exit 1
}
$warnings = $buildLog | Select-String -Pattern 'warning C'
if ($warnings) {
    Write-Host "[WARN] 编译告警 $($warnings.Count) 条：" -ForegroundColor Yellow
    $warnings | Select-Object -First 10 | ForEach-Object { Write-Host "  $_" }
}

# ---------------------------------------------------------------- 运行
Write-Host '[INFO] 运行探针（真实 D3D12 交换链）...' -ForegroundColor Cyan
if ($NoDebugLayer) { $env:OP_PROBE_NO_DEBUG_LAYER = '1' }
try {
    $output = & cmd /c "`"$work\d3d12_grab_probe.exe`" 2>&1"
    $procExit = $LASTEXITCODE
}
finally {
    if ($NoDebugLayer) { Remove-Item Env:\OP_PROBE_NO_DEBUG_LAYER -ErrorAction SilentlyContinue }
}

$output | ForEach-Object { Write-Host "  $_" }

# ---------------------------------------------------------------- 判定（看文本，不看退出码）
$text = ($output | Out-String)
$pass = $text -match 'PASS:\s*0 failure'
$hasFailLine = ($output | Select-String -Pattern '\[FAIL\]|^FAIL:').Count -gt 0

$ok = $pass -and (-not $hasFailLine)
if (-not $KeepArtifacts) { Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue }
else { Write-Host "[INFO] 保留产物：$work" -ForegroundColor Cyan }

if ($ok) {
    Write-Host '[ OK ] dx.d3d12 抓帧探针通过' -ForegroundColor Green
    if ($procExit -ne 0) {
        Write-Host "[INFO] 进程退出码 $procExit 忽略（调试层所致，见 §70.7）" -ForegroundColor DarkGray
    }
    exit 0
}

Write-Host '[FAIL] dx.d3d12 抓帧探针失败' -ForegroundColor Red
exit 1
