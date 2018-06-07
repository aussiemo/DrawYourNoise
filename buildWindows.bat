@echo off
if "%DevEnvDir%"=="" (
    call "E:\Programme\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
)
cl /EHsc -D_USE_MATH_DEFINES main.cpp /link openal\libs\Win64\OpenAL32.lib
