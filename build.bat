@echo off
call "E:\Visual Studio\Visual Studio Community 2022\VC\Auxiliary\Build\vcvars64.bat"
if %errorlevel% neq 0 (
    echo Failed to initialize Visual Studio build environment.
    if not "%YCODE_NONINTERACTIVE%"=="1" pause
    exit /b 1
)

cl /EHsc /utf-8 agent.cpp /I C:\vcpkg\installed\x64-windows\include /link /LIBPATH:C:\vcpkg\installed\x64-windows\lib libcurl.lib shell32.lib
if %errorlevel% neq 0 (
    echo Agent build failed.
    if not "%YCODE_NONINTERACTIVE%"=="1" pause
    exit /b 1
)

if exist "C:\vcpkg\installed\x64-windows\bin\*.dll" (
    xcopy /y /q "C:\vcpkg\installed\x64-windows\bin\*.dll" "%~dp0" >nul
)
echo.
echo Build completed. Run agent.exe to start.
