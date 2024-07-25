@echo off
mkdir build
pushd build
del .\FlusssensorTool.exe
rem /Zi
rem /NODEFAULTLIB:libcmt /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup
cl /Zi /EHsc /std:c++20 /I..\raylib50 ..\src\FlusssensorTool.cpp ..\src\utils.cpp gdi32.lib msvcrt.lib ..\raylib50\raylib.lib user32.lib shell32.lib winmm.lib /link /libpath:..\raylib50
.\FlusssensorTool.exe
popd
