@echo off
setlocal enabledelayedexpansion

set defines=/D FASTER_PLOT_LIBRARY=1
set include_paths=/I..\raylib50
set src_files=..\src\faster_plot.cpp ..\src\utils.cpp ..\src\functions.cpp ..\src\function_parsing.cpp ..\src\lexer.cpp ..\src\command_parser.cpp ..\src\object_operations.cpp ..\src\gui_elements.cpp ..\src\data_manager.cpp ..\src\app_loop.cpp ..\resources\font.cpp
set obj_files=faster_plot.obj utils.obj functions.obj function_parsing.obj lexer.obj command_parser.obj object_operations.obj gui_elements.obj data_manager.obj app_loop.obj font.obj
set libs=gdi32.lib msvcrt.lib ..\raylib50\raylib.lib user32.lib shell32.lib winmm.lib
set CFlags=/O2 /EHsc /std:c++20 /c

mkdir build
pushd build
cl %Cflags% %defines% %include_paths% %src_files%
lib /out:faster_plot.lib %obj_files% %libs% /libpath:..\raylib50
popd

endlocal
