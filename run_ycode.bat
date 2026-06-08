@echo off
REM YCode 启动脚本，确保 API Key 正确传递

echo ========================================
echo 启动 YCode
echo ========================================

REM 获取 API Key（优先级：系统 > 用户 > 当前会话）
set "API_KEY="
for /f "tokens=*" %%i in ('powershell -Command "[Environment]::GetEnvironmentVariable(''DEEPSEEK_API_KEY'', [EnvironmentVariableTarget]::Machine)"') do set API_KEY=%%i
if not defined API_KEY (
    for /f "tokens=*" %%i in ('powershell -Command "[Environment]::GetEnvironmentVariable(''DEEPSEEK_API_KEY'', [EnvironmentVariableTarget]::User)"') do set API_KEY=%%i
)
if not defined API_KEY (
    set API_KEY=%DEEPSEEK_API_KEY%
)

if not defined API_KEY (
    echo 错误: 未找到 DEEPSEEK_API_KEY 环境变量
    echo.
    echo 请运行以下命令之一设置 API Key:
    echo 1. 当前会话: set DEEPSEEK_API_KEY=your-api-key-here
    echo 2. 用户级别: powershell -Command "[Environment]::SetEnvironmentVariable('DEEPSEEK_API_KEY', 'your-api-key-here', [EnvironmentVariableTarget]::User)"
    echo 3. 系统级别: 以管理员身份运行 set_api_key_system.bat
    echo.
    pause
    exit /b 1
)

echo API Key 已设置: %API_KEY:~0,10%...
echo 工作目录: %CD%
echo.

REM 设置环境变量
set DEEPSEEK_API_KEY=%API_KEY%

REM 启动程序
set "APP_DIR=F:\YiyangzaiCode\YZCodex\build\msvc2022_64\Release"
set "APP_EXE=%APP_DIR%\YCode.exe"

if not exist "%APP_EXE%" (
    echo 错误: 找不到应用程序: %APP_EXE%
    pause
    exit /b 1
)

cd /d "F:\YiyangzaiCode"
start "" "%APP_EXE%"
echo YCode 已启动
