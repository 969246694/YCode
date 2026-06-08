@echo off
call "E:\Visual Studio\Visual Studio Community 2022\VC\Auxiliary\Build\vcvars64.bat"
cl /EHsc /utf-8 agent.cpp /I C:\vcpkg\installed\x64-windows\include /link /LIBPATH:C:\vcpkg\installed\x64-windows\lib libcurl.lib
if exist "C:\vcpkg\installed\x64-windows\bin\*.dll" (
    xcopy /y /q "C:\vcpkg\installed\x64-windows\bin\*.dll" "%~dp0" >nul
)
echo.
echo Build completed! Run agent.exe to start.