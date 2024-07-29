#include "raylib.h"

#include <cstdint>

#include "command_parser.hpp"
#include "global_vars.hpp"
#include "data_manager.hpp"
#include "gui_elements.hpp"

constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 600;
constexpr int TARGET_FPS = 60;
     
static bool load_dropped_files(Data_Manager& fluss_daten) {
    if (IsFileDropped()) {
	FilePathList path_list = LoadDroppedFiles();
	for(uint32_t i = 0; i < path_list.count; ++i)
	    fluss_daten.add_plot_data(path_list.paths[i]);
	UnloadDroppedFiles(path_list);
	return true;
    }
    return false;
}

int main()
{
    SetTraceLogLevel(LOG_NONE);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Flusssensor Tool");
    SetTargetFPS(TARGET_FPS);
    SetExitKey(KEY_NULL);

    g_app_font_18 = LoadFontEx("resources/Roboto-Regular.ttf", 18, nullptr, 0);
    g_app_font_20 = LoadFontEx("resources/Roboto-Regular.ttf", 20, nullptr, 0);
    g_app_font_22 = LoadFontEx("resources/Roboto-Regular.ttf", 22, nullptr, 0);

    Data_Manager data_manager;
    Text_Input text_input;

    while (!WindowShouldClose()) {

	load_dropped_files(data_manager);

	text_input.update(data_manager);

	BeginDrawing();
	{
	    ClearBackground(WHITE);
	    data_manager.draw();
	    text_input.draw();
	}
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
