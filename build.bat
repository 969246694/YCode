@echo off
setlocal EnableExtensions

set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

if not defined VCPKG_ROOT set "VCPKG_ROOT=C:\vcpkg"
if not defined VCPKG_TRIPLET set "VCPKG_TRIPLET=x64-windows"

call :find_vs

if exist "%VS_VCVARS64%" goto have_vs
echo Failed to find Visual Studio vcvars64.bat.
echo Set VS_VCVARS64 to your Visual Studio vcvars64.bat path.
if not "%YCODE_NONINTERACTIVE%"=="1" pause
exit /b 1

:have_vs

call "%VS_VCVARS64%"
if errorlevel 1 (
    echo Failed to initialize Visual Studio build environment.
    if not "%YCODE_NONINTERACTIVE%"=="1" pause
    exit /b 1
)

set "VCPKG_INSTALLED=%VCPKG_ROOT%\installed\%VCPKG_TRIPLET%"
if not exist "%VCPKG_INSTALLED%\include" (
    echo Failed to find vcpkg include directory: %VCPKG_INSTALLED%\include
    echo Set VCPKG_ROOT and VCPKG_TRIPLET for your vcpkg installation.
    if not "%YCODE_NONINTERACTIVE%"=="1" pause
    exit /b 1
)

cd /d "%ROOT%"
cl /EHsc /utf-8 "%ROOT%\agent.cpp" /I "%ROOT%\YCodeEngine\third_party" /I "%VCPKG_INSTALLED%\include" /link /LIBPATH:"%VCPKG_INSTALLED%\lib" libcurl.lib shell32.lib /OUT:"%ROOT%\agent.exe"
if errorlevel 1 (
    echo Agent build failed.
    if not "%YCODE_NONINTERACTIVE%"=="1" pause
    exit /b 1
)

if exist "%VCPKG_INSTALLED%\bin\*.dll" (
    xcopy /y /q "%VCPKG_INSTALLED%\bin\*.dll" "%ROOT%\" >nul
)

echo.
echo Build completed. Run agent.exe to start.
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
