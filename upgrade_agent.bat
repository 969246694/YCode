@echo off
chcp 65001 >nul
title YCode Agent 升级程序

echo.
echo ╔══════════════════════════════════════════╗
echo ║   🔄 YCode Agent 自动升级中...         ║
echo ╚══════════════════════════════════════════╝
echo.

REM 等待旧 agent 完全退出
echo [1/3] 等待旧版 Agent 退出...
:waitloop
ping 127.0.0.1 -n 2 >nul
REM 尝试检查 agent.exe 是否还被锁定
copy /y "F:\YiyangzaiCode\agent.exe" "F:\YiyangzaiCode\agent.exe.test" >nul 2>&1
if errorlevel 1 (
    echo     旧 Agent 仍在运行，继续等待...
    goto waitloop
)
del "F:\YiyangzaiCode\agent.exe.test" >nul 2>&1

echo [2/3] 安装新版本...
copy /y "F:\YiyangzaiCode\agent_v2.exe" "F:\YiyangzaiCode\agent.exe"
if errorlevel 1 (
    echo ❌ 升级失败！请手动操作
    pause
    exit /b 1
)

echo [3/3] 启动 YCode Agent v2.0 ...
echo.
start "YCode Agent v2.0" cmd /k "cd /d F:\YiyangzaiCode && agent.exe"
echo ✅ 升级完成！新版本已启动！
timeout /t 2 >nul
del "%~f0" >nul 2>&1
exit
