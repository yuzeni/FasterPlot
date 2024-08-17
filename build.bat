@echo off
setlocal enabledelayedexpansion

set exe_name=faster_plot.exe
set defines
set include_paths=/I..\raylib50
set src_files=..\src\faster_plot.cpp ..\src\utils.cpp ..\src\functions.cpp ..\src\function_parsing.cpp ..\src\lexer.cpp ..\src\command_parser.cpp ..\src\object_operations.cpp ..\src\gui_elements.cpp ..\src\data_manager.cpp ..\src\app_loop.cpp ..\resources\font.cpp
set libs=gdi32.lib msvcrt.lib ..\raylib50\raylib.lib user32.lib shell32.lib winmm.lib
set CFlags=/O2 /EHsc /std:c++20

mkdir build
pushd build
del %exe_name%
cl %Cflags% %defines% %include_paths% %src_files% %libs% /link /libpath:..\raylib50
%exe_name%
popd

endlocal
