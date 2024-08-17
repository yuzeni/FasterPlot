#include "app_loop.hpp" 

#include "raylib.h"

#include "data_manager.hpp"

// primary application loop
void app_loop(Text_Input &text_input, Content_Tree& content_tree)
{
    if (!WindowShouldClose())
    {
	handle_dropped_files(data_manager);
	text_input.update(data_manager);
	data_manager.update(content_tree);

	BeginDrawing();
	{
	    ClearBackground(WHITE);
	    data_manager.draw();
	    text_input.draw();
	    content_tree.draw();
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
