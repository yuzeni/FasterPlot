#include "faster_plot.hpp"

#include "data_manager.hpp"
#include "raylib.h"

#include "app_loop.hpp"
#include "global_vars.hpp"
#include "gui_elements.hpp"
#include "..\resources\font.hpp"
#include "utils.hpp"
#include "command_parser.hpp"

namespace FPlot {

    static Text_Input text_input;
    static Content_Tree content_tree;

    void Faster_Plot::init_window()
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
    }

    void Faster_Plot::run_until_close()
    {
	while (!WindowShouldClose()) {
	    app_loop(text_input, content_tree, flags);
	}
	CloseWindow();
    }
    
    void Faster_Plot::next_frame()
    {
	app_loop();
    }

    void Faster_Plot::enable_flags(Faster_Plot_flags flags) { this->flags = add_flag(this->flags, flags); }
    void Faster_Plot::disable_flags(Faster_Plot_flags flags) { this->flags = remove_flag(this->flags, flags); }

    void Faster_Plot::run_command(std::string cmd)
    {
	Lexer lexer;
	lexer.get_input() = cmd;
	lexer.tokenize();
	handle_command(lexer);
    }
    
    void Faster_Plot::update_data(size_t data_idx, size_t value_idx, double value) { data_manager.update_value_data(data_idx, value_idx, value); }
    void Faster_Plot::resize_data(size_t data_idx, size_t size, double fill_value) { data_manager.resize_data(data_idx, size, fill_value); }
    void Faster_Plot::append_data(size_t data_idx, double value) { data_manager.append_data(data_idx, value); }
}

#if !FASTER_PLOT_LIBRARY

int main()
{
    using namespace FPlot;
    Faster_Plot fplot_handle;
    fplot_handle.init_window();
    fplot_handle.enable_flags(Faster_Plot_flags(FPL_CONTENT_TREE | FPL_TEXT_INPUT));
    fplot_handle.run_until_close();
    return 0;
}

#endif
