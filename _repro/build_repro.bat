@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
cd /d F:\YiyangzaiCode\_repro
cmake -S . -B build -A x64 -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64\lib\cmake" >nul
cmake --build build --config Release
