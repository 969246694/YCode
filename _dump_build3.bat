@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cl /nologo /EHsc F:\YiyangzaiCode\_dump_ctx.cpp /Fe:F:\YiyangzaiCode\_dump_ctx.exe /link dbghelp.lib
F:\YiyangzaiCode\_dump_ctx.exe
