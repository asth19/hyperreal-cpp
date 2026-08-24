@echo off
REM ============================================================
REM   Hyperreal 超实数库 —— 双击一键编译 & 运行
REM   作用：绕过 PowerShell 默认执行策略，并在用户系统上直接双击可用
REM ============================================================

REM 切到当前脚本所在目录（避免用户把快捷方式放别的位置）
cd /d "%~dp0"

REM 以 Bypass 执行策略调用 PowerShell 主脚本
REM  -NoProfile  ：不加载用户 profile，避免自定义脚本干扰
REM  -ExecutionPolicy Bypass ：绕过"禁止运行脚本"的默认策略
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0build_and_run.ps1"

REM 脚本内部已有暂停，此处只需等它结束再退出 cmd
exit /b %ERRORLEVEL%
