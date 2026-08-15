@echo off
setlocal EnableExtensions
REM Build all YCode components in order: agent, YCodeEngine, YZCodex.
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "YCODE_NONINTERACTIVE=1"

echo ========================================
echo Building agent.exe
echo ========================================
call "%ROOT%\build.bat"
if errorlevel 1 (
    echo agent build failed.
    exit /b 1
)

echo ========================================
echo Building YCodeEngine
echo ========================================
call "%ROOT%\YCodeEngine\build.bat"
if errorlevel 1 (
    echo YCodeEngine build failed.
    exit /b 1
)

echo ========================================
echo Building YZCodex (Qt client)
echo ========================================
call "%ROOT%\YZCodex\build.bat"
if errorlevel 1 (
    echo YZCodex build failed.
    exit /b 1
)

echo.
echo ========================================
echo All components built successfully.
echo ========================================
exit /b 0
