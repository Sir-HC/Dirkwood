@echo off

mkdir ..\..\build
pushd ..\..\build
cl -Zi ..\dirkwood\code\win32_dirkwood.cpp user32.lib Gdi32.lib
popd
