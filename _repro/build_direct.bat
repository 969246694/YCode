@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d F:\YiyangzaiCode\_repro
cl /nologo /EHsc /std:c++17 /Zc:__cplusplus /utf-8 /DUNICODE /D_UNICODE /I "C:\Qt\6.8.0\msvc2022_64\include" /I "C:\Qt\6.8.0\msvc2022_64\include\QtCore" /I "C:\Qt\6.8.0\msvc2022_64\include\QtGui" /I "C:\Qt\6.8.0\msvc2022_64\include\QtWidgets" main_nofilemodel.cpp /Fe:repro_nofm.exe /link /LIBPATH:"C:\Qt\6.8.0\msvc2022_64\lib" Qt6Core.lib Qt6Gui.lib Qt6Widgets.lib /ENTRY:mainCRTStartup

