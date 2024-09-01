#pragma once

#include <cstddef>
#include <string>

namespace FPlot
{
    enum Faster_Plot_flags
    {
	FPL_NONE         = 0,
	FPL_TEXT_INPUT   = 1,
	FPL_CONTENT_TREE = 1 << 1,
    };

    class Faster_Plot
    {
    public:
	
	void init_window();
	void enable_flags(Faster_Plot_flags flags);
	void disable_flags(Faster_Plot_flags flags);
	void run_until_close();                                                // keeps the plot window open, until it is closed by the user.
	bool next_frame();                                                     // advance the window by one frame.
	void run_command(std::string cmd);                                     // run any command
	void update_data(size_t data_idx, size_t value_idx, double value);     // change a single value of a data object.
	void resize_data(size_t data_idx, size_t size, double fill_value = 0); // resize a data object.
	void append_data(size_t data_idx, double value);                       // add a new element at the end of a data object.
	
    private:
	
	Faster_Plot_flags flags = FPL_NONE;
    };
}
