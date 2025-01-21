@echo off
pushd out
cl /nologo /std:c++20 -Zi ..\src\main.cpp user32.lib gdi32.lib
popd
