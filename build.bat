@echo off
mkdir build
pushd build
del .\flusssensor_tool.exe
rem /Zi
rem /NODEFAULTLIB:libcmt /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup
cl /Zi /EHsc /std:c++20 /I..\raylib50 ..\src\flusssensor_tool.cpp ..\src\utils.cpp ..\src\function_fitting.cpp ..\src\lexer.cpp ..\src\command_parser.cpp gdi32.lib msvcrt.lib ..\raylib50\raylib.lib user32.lib shell32.lib winmm.lib /link /libpath:..\raylib50
.\flusssensor_tool.exe
popd
