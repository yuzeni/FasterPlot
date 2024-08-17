#pragma once

#include "raylib.h"

#include <string>
#include <vector>

#include "gui_elements.hpp"
#include "functions.hpp"

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

    void update_viewport();
    void update_content_tree(Content_Tree& content_tree);
    void draw();
    void load_external_plot_data(const std::string& file_name);
    Plot_Data* new_plot_data(Plot_Data* data = nullptr);
    void delete_plot_data(Plot_Data *data);
    Function* new_function();
    void delete_function(Function *function);
    Function* change_function_type(Function *orig_func, Function* new_func);
    void fit_camera_to_plot(Plot_Data* plot_data);
    void fit_camera_to_plot(Function* func);
    void zero_coord_sys_origin();
    void update_references();
    void export_plot_data(std::string file_name, std::vector<Plot_Data*>& plot_data);
    void export_functions(std::string file_name, std::vector<Function*>& functions);
    void revert_command();
    void revert_reverting();
    
    void update_value_data(size_t data_idx, size_t value_idx, double value)
    {
	if (data_idx < plot_data.size()) {
	    plot_data[data_idx]->y.at(value_idx) = value;
	}
    }
    
    void resize_data(size_t data_idx, size_t size, double fill_value)
    {
	if (data_idx < plot_data.size()) {
	    plot_data[data_idx]->y.resize(size, fill_value);
	}
    }
    
    void append_data(size_t data_idx, double value)
    {
	plot_data[data_idx]->y.push_back(value);
    }
    
private:

    int graph_color_array_idx = 0;
    const int key_board_lock_id;

    std::vector<Plot_Data*> original_plot_data;
    std::vector<Function*> original_functions;
    int original_graph_color_array_idx = 0;

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

inline Data_Manager data_manager; // I love singletons
