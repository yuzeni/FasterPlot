#include "app_loop.hpp" 

#include "raylib.h"

#include "data_manager.hpp"

void app_loop(Text_Input &text_input)
{
    if (!WindowShouldClose())
    {
	handle_dropped_files(data_manager);
	text_input.update(data_manager);
	data_manager.update();

	BeginDrawing();
	{
	    ClearBackground(WHITE);
	    data_manager.draw();
	    text_input.draw();
	}
	EndDrawing();
    }
    else {
	CloseWindow();
    }
}

void app_loop()
{
    if (!WindowShouldClose())
    {
	// handle_dropped_files(data_manager);
	// data_manager.update();

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
