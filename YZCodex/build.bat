@echo off
echo ========================================
echo Building YCode Qt client
echo ========================================

REM Visual Studio build environment
call "E:\Visual Studio\Visual Studio Community 2022\VC\Auxiliary\Build\vcvars64.bat"
if %errorlevel% neq 0 (
    echo Error: failed to initialize Visual Studio build environment.
    if not "%YCODE_NONINTERACTIVE%"=="1" pause
    exit /b 1
)

REM Qt installation
set "QT_DIR=C:\Qt\6.8.0\msvc2022_64"
if not exist "%QT_DIR%" (
    echo Error: Qt directory not found: %QT_DIR%
    echo Update QT_DIR in YZCodex\build.bat or install Qt 6.8.0 msvc2022_64.
    if not "%YCODE_NONINTERACTIVE%"=="1" pause
    exit /b 1
)

set "CMAKE_PREFIX_PATH=%QT_DIR%\lib\cmake"

REM Switch to this script directory
cd /d "%~dp0"

REM Create build directory
set "BUILD_DIR=%~dp0build\msvc2022_64"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

echo Configuring project...
cmake "%~dp0." -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%" -A x64
if %errorlevel% neq 0 (
    echo Error: CMake configure failed.
    if not "%YCODE_NONINTERACTIVE%"=="1" pause
    exit /b 1
)

echo Building project...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Error: build failed.
    if not "%YCODE_NONINTERACTIVE%"=="1" pause
    exit /b 1
)

echo.
echo ========================================
echo Build succeeded.
echo Executable: build\msvc2022_64\Release\YCode.exe
echo ========================================
echo.
echo Set DEEPSEEK_API_KEY before running:
echo set DEEPSEEK_API_KEY=your-api-key-here
echo.
if not "%YCODE_NONINTERACTIVE%"=="1" pause
