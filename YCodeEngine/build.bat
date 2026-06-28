@echo off
setlocal EnableExtensions

echo ========================================
echo Building YCode Engine
echo ========================================

call :find_vs

if exist "%VS_VCVARS64%" goto have_vs
echo Error: failed to find Visual Studio vcvars64.bat.
echo Set VS_VCVARS64 to your Visual Studio vcvars64.bat path.
if not "%YCODE_NONINTERACTIVE%"=="1" pause
exit /b 1

:have_vs

call "%VS_VCVARS64%"
if errorlevel 1 (
    echo Error: failed to initialize Visual Studio build environment.
    if not "%YCODE_NONINTERACTIVE%"=="1" pause
    exit /b 1
)

cd /d "%~dp0"

set "BUILD_DIR=%~dp0build\msvc2022_64"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

echo Configuring YCode Engine...
cmake "%~dp0." -DCMAKE_BUILD_TYPE=Release -A x64
if errorlevel 1 (
    echo Error: CMake configure failed.
    if not "%YCODE_NONINTERACTIVE%"=="1" pause
    exit /b 1
)

echo Building YCode Engine...
cmake --build . --config Release
if errorlevel 1 (
    echo Error: build failed.
    if not "%YCODE_NONINTERACTIVE%"=="1" pause
    exit /b 1
)

echo.
echo ========================================
echo YCode Engine build succeeded.
echo Launcher: build\msvc2022_64\Release\ycode_engine_launcher.exe
echo ========================================
if not "%YCODE_NONINTERACTIVE%"=="1" pause
exit /b 0

:find_vs
if defined VS_VCVARS64 exit /b 0
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VS_VCVARS64=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" & exit /b 0
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" set "VS_VCVARS64=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" & exit /b 0
if exist "E:\Visual Studio\Visual Studio Community 2022\VC\Auxiliary\Build\vcvars64.bat" set "VS_VCVARS64=E:\Visual Studio\Visual Studio Community 2022\VC\Auxiliary\Build\vcvars64.bat" & exit /b 0
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 0
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
if defined VSINSTALL set "VS_VCVARS64=%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat"
exit /b 0

