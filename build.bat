@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio 12.0\Common7\Tools\vsvars32.bat"

set CFLAGS=/EHsc /MD /Iwin32_deps\include
set LFLAGS=/SUBSYSTEM:CONSOLE /LIBPATH:win32_deps\lib SDL2main.lib SDL2.lib

cl %CFLAGS% main.cpp /link %LFLAGS% || exit /b 1

