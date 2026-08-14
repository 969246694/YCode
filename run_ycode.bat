@echo off
setlocal EnableExtensions
REM YCode launcher - fast startup.
REM Reads the API key from the registry directly (no PowerShell, no profile load),
REM so the console window flashes only briefly before the GUI appears.
REM API Key priority: machine, user, current session.

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "API_KEY="
for /f "tokens=2,*" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v DEEPSEEK_API_KEY 2^>nul') do set "API_KEY=%%b"
if not defined API_KEY (
    for /f "tokens=2,*" %%a in ('reg query "HKCU\Environment" /v DEEPSEEK_API_KEY 2^>nul') do set "API_KEY=%%b"
)
if not defined API_KEY (
    set "API_KEY=%DEEPSEEK_API_KEY%"
)

if not defined API_KEY (
    echo Error: DEEPSEEK_API_KEY was not found.
    echo Set it via manage_api_key.ps1, or: set DEEPSEEK_API_KEY=your-key-here
    pause
    exit /b 1
)

set "DEEPSEEK_API_KEY=%API_KEY%"

set "APP_EXE=%ROOT%\YZCodex\build\msvc2022_64\Release\YCode.exe"
if not exist "%APP_EXE%" (
    echo Error: application not found: %APP_EXE%
    pause
    exit /b 1
)

cd /d "%ROOT%"
start "" "%APP_EXE%"
exit /b 0
