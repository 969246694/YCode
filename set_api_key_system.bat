@echo off
REM 设置 DeepSeek API Key 到系统环境变量
REM 需要管理员权限运行

echo ========================================
echo 设置 DeepSeek API Key
echo ========================================

REM 检查是否以管理员身份运行
net session >nul 2>&1
if %errorLevel% == 0 (
    echo 以管理员身份运行
) else (
    echo 需要管理员权限，请右键以管理员身份运行此脚本
    pause
    exit /b 1
)

REM 获取当前用户级别的 API Key
for /f "tokens=*" %%i in ('powershell -Command "[Environment]::GetEnvironmentVariable('DEEPSEEK_API_KEY', [EnvironmentVariableTarget]::User)"') do set USER_KEY=%%i

if defined USER_KEY (
    echo 找到用户级别的 API Key: %USER_KEY:~0,10%...
    echo 正在设置到系统级别...

    REM 设置系统环境变量
    setx DEEPSEEK_API_KEY "%USER_KEY%" /M

    if %errorLevel% == 0 (
        echo 成功设置系统级别环境变量
        echo API Key: %USER_KEY:~0,10%...
        echo.
        echo 请重启 YCode 应用程序以使用新的环境变量
    ) else (
        echo 设置失败
    )
) else (
    echo 未找到用户级别的 API Key
    echo 请先设置用户级别的环境变量
)

echo.
pause