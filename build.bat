@echo off
mkdir build
pushd build
del .\faster_plot.exe
rem /Zi
rem /NODEFAULTLIB:libcmt /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup
cl /Zi /EHsc /std:c++20 /I..\raylib50 ..\src\faster_plot.cpp ..\src\utils.cpp ..\src\function_fitting.cpp ..\src\lexer.cpp ..\src\command_parser.cpp ..\src\object_operations.cpp ..\src\gui_elements.cpp ..\src\data_manager.cpp gdi32.lib msvcrt.lib ..\raylib50\raylib.lib user32.lib shell32.lib winmm.lib /link /libpath:..\raylib50
xcopy /e /i /y "..\resources" "resources"
.\faster_plot.exe
popd
