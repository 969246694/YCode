@echo off
setlocal EnableExtensions EnableDelayedExpansion
chcp 65001 >nul

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "LOG=%ROOT%\ycode_self_update.log"
set "PULL_LATEST=0"
set "NO_LAUNCH=0"

:parse_args
if "%~1"=="" goto args_done
if /I "%~1"=="--pull" set "PULL_LATEST=1"
if /I "%~1"=="--no-launch" set "NO_LAUNCH=1"
shift
goto parse_args
:args_done

> "%LOG%" echo [%date% %time%] YCode self update started.
call :log Root: "%ROOT%"

call :wait_for_exit "YCode.exe" 90
if errorlevel 1 goto fail

call :wait_for_exit "agent.exe" 90
if errorlevel 1 goto fail

set "YCODE_NONINTERACTIVE=1"

if "%PULL_LATEST%"=="1" (
    call :log Pulling latest source from origin/main...
    git -C "%ROOT%" pull --ff-only origin main >> "%LOG%" 2>&1
    if errorlevel 1 goto fail
)

call :log Building agent.exe...
call "%ROOT%\build.bat" >> "%LOG%" 2>&1
if errorlevel 1 goto fail

call :log Building YCode client...
call "%ROOT%\YZCodex\build.bat" >> "%LOG%" 2>&1
if errorlevel 1 goto fail

call :log Updating desktop shortcut...
powershell -NoProfile -ExecutionPolicy Bypass -File "%ROOT%\update_shortcut.ps1" >> "%LOG%" 2>&1
if errorlevel 1 goto fail

if "%NO_LAUNCH%"=="1" (
    call :log Skipping YCode launch because --no-launch was specified.
    call :log YCode self update completed.
    exit /b 0
)

call :log Restarting YCode...
start "" "%ROOT%\run_ycode.bat"
call :log YCode self update completed.
exit /b 0

:wait_for_exit
set "PROC=%~1"
set /a "LIMIT=%~2"
set /a COUNT=0
:wait_loop
tasklist /FI "IMAGENAME eq %PROC%" 2>nul | find /I "%PROC%" >nul
if errorlevel 1 (
    call :log "%PROC% has exited."
    exit /b 0
)
if !COUNT! geq !LIMIT! (
    call :log Timed out waiting for "%PROC%" to exit.
    exit /b 1
)
timeout /t 1 /nobreak >nul
set /a COUNT+=1
goto wait_loop

:log
echo [%date% %time%] %*
>> "%LOG%" echo [%date% %time%] %*
exit /b 0

:fail
call :log YCode self update failed. See "%LOG%".
start "YCode self update failed" notepad "%LOG%"
exit /b 1
