#include "raylib.h"

#include "app_loop.hpp"
#include "global_vars.hpp"
#include "gui_elements.hpp"
#include "..\resources\font.hpp"

#if FASTER_PLOT_LIBRARY


#else

int main()
{
    // init raylib window
    SetTraceLogLevel(LOG_NONE);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Faster Plot");
    SetTargetFPS(TARGET_FPS);
    SetExitKey(KEY_NULL);

    // load font in different sizes
    g_app_font_18 = LoadFontFromMemory(".ttf", resources_Roboto_Regular_ttf, resources_Roboto_Regular_ttf_len, 18, nullptr, 0);
    g_app_font_20 = LoadFontFromMemory(".ttf", resources_Roboto_Regular_ttf, resources_Roboto_Regular_ttf_len, 20, nullptr, 0);
    g_app_font_22 = LoadFontFromMemory(".ttf", resources_Roboto_Regular_ttf, resources_Roboto_Regular_ttf_len, 22, nullptr, 0);

    Text_Input text_input;
    Content_Tree content_tree;

    while (!WindowShouldClose())
    {
	app_loop(text_input, content_tree);
    }
    
    CloseWindow();
    return 0;
}

#endif
