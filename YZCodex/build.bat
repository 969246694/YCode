@echo off
echo ========================================
echo YZCodex Qt 客户端编译脚本
echo ========================================

REM 使用 Visual Studio 编译环境
call "E:\Visual Studio\Visual Studio Community 2022\VC\Auxiliary\Build\vcvars64.bat"
if %errorlevel% neq 0 (
    echo 错误: 无法加载 Visual Studio 编译环境，请检查路径是否正确。
    pause
    exit /b 1
)

REM Qt 安装目录
set "QT_DIR=C:\Qt\6.8.0\msvc2022_64"
if not exist "%QT_DIR%" (
    echo 错误: 未找到 Qt 安装目录: %QT_DIR%
    echo 请修改 YZCodex\build.bat 中的 QT_DIR 或安装 Qt 6.8.0 msvc2022_64。
    pause
    exit /b 1
)

set "CMAKE_PREFIX_PATH=%QT_DIR%\lib\cmake"

REM 切换到脚本所在目录
cd /d "%~dp0"

REM 创建构建目录
set "BUILD_DIR=%~dp0build\msvc2022_64"
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

REM 配置项目
echo 配置项目...
cmake "%~dp0." -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%CMAKE_PREFIX_PATH%" -A x64
if %errorlevel% neq 0 (
    echo 错误: CMake 配置失败
    cd ..
    pause
    exit /b 1
)

REM 编译项目
echo 编译项目...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo 错误: 编译失败
    cd ..
    pause
    exit /b 1
)

echo.
echo ========================================
echo 编译成功！
echo 可执行文件: build\msvc2022_64\Release\YZCodex.exe
echo ========================================
echo.
echo 运行前请设置 API Key:
echo set DEEPSEEK_API_KEY=your-api-key-here
echo.
pause