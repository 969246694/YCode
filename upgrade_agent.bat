@echo off
chcp 65001 >nul
title YCode Agent 升级程序

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

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
copy /y "%ROOT%\agent.exe" "%ROOT%\agent.exe.test" >nul 2>&1
if errorlevel 1 (
    echo     旧 Agent 仍在运行，继续等待...
    goto waitloop
)
del "%ROOT%\agent.exe.test" >nul 2>&1

echo [2/3] 安装新版本...
copy /y "%ROOT%\agent_v2.exe" "%ROOT%\agent.exe"
if errorlevel 1 (
    echo ❌ 升级失败！请手动操作
    pause
    exit /b 1
)

echo [3/3] 启动 YCode Agent v2.0 ...
echo.
start "YCode Agent v2.0" cmd /k "cd /d ""%ROOT%"" && agent.exe"
echo ✅ 升级完成！新版本已启动！
timeout /t 2 >nul
del "%~f0" >nul 2>&1
exit
