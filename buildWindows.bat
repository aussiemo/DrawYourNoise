@echo off

REM Calls vcvarsall if it hasn't been called already.
if "%DevEnvDir%"=="" (
    call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\vcvarsall.bat" x64
)

cl /EHsc -D_USE_MATH_DEFINES main.cpp /link openal\libs\Win64\OpenAL32.lib
