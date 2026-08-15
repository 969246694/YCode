@echo off
setlocal EnableExtensions
REM Run all tests: YCodeEngine CTest + Agent logic tests.
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"
set "YCODE_NONINTERACTIVE=1"

call :find_vs
if not exist "%VS_VCVARS64%" (
    echo Failed to find Visual Studio vcvars64.bat.
    exit /b 1
)
call "%VS_VCVARS64%" >nul 2>&1

if not defined VCPKG_INSTALLED set "VCPKG_INSTALLED=C:\vcpkg\installed\x64-windows"

echo ========================================
echo Engine tests
echo ========================================
"%ROOT%\YCodeEngine\build\msvc2022_64\Release\ycode_engine_tests.exe"
if errorlevel 1 (
    echo Engine tests failed. Run build_all.bat first.
    exit /b 1
)

echo ========================================
echo Agent logic tests
echo ========================================
cl /nologo /EHsc /utf-8 "%ROOT%\tests\agent_logic_tests.cpp" /I "%ROOT%\YCodeEngine\third_party" /I "%VCPKG_INSTALLED%\include" /link /LIBPATH:"%VCPKG_INSTALLED%\lib" libcurl.lib shell32.lib dbghelp.lib /OUT:"%ROOT%\_agent_tests.exe" >nul 2>&1
if errorlevel 1 (
    echo Agent test build failed.
    exit /b 1
)
"%ROOT%\_agent_tests.exe"
set "TEST_EXIT=%errorlevel%"
del "%ROOT%\_agent_tests.exe" >nul 2>&1
exit /b %TEST_EXIT%

:find_vs
if defined VS_VCVARS64 exit /b 0
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" set "VS_VCVARS64=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" & exit /b 0
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" set "VS_VCVARS64=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" & exit /b 0
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "%VSWHERE%" exit /b 0
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSINSTALL=%%i"
if defined VSINSTALL set "VS_VCVARS64=%VSINSTALL%\VC\Auxiliary\Build\vcvars64.bat"
exit /b 0
