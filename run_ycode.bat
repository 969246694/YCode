@echo off
REM YCode launcher. Ensures DEEPSEEK_API_KEY is passed to the client.

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

echo ========================================
echo Starting YCode
echo ========================================

REM API Key priority: machine, user, current session.
set "API_KEY="
for /f "tokens=*" %%i in ('powershell -Command "[Environment]::GetEnvironmentVariable(''DEEPSEEK_API_KEY'', [EnvironmentVariableTarget]::Machine)"') do set API_KEY=%%i
if not defined API_KEY (
    for /f "tokens=*" %%i in ('powershell -Command "[Environment]::GetEnvironmentVariable(''DEEPSEEK_API_KEY'', [EnvironmentVariableTarget]::User)"') do set API_KEY=%%i
)
if not defined API_KEY (
    set API_KEY=%DEEPSEEK_API_KEY%
)

if not defined API_KEY (
    echo Error: DEEPSEEK_API_KEY was not found.
    echo.
    echo Set it with one of these commands:
    echo 1. Current session: set DEEPSEEK_API_KEY=your-api-key-here
    echo 2. User level: powershell -Command "[Environment]::SetEnvironmentVariable('DEEPSEEK_API_KEY', 'your-api-key-here', [EnvironmentVariableTarget]::User)"
    echo 3. System level: run set_api_key_system.bat as Administrator
    echo.
    pause
    exit /b 1
)

echo API Key is set: %API_KEY:~0,10%...
echo Working directory: %ROOT%
echo.

set "DEEPSEEK_API_KEY=%API_KEY%"

set "APP_DIR=%ROOT%\YZCodex\build\msvc2022_64\Release"
set "APP_EXE=%APP_DIR%\YCode.exe"

if not exist "%APP_EXE%" (
    echo Error: application not found: %APP_EXE%
    pause
    exit /b 1
)

cd /d "%ROOT%"
start "" "%APP_EXE%"
echo YCode started.
