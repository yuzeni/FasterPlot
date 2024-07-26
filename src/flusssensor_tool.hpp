#pragma once

#include <string>
#include <vector>

#include "function_fitting.hpp"

enum Plot_Type
{
    PT_DISCRETE = 1,
    PT_INTERP_LINEAR = 1 << 1,
};

struct Plot_Data
{
    std::string header;
    Color color = BLACK;
    Plot_Type plot_type = Plot_Type(PT_DISCRETE | PT_INTERP_LINEAR);
    float thickness = 4;
    
    std::vector<double> x;
    Plot_Data* y = nullptr;
    Sinusoidal_Function periodic_fit;
};

struct Coordinate_System
{
    Vec2<double> origin = {0, 0};
    Vec2<double> basis_x = {0, 0};
    Vec2<double> basis_y = {0, 0};
    
    void set_basis_normalized(Vec2<double> b_x, Vec2<double> b_y)
    {
	basis_x = b_x;
	basis_y = b_y;
	basis_x.normalize();
	basis_y.normalize();
    }

    Vec2<double> transform_to(Vec2<double> p, Coordinate_System to_sys) const
    {
	double t_p_x = (p.x * basis_x.x + p.y * basis_y.x - ((p.x * basis_x.y + p.y * basis_y.y) * to_sys.basis_y.x) / to_sys.basis_y.y)
	    / (to_sys.basis_x.x - (to_sys.basis_x.y * to_sys.basis_y.x) / to_sys.basis_y.y);
	double t_p_y = (p.x * basis_x.y + p.y * basis_y.y - t_p_x * to_sys.basis_x.y) / to_sys.basis_y.y;
	return {t_p_x + origin.x, t_p_y + origin.y};
    }
};

struct VP_Camera
{
    Coordinate_System coord_sys;

    bool is_undefined() { return coord_sys.basis_x.length() == 0 || coord_sys.basis_y.length() == 0; }
};

constexpr int graph_color_array_cnt = 21;
inline Color graph_color_array[graph_color_array_cnt] = {
    RED,
    YELLOW,
    GOLD,
    ORANGE,
    PINK,
    MAROON,
    GREEN,
    LIME,
    DARKGREEN,
    SKYBLUE,
    BLUE,
    DARKBLUE,
    PURPLE,
    VIOLET,
    DARKPURPLE,
    BEIGE,
    BROWN,
    DARKBROWN,
    LIGHTGRAY,
    GRAY,
    DARKGRAY,
};
