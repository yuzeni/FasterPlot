#pragma once

namespace FPlot
{

    enum FPlotter_flags {
	FPL_TEXT_INPUT   = 1,
	FPL_CONTENT_TREE = 1 << 1,
    };

    class FPlotter
    {
	void init_window(FPlotter_flags flags);
	
	
    };

}
