@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
dumpbin /EXPORTS "F:\YiyangzaiCode\YZCodex\build\msvc2022_64\Release\Qt6Widgets.dll" > F:\YiyangzaiCode\_qt_exports.txt 2>&1
echo done
