@echo off
echo ========================================
echo YZCodex Agent 测试脚本
echo ========================================

REM 检查 agent.exe 是否存在
if not exist "..\agent.exe" (
    echo 错误: 找不到 agent.exe，请先编译 C++ agent
    echo 运行上级目录的 build.bat
    pause
    exit /b 1
)

REM 复制 agent.exe 到当前目录
copy "..\agent.exe" . >nul

echo 启动 Agent 测试...
echo 按 Ctrl+C 退出
echo.

agent.exe

echo.
echo 测试完成
pause