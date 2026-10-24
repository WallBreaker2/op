#requires -version 5
<#
OP 环境自检（不需要管理员权限，幂等，只读）

用途：当出现"只有连跑才失败 / 截图取不到帧 / OCR 用例全跳过"这类现象时，
先跑这一条命令把环境事实一次性填出来，再决定是排查代码还是环境。

用法：
    powershell -ExecutionPolicy Bypass -File scripts\check_env.ps1
    powershell -ExecutionPolicy Bypass -File scripts\check_env.ps1 -SkipCapture

输出分三段：事实（Facts）→ 判定（Verdicts）→ 建议命令（Next steps）。
# 注意：本文件必须保存为 UTF-8 **带 BOM**。Windows PowerShell 5.1 读不带 BOM 的中文脚本会按 ANSI 解析，
#       直接报 "The Try statement is missing its Catch or Finally block" 这类假错误（本项目踩过）。
#>

[CmdletBinding()]
param(
    [string]$TestExe = "",
    [switch]$SkipCapture
)

$ErrorActionPreference = "Continue"

function Write-Section($title) {
    Write-Host ""
    Write-Host "== $title ==" -ForegroundColor Cyan
}

function Write-Fact($name, $value) {
    "{0,-30} {1}" -f $name, $value
}

function Write-Verdict($ok, $text) {
    if ($ok) { Write-Host ("  [OK]   " + $text) -ForegroundColor Green }
    else { Write-Host ("  [WARN] " + $text) -ForegroundColor Yellow }
}

# ---------- 1. 会话与系统 ----------
Write-Section "会话与系统"
$session = $env:SESSIONNAME
$isRdp = $session -like "RDP*"
$isConsole = $session -eq "Console"
Write-Fact "SESSIONNAME" $session
$os = Get-CimInstance Win32_OperatingSystem -ErrorAction SilentlyContinue
if ($os) { Write-Fact "操作系统" ("{0} / build {1}" -f $os.Caption, $os.BuildNumber) }

# ---------- 2. 显示适配器 ----------
Write-Section "显示适配器"
$adapters = @()
try { $adapters = Get-CimInstance Win32_VideoController -ErrorAction Stop } catch { }
$virtualPatterns = "IDD|Virtual|Oray|GameViewer|AskLink|Parsec|RustDesk|Sunlogin|Todesk|Mirror|Indirect"
$virtualCount = 0
foreach ($a in $adapters) {
    $isVirtual = $a.Name -match $virtualPatterns
    if ($isVirtual) { $virtualCount++ }
    $tag = if ($isVirtual) { "虚拟/IDD" } else { "物理" }
    Write-Fact ("  " + $a.Name) ("{0}  status={1}" -f $tag, $a.Status)
}
if ($adapters.Count -eq 0) { Write-Fact "  (无适配器信息)" "-" }

# ---------- 3. 显示器布局 ----------
Write-Section "显示器布局"
try {
    Add-Type -AssemblyName System.Windows.Forms -ErrorAction Stop
    $screens = [System.Windows.Forms.Screen]::AllScreens
    Write-Fact "显示器数量" $screens.Count
    foreach ($s in $screens) {
        Write-Fact ("  " + $s.DeviceName) ("primary={0} bounds={1} work={2}" -f $s.Primary, $s.Bounds, $s.WorkingArea)
    }
} catch {
    Write-Fact "显示器信息" "读取失败: $_"
}

# ---------- 4. DWM ----------
Write-Section "桌面合成"
$dwm = Get-Process dwm -ErrorAction SilentlyContinue
if ($dwm) { Write-Fact "dwm.exe" ("运行中 (pid " + $dwm.Id + ")") } else { Write-Fact "dwm.exe" "未运行" }

