@echo off

mkdir ..\..\build
pushd ..\..\build
cl -MT -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -DINTERNAL=1 -DSLOWBUILD=1 -FC -Z7 -Fmwin32_dirkwood.map ..\dirkwood\code\win32_dirkwood.cpp user32.lib Gdi32.lib
popd
