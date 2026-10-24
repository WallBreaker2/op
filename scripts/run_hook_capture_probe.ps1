<#
.SYNOPSIS
    编译并运行后台（hook 注入式）截图探针，按输出文本判定成败并打印每帧耗时。

.DESCRIPTION
    探针本体是 scripts\hook_capture_probe.cpp + hook_capture_probe_stubs.cpp，
    被验证的代码是 libop\hook\D3D11Capture.cpp / DxCaptureCommon.cpp /
    SharedFrameChannel.cpp（同一份源码，不复制）。

    探针在本进程里建一个真 D3D11 交换链 + 隐藏窗口，直接调 op::hook::dx11_capture()，
    所以它测的是"目标渲染线程被占用多少"这件事本身 —— 也就是玩家感知的卡顿来源。

    判定看 stdout 的 "PASS: 0 failure(s)" 行，不看退出码（沿用 d3d12 探针的约定：
    退出码可能被显卡驱动/调试层改掉，文本不会被改）。

.PARAMETER Width / .PARAMETER Height
    渲染分辨率，默认 1920x1080。

.PARAMETER Iters
    采样帧数，默认 60。

.PARAMETER Rgba
    交换链用 R8G8B8A8 而不是默认的 B8G8R8A8，用来验证 CopyImageData 的通道交换路径。

.PARAMETER CopyBench
    附加整帧搬运基准（memcpy / SSE2 流式写 / 读回）。会改写共享段的像素区，
    所以产品代码把校验收在它之前跑，加了这个开关校验也照样有效。

.PARAMETER Phase
    分阶段计时（GetBuffer / CopyResource / Map / open / 拷贝 / Unmap）。

.PARAMETER KeepArtifacts
    保留编译产物目录，便于用调试器复跑。

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\run_hook_capture_probe.ps1

.EXAMPLE
    powershell -ExecutionPolicy Bypass -File scripts\run_hook_capture_probe.ps1 -Width 2560 -Height 1440 -Phase
#>
[CmdletBinding()]
param(
    [int]$Width = 1920,
    [int]$Height = 1080,
    [int]$Iters = 60,
    [switch]$Rgba,
    [switch]$CopyBench,
    [switch]$Phase,
    [switch]$KeepArtifacts
)

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$sources = @(
    (Join-Path $PSScriptRoot 'hook_capture_probe.cpp'),
    (Join-Path $PSScriptRoot 'hook_capture_probe_stubs.cpp'),
    (Join-Path $repoRoot    'libop\hook\D3D11Capture.cpp'),
    (Join-Path $repoRoot    'libop\hook\DxCaptureCommon.cpp'),
    (Join-Path $repoRoot    'libop\hook\SharedFrameChannel.cpp')
)

foreach ($f in $sources) {
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

# 探针只需要 d3d11/dxgi 这类 SDK 头；libop 里被拉进来的头只用到 std，
# 所以不需要 vcpkg 的 include。标准库由 /MT 静态链。
# 必须是 C++20：libop\hook\SharedFrameWriter.h 用了 std::span。
$work = Join-Path $env:TEMP ("hook_probe_" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
New-Item -ItemType Directory -Path $work -Force | Out-Null

$srcList = ($sources | ForEach-Object { "`"$_`"" }) -join ' '
$cl = "cl /nologo /utf-8 /std:c++20 /EHsc /MT /O2 " +
      "$srcList " +
      "/Fo:`"$work\\`" /Fe:`"$work\hook_capture_probe.exe`" " +
      "/link d3d11.lib dxgi.lib user32.lib ole32.lib oleaut32.lib gdi32.lib"

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
$probeArgs = @("--w=$Width", "--h=$Height", "--iters=$Iters")
if ($Rgba)      { $probeArgs += '--rgba' }
if ($CopyBench) { $probeArgs += '--copybench' }
if ($Phase)     { $probeArgs += '--phase' }

Write-Host "[INFO] 运行探针（真实 D3D11 交换链，$Width x $Height，$Iters 帧）..." -ForegroundColor Cyan
$output = & cmd /c "`"$work\hook_capture_probe.exe`" $($probeArgs -join ' ') 2>&1"
$procExit = $LASTEXITCODE

$output | ForEach-Object { Write-Host "  $_" }

# ---------------------------------------------------------------- 判定（看文本，不看退出码）
$text = ($output | Out-String)
$pass = $text -match 'PASS:\s*0 failure'
$hasFailLine = ($output | Select-String -Pattern '\[FAIL\]|^FAIL:').Count -gt 0
$metric = ($output | Select-String -Pattern 'METRIC capture_mean_ms' | ForEach-Object { $_.Line }) -join ' '

$ok = $pass -and (-not $hasFailLine)
if (-not $KeepArtifacts) { Remove-Item $work -Recurse -Force -ErrorAction SilentlyContinue }
else { Write-Host "[INFO] 保留产物：$work" -ForegroundColor Cyan }

if ($ok) {
    Write-Host "[ OK ] 后台截图探针通过  $metric" -ForegroundColor Green
    if ($procExit -ne 0) {
        Write-Host "[INFO] 进程退出码 $procExit 忽略（按文本判定）" -ForegroundColor DarkGray
    }
    exit 0
}

Write-Host '[FAIL] 后台截图探针失败' -ForegroundColor Red
exit 1
