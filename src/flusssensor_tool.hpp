#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "function_fitting.hpp"
#include "utils.hpp"

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

enum Plot_Type
{
    PT_DISCRETE = 1,
    PT_INTERP_LINEAR = 1 << 1,
};

struct Plot_Info
{
    std::string header;
    Color color = BLACK;
    Plot_Type plot_type = Plot_Type(PT_INTERP_LINEAR);
    float thickness = 4;
};

struct Plot_Data
{
    Plot_Info info;
    std::vector<double> y;
    Plot_Data* x = nullptr;
    
    size_t size()
    {
	if(x)
	    return std::min(y.size(), x->y.size());
	return y.size();
    };
};

struct Function
{
    Plot_Info info;
    Function_Type type = FT_undefined;
    Plot_Data* x = nullptr;
    
    double operator()(double x) const
    {
	switch(type) {
	case FT_linear:   return func.linear(x);
	case FT_sinusoid: return func.sinusoid(x);
	}
	return 0;
    }

    bool is_defined() const
    {
	switch(type) {
	case FT_linear:   return func.linear.is_defined();
	case FT_sinusoid: return func.sinusoid.is_defined();
	}
	return false;
    }

    size_t size()
    {
	if(x)
	    return x->y.size();
	return 0;
    };
    
    union {
	Undefined_Function undefined = Undefined_Function{};
	Linear_Function linear;
	Sinusoidal_Function sinusoid;
    } func;
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
    Vec2<double> origin_offset = {0, 0};

    bool is_undefined() { return coord_sys.basis_x.length() == 0 || coord_sys.basis_y.length() == 0; }
};

struct Data_Manager
{
    Data_Manager();
    ~Data_Manager();

    void add_plot_data(std::string file = "../test_eingabe/simple.csv");
    void draw();
    
    std::vector<Plot_Data*> plot_data;
    std::vector<Function*> functions;
    VP_Camera camera;
    const Vec2<int> plot_padding = {50, 50};

    void new_plot_data(Plot_Data* data = nullptr);
    void new_function();
    
private:

    Plot_Data default_x;
    int graph_color_array_idx = 0;
    
    void fit_camera_to_plot();
    void draw_plot_data();
    void draw_functions();
};