# ---------- 5. OCR ----------
Write-Section "OCR"
$ocrUrl = $env:OP_OCR_URL
if ([string]::IsNullOrEmpty($ocrUrl)) {
    $backend = $env:OP_OCR_BACKEND
    switch -Regex ($backend) {
        "^tess" { $ocrUrl = "http://127.0.0.1:8080/api/v1/ocr" }
        "^paddle$|^paddleocr$" { $ocrUrl = "http://127.0.0.1:8081/api/v1/ocr" }
        default { $ocrUrl = "http://127.0.0.1:8082/api/v1/ocr" }
    }
}
Write-Fact "OP_OCR_URL/端点" $ocrUrl
$healthUrl = ($ocrUrl -replace "/api/v1/.*$", "/health")
$ocrHealthy = $false
try {
    $resp = Invoke-WebRequest -Uri $healthUrl -TimeoutSec 3 -UseBasicParsing -NoProxy -ErrorAction Stop
    $ocrHealthy = $true
    Write-Fact "健康检查" $resp.Content
} catch {
    Write-Fact "健康检查" "不可达（OCR 识别用例会被跳过）"
}
$tesseract = Get-Command tesseract -ErrorAction SilentlyContinue
if ($tesseract) { Write-Fact "tesseract.exe" $tesseract.Source } else { Write-Fact "tesseract.exe" "未安装" }

# ---------- 6. 测试二进制 & DXGI 取帧 ----------
Write-Section "测试与截图"
if ([string]::IsNullOrEmpty($TestExe)) {
    $candidates = @("bin\x64\op_test.exe", "bin\x86\op_test.exe")
    foreach ($c in $candidates) {
        $full = Join-Path (Get-Location) $c
        if (Test-Path $full) { $TestExe = $full; break }
    }
}
if ($TestExe -and (Test-Path $TestExe)) {
    Write-Fact "op_test.exe" $TestExe
    if (-not $SkipCapture) {
        $filter = "CaptureBackendTest.DxgiRegionCaptureReusesLargerStagingTexture"
        $out = & $TestExe --gtest_filter=$filter 2>&1 | Out-String
        if ($out -match "\[       OK \]") {
            Write-Fact "DXGI 取帧" "成功（本次能拿到新帧）"
        } elseif ($out -match "\[  SKIPPED \]") {
            Write-Fact "DXGI 取帧" "跳过（本次拿不到新帧 —— 见下面的判定）"
        } elseif ($out -match "\[  FAILED  \]") {
            Write-Fact "DXGI 取帧" "失败"
        } else {
            Write-Fact "DXGI 取帧" "未得出结论"
        }
    } else {
        Write-Fact "DXGI 取帧" "（已按 -SkipCapture 跳过）"
    }
} else {
    Write-Fact "op_test.exe" "未找到（先跑 python build.py）"
}

# ---------- 判定 ----------
Write-Section "判定"
Write-Verdict $isConsole "会话是 Console（真实桌面）；RDP/服务会话下桌面复制与输入注入都不可靠"
if (-not $isConsole) { Write-Verdict $false ("当前 SESSIONNAME=" + $session + "，建议在控制台会话里跑截图/输入类用例") }
Write-Verdict ($virtualCount -eq 0) ("显示适配器里虚拟/IDD 数量 = " + $virtualCount + "；>0 时 DXGI 桌面复制可能拿不到新帧")
Write-Verdict ([bool]$dwm) "DWM 在运行（桌面复制依赖它）"
if ($ocrHealthy) { Write-Verdict $true "OCR 端点可达：OcrFixture 的 25 条识别用例会真跑" }
else { Write-Verdict $false "OCR 端点不可达：OcrFixture 的 25 条识别用例会全部跳过（环境限制，不是缺陷）" }
Write-Verdict ([bool]$tesseract) "本机是否装了 Tesseract（跑真实 OCR 需要它 + pip install pytesseract pillow）"

# ---------- 建议 ----------
Write-Section "下一步"
Write-Host "  · 只想跑不依赖桌面的用例：ctest -C Release -R ""op_test|ocr_contract"""
Write-Host "  · DXGI 截图偶发失败：按上面 4 项事实对照 doc/capture_perf.md §10 的判定表"
Write-Host "  · 想让 OCR 用例真跑："
Write-Host "      1) 安装 tesseract.exe 并加入 PATH；pip install pytesseract pillow"
Write-Host "      2) python scripts\op_ocr_server_min.py --backend tesseract --lang eng --port 8082"
Write-Host "      3) set OP_OCR_URL=http://127.0.0.1:8082/api/v1/ocr"
Write-Host "  · 本机存在系统 HTTP 代理时，用 Invoke-WebRequest 验本地服务要加 -NoProxy（本脚本已加）"
Write-Host ""
