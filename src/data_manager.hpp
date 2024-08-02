#pragma once

#include "raylib.h"

#include <string>
#include <vector>
#include <limits>

#include "function_fitting.hpp"
#include "gui_elements.hpp"

constexpr int graph_color_array_cnt = 20;
inline Color graph_color_array[graph_color_array_cnt] = {
    
    MAROON,
    GREEN,
    LIME,
    DARKGREEN,
    SKYBLUE,
    BLUE,
    DARKBLUE,
    RED,
    // YELLOW,
    GOLD,
    ORANGE,
    PINK,
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
    PT_SHOW_INDEX = 1 << 2,
};

struct Plot_Info
{
    std::string header;
    Color color = BLACK;
    Plot_Type plot_type = Plot_Type(PT_INTERP_LINEAR);
    float thickness = 4;
    bool visible = true;
};

struct Plot_Data
{
    Plot_Info info;
    std::vector<double> y;
    Content_Tree_Element content_element;
    std::vector<Plot_Data*> x_referencees;
    size_t index = 0;
    
    size_t size() const
    {
	if(x)
	    return std::min(y.size(), x->y.size());
	return y.size();
    };

    void update_content_tree_element(size_t index);
    Plot_Data* x = nullptr;
};

struct Function
{
    Plot_Info info;
    Function_Type type = FT_undefined;
    Content_Tree_Element content_element;
    Plot_Data* fit_from_data = nullptr;
    size_t index = 0;
    
    double operator()(double x) const
    {
	switch(type) {
	case FT_linear:   return func.linear(x);
	case FT_sinusoid: return func.sinusoid(x);
	}
	return 0;
    }

    std::string get_string_value() const
    {
	switch(type) {
	case FT_linear:   return func.linear.get_string_value();
	case FT_sinusoid: return func.sinusoid.get_string_value();
	}
	return "undefined function";
    }
    
    std::string get_string_no_value() const
    {
	switch(type) {
	case FT_linear:   return func.linear.get_string_no_value();
	case FT_sinusoid: return func.sinusoid.get_string_no_value();
	}
	return "undefined function";
    }

    bool is_defined() const
    {
	switch(type) {
	case FT_linear:   return func.linear.is_defined();
	case FT_sinusoid: return func.sinusoid.is_defined();
	}
	return false;
    }

    double get_parameter(std::string_view name) const
    {
	switch(type) {
	case FT_linear:   return func.linear.get_parameter(name);
	case FT_sinusoid: return func.sinusoid.get_parameter(name);
	}
	return std::numeric_limits<double>().quiet_NaN();
    }

    void fit_to_data(Plot_Data* plot_data, int iterations)
    {
	switch(type) {
	case FT_linear:   func.linear.get_fit_init_values(plot_data);
	case FT_sinusoid: func.sinusoid.get_fit_init_values(plot_data);
	}
	
	function_fit_iterative_naive(plot_data, *this, iterations);
    }

    double get_fit_parameter_change_rate(int idx)
    {
	switch(type) {
	case FT_linear:   return func.linear.get_fit_parameter_change_rate(idx);
	case FT_sinusoid: return func.sinusoid.get_fit_parameter_change_rate(idx);
	default: return 0;
	}
    }

    double* get_param_ref(int idx)
    {
	switch(type) {
	case FT_linear:   return func.linear.get_parameter_ref(idx);
	case FT_sinusoid: return func.sinusoid.get_parameter_ref(idx);
	default: return nullptr;
	}
    }

    void update_content_tree_element(size_t index);
    
    union {
	Undefined_Function undefined = Undefined_Function{};
	Linear_Function linear;
	Sinusoidal_Function sinusoid;
    } func;

    std::vector<double*> param_list;
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

    std::vector<Plot_Data*> plot_data;
    std::vector<Function*> functions;
    VP_Camera camera;
    const Vec2<int> plot_padding = {50, 50};

    void update();
    void draw();
    void load_external_plot_data(const std::string& file_name);
    Plot_Data* new_plot_data(Plot_Data* data = nullptr);
    void delete_plot_data(Plot_Data *data);
    Function* new_function();
    void delete_function(Function *function);
    void fit_camera_to_plot(Plot_Data* plot_data);
    void fit_camera_to_plot(Function* func);
    void zero_coord_sys_origin();
    void update_references();
    void export_plot_data(std::string file_name, std::vector<Plot_Data*>& plot_data);
    void export_functions(std::string file_name, std::vector<Function*>& functions);
    
private:

    // Plot_Data default_x;
    int graph_color_array_idx = 0;
    Content_Tree content_tree;
    const int key_board_lock_id;

    std::vector<Plot_Data*> original_plot_data;
    std::vector<Function*> original_functions;

    void copy_data_to_data(std::vector<Plot_Data*>& from_plot_data, std::vector<Plot_Data*>& to_plot_data,
			   std::vector<Function*>& from_functions, std::vector<Function*>& to_functions);

    void fit_camera_to_plot(bool go_to_zero = false);
    void draw_plot_data();
    void draw_functions();
    void update_element_indices();
    bool keyboard_access();

};

Plot_Data *get_new_default_x_for_plot_data(Plot_Data *plot_data);

void run_command_file(std::string file_name);
