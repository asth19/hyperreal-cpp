# ============================================================
#  Hyperreal 超实数库 —— 一键编译 & 运行脚本
#  用法：右键此文件 → "使用 PowerShell 运行"，或配合 build_and_run.cmd 双击
# ============================================================

# -------- 1. 切到脚本所在目录（避免在用户桌面/其它目录执行找不到源文件） --------
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $ScriptDir

# -------- 2. 控制台 UTF-8 输出（三步法：切代码页 → 同步 .NET 读写编码） --------
# PowerShell 5 默认：OutputEncoding=ASCII，Console Output/Input=系统代码页(936/GBK)
# 解决沙盒里出现鏈熸湜式乱码的根本原因：Console InputEncoding 缺失 + 顺序不对
chcp 65001 | Out-Null                                                     # 1) 控制台代码页切到 UTF-8（必须先切）
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)      # 2) .NET 写控制台（Write-Host 等）
[Console]::InputEncoding  = [System.Text.UTF8Encoding]::new($false)      # 3) .NET 读外部程序输出（cmake/ninja/test_hyperreal 中文）
$OutputEncoding           = [System.Text.UTF8Encoding]::new($false)      # 4) PowerShell pipe 给外部程序时的编码

# -------- 3. 色标辅助函数（可选的视觉美化） --------
function Write-Info($m)  { Write-Host " [INFO]  $m" -ForegroundColor Cyan }
function Write-Succ($m)  { Write-Host " [ OK ]  $m" -ForegroundColor Green }
function Write-Warn($m)  { Write-Host " [WARN]  $m" -ForegroundColor Yellow }
function Write-Err($m)   { Write-Host " [FAIL]  $m" -ForegroundColor Red }
function Pause-IfDoubleClicked {
    # 仅在检测到"交互式运行"（非管道）时才暂停；避免 CI 卡住
    if ([Environment]::UserInteractive) {
        Write-Host ""
        Write-Host "按任意键退出..." -ForegroundColor DarkGray
        $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
    }
}

# -------- 4. 工具链检查 --------
Write-Info "检查工具链..."

$tools = @{
    cmake = & { try { & cmake --version 2>$null | Select-Object -First 1 } catch {} }
    ninja = & { try { & ninja --version 2>$null } catch {} }
    gpp   = & { try { & g++ --version 2>$null | Select-Object -First 1 } catch {} }
}

$missing = @()
if (-not $tools.cmake) { $missing += "cmake (请确保已加入 PATH)" }
if (-not $tools.ninja) { $missing += "ninja (MinGW/bin 下通常自带，请确保已加入 PATH)" }
if (-not $tools.gpp)   { $missing += "g++/MinGW (本项目依赖 GCC/MinGW 工具链)" }

if ($missing.Count -gt 0) {
    Write-Err "缺少以下工具："
    $missing | ForEach-Object { Write-Err "  - $_" }
    Write-Warn "当前 PATH 中只搜索到已安装的程序；若已安装但未加入 PATH，请先配置环境变量。"
    Pause-IfDoubleClicked
    exit 1
}

Write-Succ "cmake:  $($tools.cmake)"
Write-Succ "ninja:  $($tools.ninja)"
Write-Succ "g++  :  $($tools.gpp)"

# -------- 5. 构建目录 & CMake 配置 --------
$BuildDir = Join-Path $ScriptDir "build"
$ExePath  = Join-Path $BuildDir "bin\test_hyperreal.exe"

Write-Info "生成构建文件 (Ninja)..."
$cmakeArgs = @(
    "-S", ".",
    "-B", $BuildDir,
    "-G", "Ninja"
)
& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Err "CMake 配置失败，退出码 $LASTEXITCODE"
    Pause-IfDoubleClicked
    exit $LASTEXITCODE
}
Write-Succ "CMake 配置完成"

# -------- 6. 编译 --------
Write-Info "开始编译..."
& cmake --build $BuildDir
if ($LASTEXITCODE -ne 0) {
    Write-Err "编译失败，退出码 $LASTEXITCODE"
    Pause-IfDoubleClicked
    exit $LASTEXITCODE
}
Write-Succ "编译成功：$ExePath"

# -------- 7. 运行 --------
if (-not (Test-Path $ExePath)) {
    Write-Err "找不到可执行文件：$ExePath"
    Pause-IfDoubleClicked
    exit 2
}

Write-Info "执行程序...`n"
Write-Host ("=" * 60) -ForegroundColor DarkCyan
& $ExePath
$runExit = $LASTEXITCODE
Write-Host ("=" * 60) -ForegroundColor DarkCyan

if ($runExit -ne 0) {
    Write-Warn "程序退出码：$runExit（非 0）"
} else {
    Write-Succ "程序正常结束（退出码 0）"
}

Pause-IfDoubleClicked
exit $runExit
