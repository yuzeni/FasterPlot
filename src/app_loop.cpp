#include "app_loop.hpp" 

#include "raylib.h"

#include "data_manager.hpp"
#include "faster_plot.hpp"
#include "utils.hpp"

// primary application loop
void app_loop(Text_Input &text_input, Content_Tree& content_tree, FPlot::Faster_Plot_flags flags)
{
    using namespace FPlot;
    
    if (!WindowShouldClose())
    {
	handle_dropped_files();
	
	if (check_flag(flags, FPL_TEXT_INPUT)) {
	    text_input.update();
	}

	data_manager.update_viewport();
	
	if (check_flag(flags, FPL_CONTENT_TREE)) {
	    data_manager.update_content_tree(content_tree);
	}

	BeginDrawing();
	{
	    ClearBackground(WHITE);
	    data_manager.draw();
	    
	    if (check_flag(flags, FPL_TEXT_INPUT)) {
		text_input.draw();
	    }
	    
	    if (check_flag(flags, FPL_CONTENT_TREE)) {
		content_tree.draw();
	    }
	}
	EndDrawing();
    }
    else {
	CloseWindow();
    }
}

// Secondary loop, for exclusively updatting the visuals.
// The loop is used for showing data changes live, during computation, with an arbitary call rate.
void app_loop()
{
    if (!WindowShouldClose())
    {
	data_manager.update_viewport();
	BeginDrawing();
	{
	    ClearBackground(WHITE);
	    data_manager.draw();
	}
	EndDrawing();
    }
    else {
	CloseWindow();
    }
}
