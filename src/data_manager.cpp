#include "data_manager.hpp"

#include <cstddef>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>

#include "global_vars.hpp"
#include "raylib.h"
#include "utils.hpp"
#include "command_parser.hpp"

// coordinate system
constexpr int COORDINATE_SYSTEM_GRID_SPACING = 60;
constexpr int COORDINATE_SYSTEM_FONT_SIZE = 18;
constexpr Font* COORDINATE_SYSTEM_FONT = &g_app_font_18;
constexpr Color COORDINATE_SYSTEM_FONT_COLOR = Color{0, 0, 0, 150};
constexpr Color COORDINATE_SYSTEM_MAIN_AXIS_COLOR = Color{0, 0, 0, 255};
constexpr Color COORDINATE_SYSTEM_GRID_COLOR = Color{0, 0, 0, 75};
constexpr Coordinate_System app_coordinate_system = {{0, 0}, {1, 0}, {0, 1}};

// plot data
constexpr int PLOT_DATA_FONT_SIZE = 18;
constexpr Font *PLOT_DATA_FONT = &g_app_font_18;

#define EXPORT_FILE_TYPE ".txt"
#define EXPORT_DIRECTORY "exports/"

void Plot_Data::update_content_tree_element(size_t index)
{
    content_element.name = "data " + std::to_string(index) + (!info.header.empty() ? " '" + info.header + "'" : "");
    if (!x_referencees.empty()) {
	content_element.name += " | X of data " + std::to_string(x_referencees[0]->index);
	for (size_t i = 1; i < x_referencees.size(); ++i) {
	    content_element.name += ", " + std::to_string(x_referencees[i]->index);
	}
    }
    content_element.name_color = info.color;
    content_element.content.clear();
    if (info.visible)
	content_element.content.push_back({"visible"});
    else
	content_element.content.push_back({"hidden"});
    if (x) {
	content_element.content.push_back({"X = "});
	content_element.content.push_back({x->content_element.name, false, x->info.color});
    }
    content_element.content.push_back({"size = " + std::to_string(y.size())});
}

void Function::update_content_tree_element(size_t index)
{
    content_element.name = "function " + std::to_string(index) + (!info.header.empty() ? " '" + info.header + "'" : "");

    content_element.name += " (x) = " + get_string_no_value();
	
    content_element.name_color = info.color;
    content_element.content.clear();
    
    if (!is_defined())
	content_element.content.push_back({"f(x) = " + get_string_no_value()});
    else
	content_element.content.push_back({"f(x) = " + get_string_value()});
    
    if (info.visible)
	content_element.content.push_back({"visible"});
    else
	content_element.content.push_back({"hidden"});
    if (fit_from_data) {
	content_element.content.push_back({"fit of "});
	content_element.content.push_back({fit_from_data->content_element.name, false, fit_from_data->info.color});
    }
}

static void draw_vp_camera_coordinate_system(VP_Camera camera, int target_spacing)
{
    int grid_resolution = std::max(GetScreenWidth(), GetScreenHeight()) / target_spacing;
    
    Vec2<double> axis_length = {ceil(std::max(GetScreenWidth(), GetScreenHeight()) / camera.coord_sys.basis_x.length()),
				ceil(std::max(GetScreenWidth(), GetScreenHeight()) / camera.coord_sys.basis_y.length())};
    Vec2<double> t_origin = camera.coord_sys.origin;
    Vec2<double> t_grid_base_x = camera.coord_sys.basis_x * (axis_length.x / double(grid_resolution));
    Vec2<double> t_grid_base_y = camera.coord_sys.basis_y * (axis_length.y / double(grid_resolution));

    char text_buffer[64];
    for(int i = -grid_resolution; i < grid_resolution; ++i) {
	Color color = i == 0 ? COORDINATE_SYSTEM_MAIN_AXIS_COLOR : COORDINATE_SYSTEM_GRID_COLOR;
	DrawLineEx(t_origin + t_grid_base_y * grid_resolution + t_grid_base_x * i,
		   t_origin + t_grid_base_y * grid_resolution * -1 + t_grid_base_x * i, 1, color);
	DrawLineEx(t_origin + t_grid_base_x * grid_resolution + t_grid_base_y * i,
		   t_origin + t_grid_base_x * grid_resolution * -1 + t_grid_base_y * i, 1, color);
	
	snprintf(text_buffer, 64, "%.4g", (t_grid_base_x.length() * double(i)) / camera.coord_sys.basis_x.length() - camera.origin_offset.x);
	Vec2<double> text_pos = t_origin + t_grid_base_x * double(i);
	DrawTextEx(*COORDINATE_SYSTEM_FONT, text_buffer, Vector2{float(text_pos.x), float(text_pos.y)}, COORDINATE_SYSTEM_FONT_SIZE, 0, COORDINATE_SYSTEM_FONT_COLOR);
	snprintf(text_buffer, 64, "%.4g", (t_grid_base_y.length() * double(i)) / camera.coord_sys.basis_y.length() - camera.origin_offset.y);
	text_pos = t_origin + t_grid_base_y * (double(i) + double(i == 0 ? 0.25 : 0));
	DrawTextEx(*COORDINATE_SYSTEM_FONT, text_buffer, Vector2{float(text_pos.x), float(text_pos.y)}, COORDINATE_SYSTEM_FONT_SIZE, 0, COORDINATE_SYSTEM_FONT_COLOR);
    }
}

void Data_Manager::draw_plot_data()
{
    for(const auto& pd : plot_data)
    {
	if (!pd->info.visible)
	    continue;
	
	for(size_t ix = 0; ix < pd->size(); ++ix)
	{
	    Vec2<double> screen_space_point = camera.coord_sys.transform_to(Vec2<double>{pd->x ? pd->x->y[ix] : double(ix), pd->y[ix]} + camera.origin_offset, app_coordinate_system);
	    Vec2<double> prev_screen_space_point;
	    if(pd->info.plot_type & PT_DISCRETE) {
		DrawCircle(std::round(screen_space_point.x), std::round(screen_space_point.y), pd->info.thickness / 2.f, pd->info.color);
	    }
	    if(pd->info.plot_type & PT_INTERP_LINEAR) {
		if(ix > 0)
		    DrawLineEx(prev_screen_space_point, screen_space_point, pd->info.thickness / 3.f, pd->info.color);
	    }
	    if(pd->info.plot_type & PT_SHOW_INDEX) {
		DrawTextEx(*PLOT_DATA_FONT, std::to_string(ix).c_str(), Vector2{float(screen_space_point.x + 2.0), float(screen_space_point.y + 2.0)},
			   PLOT_DATA_FONT_SIZE, 0, pd->info.color);
	    }
	    prev_screen_space_point = screen_space_point;
	}
    }
}

void Data_Manager::draw_functions()
{
    for(const auto& func : functions)
    {
	if (!func->is_defined() || !func->info.visible)
	    continue;
	
	for(size_t ix = 0; ix < size_t(GetScreenWidth()); ix += 1)
	{
	    double camera_space_x = app_coordinate_system.transform_to(Vec2<double>{double(ix), 0} - camera.coord_sys.origin, camera.coord_sys).x - camera.origin_offset.x;
	    Vec2<double> screen_space_point = camera.coord_sys.transform_to(Vec2<double>{camera_space_x, func->operator()(camera_space_x)} + camera.origin_offset,
									    app_coordinate_system);
	    Vec2<double> prev_screen_space_point;
	    if(func->info.plot_type & PT_DISCRETE) {
		DrawCircle(std::round(screen_space_point.x), std::round(screen_space_point.y), func->info.thickness / 2.f, func->info.color);
	    }
	    if(func->info.plot_type & PT_INTERP_LINEAR) {
		if(ix > 0)
		    DrawLineEx(prev_screen_space_point, screen_space_point, func->info.thickness / 3.f, func->info.color);
	    }
	    prev_screen_space_point = screen_space_point;
	}
    }
}

Data_Manager::Data_Manager() : key_board_lock_id(get_uuid())
{
    camera.coord_sys.origin = {double(plot_padding.x), double(GetScreenHeight() - plot_padding.y)};
    camera.coord_sys.basis_x = {1, 0};
    camera.coord_sys.basis_y = {0, -1};
}

Data_Manager::~Data_Manager()
{
    for (auto pd : plot_data)
	delete pd;
    for (auto f : functions)
	delete f;
    for (auto pd : original_plot_data)
	delete pd;
    for (auto f : original_functions)
	delete f;
}

Plot_Data* Data_Manager::new_plot_data(Plot_Data *data)
{
    if (data)
	plot_data.push_back(data);
    else
	plot_data.push_back(new Plot_Data);
    
    plot_data.back()->info.color = graph_color_array[graph_color_array_idx];
    plot_data.back()->index = plot_data.size() - 1;
    ++graph_color_array_idx;
    if (graph_color_array_idx >= graph_color_array_cnt)
	graph_color_array_idx = 0;

    return plot_data.back();
}

void Data_Manager::delete_plot_data(Plot_Data *data)
{
    // resolve reference
    for (size_t i = 0; i < data->x_referencees.size(); ++i) {
	data->x_referencees[i]->x = nullptr;
    }
    
    plot_data.erase(plot_data.begin() + data->index);
    delete data;

    update_element_indices();
}

Function* Data_Manager::new_function()
{
    functions.push_back(new Function);
    
    functions.back()->info.color = graph_color_array[graph_color_array_idx];
    functions.back()->index = functions.size() - 1;
    ++graph_color_array_idx;
    if (graph_color_array_idx >= graph_color_array_cnt)
	graph_color_array_idx = 0;
    
    return functions.back();
}

void Data_Manager::delete_function(Function *func)
{
    functions.erase(functions.begin() + func->index);
    delete func;

    update_element_indices();
}

void Data_Manager::copy_data_to_data(std::vector<Plot_Data*>& from_plot_data, std::vector<Plot_Data*>& to_plot_data,
				     std::vector<Function*>& from_functions, std::vector<Function*>& to_functions)
{
    for(size_t i = 0; i < to_plot_data.size(); ++i) {
	delete to_plot_data[i];
    }

    for(size_t i = 0; i < to_functions.size(); ++i) {
	delete to_functions[i];
    }
    
    to_plot_data.clear();
    to_functions.clear();
    to_plot_data.resize(from_plot_data.size(), nullptr);
    to_functions.resize(from_functions.size(), nullptr);

    for(size_t i = 0; i < from_plot_data.size(); ++i) {
        to_plot_data[i] = new Plot_Data;
	*to_plot_data[i] = *from_plot_data[i];
    }

    for(size_t i = 0; i < from_functions.size(); ++i) {
        to_functions[i] = new Function;
	*to_functions[i] = *from_functions[i];
    }
}

void Data_Manager::load_external_plot_data(const std::string& file_name)
{
    std::vector<Plot_Data*> data_list = parse_numeric_csv_file(file_name);
    for(const auto data : data_list) {
	new_plot_data(data);
    }
    fit_camera_to_plot();

    copy_data_to_data(plot_data, original_plot_data, functions, original_functions);

    logger.log_info("Loaded file '%s'.", file_name.c_str());
    if (g_all_commands.has_commands()) {
	g_all_commands.clear();
	logger.log_info(" and" UTILS_BRIGHT_RED " cleared the command list." UTILS_END_COLOR);
    }
    logger.log_info("\n");
}

bool Data_Manager::keyboard_access()
{
    return g_keyboard_lock == 0 || g_keyboard_lock == key_board_lock_id;
}

void Data_Manager::update()
{
    static int old_g_screen_height = GetScreenHeight();
    camera.coord_sys.origin.y += GetScreenHeight() - old_g_screen_height;
    old_g_screen_height = GetScreenHeight();

    if (camera.is_undefined())
	fit_camera_to_plot();



    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
	Vector2 mouse_delta = GetMouseDelta();
	camera.coord_sys.origin.x += mouse_delta.x;
	camera.coord_sys.origin.y += mouse_delta.y;
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
	Vec2<double> normalize = Vec2<double>{camera.coord_sys.basis_x.length(), -camera.coord_sys.basis_y.length()};
	camera.coord_sys.origin = camera.coord_sys.origin + camera.origin_offset * normalize;
	camera.origin_offset = (camera.coord_sys.origin - Vec2<double>{double(GetMousePosition().x), double(GetMousePosition().y)}) / normalize;
	camera.coord_sys.origin = camera.coord_sys.origin - camera.origin_offset * normalize;
    }

    if (!IsKeyDown(KEY_LEFT_CONTROL)) {
	camera.coord_sys.basis_x = camera.coord_sys.basis_x + camera.coord_sys.basis_x * (GetMouseWheelMoveV().y / 10.f);
    }
	
    if (!IsKeyDown(KEY_LEFT_SHIFT)) {
	camera.coord_sys.basis_y = camera.coord_sys.basis_y + camera.coord_sys.basis_y * (GetMouseWheelMoveV().y / 10.f);
    }

    if (keyboard_access())
    {
	if (IsKeyPressed(KEY_SPACE) && !g_keyboard_lock) {
	    fit_camera_to_plot();
	}
	
	if (IsKeyDown(KEY_LEFT_CONTROL)) {
	    if ((IsKeyPressed(KEY_Z) && g_all_commands.decr_command_idx()) ||
		(IsKeyPressed(KEY_Y) && g_all_commands.incr_command_idx()))
	    {
		if(IsKeyPressed(KEY_Z))
		    logger.log_info("Revert command.\n");
		if(IsKeyPressed(KEY_Y))
		    logger.log_info("Revert reverting.\n");

		copy_data_to_data(original_plot_data, plot_data, original_functions, functions);
		re_run_all_commands(*this);
	    }
	}
    }

    content_tree.clear();
    
    for (size_t i = 0; i < plot_data.size(); ++i) {
	plot_data[i]->update_content_tree_element(i);
	content_tree.add_element(&plot_data[i]->content_element);
    }
    
    for (size_t i = 0; i < functions.size(); ++i) {
	functions[i]->update_content_tree_element(i);
	content_tree.add_element(&functions[i]->content_element);
    }
}

void Data_Manager::draw()
{
    draw_vp_camera_coordinate_system(camera, COORDINATE_SYSTEM_GRID_SPACING);
    draw_plot_data();
    draw_functions();
    content_tree.draw();
}

void Data_Manager::zero_coord_sys_origin()
{
    fit_camera_to_plot(true);
}

void Data_Manager::fit_camera_to_plot(bool go_to_zero)
{
    // if (plot_data.empty() && functions.empty())
    // 	return;

    camera.coord_sys.origin = {double(plot_padding.x), double(GetScreenHeight() - plot_padding.y)};
    
    double max_x = -HUGE_VAL, max_y = -HUGE_VAL, min_x = HUGE_VAL, min_y = HUGE_VAL;

    if (go_to_zero) {
	max_x = 0, max_y = 0, min_x = 0, min_y = 0;
    }
	
    for(const auto& pd : plot_data) {
	if (!pd->info.visible)
	    continue;

	if (pd->x) {
	    for(double val : pd->x->y) {
		max_x = val > max_x ? val : max_x;
		min_x = val < min_x ? val : min_x;
	    }
	}
	else {
	    for(size_t i = 0; i < pd->size(); ++i) {
		max_x = i > max_x ? i : max_x;
		min_x = i < min_x ? i : min_x;
	    }	    
	}
	
	for(double val : pd->y) {
	    max_y = val > max_y ? val : max_y;
	    min_y = val < min_y ? val : min_y;
	}
    }

    for(const auto& func : functions) {
	if (!func->info.visible)
	    continue;

	for(size_t ix = 0; ix < size_t(GetScreenWidth()); ++ix)
	{
	    double camera_space_x = app_coordinate_system.transform_to(Vec2<double>{double(ix), 0} - camera.coord_sys.origin, camera.coord_sys).x - camera.origin_offset.x;
	    double func_y = func->operator()(camera_space_x);
	    max_y = func_y > max_y ? func_y : max_y;
	    min_y = func_y < min_y ? func_y : min_y;
	}
    }

    if (max_x != -HUGE_VAL && min_x != HUGE_VAL && max_x != min_x) {
	camera.coord_sys.basis_x = { double(GetScreenWidth() - plot_padding.x * 2) / (max_x - min_x), 0 };
	camera.origin_offset.x = -min_x;
    }
    else {
	camera.coord_sys.basis_x = { 1, 0 };
	camera.origin_offset.x = 0;
    }
    
    if (max_y != -HUGE_VAL && min_y != HUGE_VAL &&  max_y != min_y) {
	camera.coord_sys.basis_y = { 0, -double(GetScreenHeight() - plot_padding.y * 2) / (max_y - min_y)};
	camera.origin_offset.y = -min_y;
    }
    else {
	camera.coord_sys.basis_y = { 0, -1};
	camera.origin_offset.x = 0;
    }

    if (go_to_zero) {
	camera.origin_offset = {0, 0};
    }
}

void Data_Manager::fit_camera_to_plot(Plot_Data *plot_data)
{
    camera.coord_sys.origin = {double(plot_padding.x), double(GetScreenHeight() - plot_padding.y)};
    
    double max_x = -HUGE_VAL, max_y = -HUGE_VAL, min_x = HUGE_VAL, min_y = HUGE_VAL;

    if (plot_data->x) {
	for(double val : plot_data->x->y) {
	    max_x = val > max_x ? val : max_x;
	    min_x = val < min_x ? val : min_x;
	}
    }
    else {
	for(size_t i = 0; i < plot_data->size(); ++i) {
	    max_x = i > max_x ? i : max_x;
	    min_x = i < min_x ? i : min_x;
	}	    
    }
    
    for(double val : plot_data->y) {
	max_y = val > max_y ? val : max_y;
	min_y = val < min_y ? val : min_y;
    }
    
    camera.coord_sys.basis_x = { double(GetScreenWidth() - plot_padding.x * 2) / (max_x - min_x), 0 };
    camera.coord_sys.basis_y = { 0 , -double(GetScreenHeight() - plot_padding.y * 2) / (max_y - min_y)};
    camera.origin_offset = { -min_x, -min_y };
}

void Data_Manager::fit_camera_to_plot(Function* func)
{
    double max_y = -HUGE_VAL, min_y = HUGE_VAL;

    for(size_t ix = 0; ix < size_t(GetScreenWidth()); ++ix)
    {
	double camera_space_x = app_coordinate_system.transform_to(Vec2<double>{double(ix), 0} - camera.coord_sys.origin, camera.coord_sys).x - camera.origin_offset.x;
	double func_y = func->operator()(camera_space_x);
	max_y = func_y > max_y ? func_y : max_y;
	min_y = func_y < min_y ? func_y : min_y;
    }
    
    camera.coord_sys.basis_y = { 0 , -double(GetScreenHeight() - plot_padding.y * 2) / (max_y - min_y)};
    camera.origin_offset.y = -min_y;
}

// quadratic, only use when actually necessary
void Data_Manager::update_references()
{
    for (size_t i = 0; i < plot_data.size(); ++i) {
	plot_data[i]->x_referencees.clear();
    }
    
    for (size_t i = 0; i < plot_data.size(); ++i) {
	// x-axis references
	if (plot_data[i]->x) {
	    plot_data[i]->x->x_referencees.push_back(plot_data[i]);
	}
    }
}

// quadratic, only use when actually necessary
void Data_Manager::update_element_indices()
{
    for (size_t i = 0; i < plot_data.size(); ++i) {
	plot_data[i]->index = i;
    }
    
    for (size_t i = 0; i < functions.size(); ++i) {
        functions[i]->index = i;
    }
}

static bool get_valid_file_name_and_ensure_directory(std::string& file_name)
{
    if (!(std::filesystem::exists(EXPORT_DIRECTORY))) {
        if (!(std::filesystem::create_directory(EXPORT_DIRECTORY))) {
	    logger.log_error("Failed to create directory '%s', please create it manually.", EXPORT_DIRECTORY);
	    return false;
	}
    }
    
    file_name = EXPORT_DIRECTORY + file_name;
    std::string orig_file_name = file_name;
    file_name += EXPORT_FILE_TYPE;
    
    int file_idx = 1;
    while (file_exists(file_name)) {
	file_name = orig_file_name + "(" + std::to_string(file_idx) + ")" EXPORT_FILE_TYPE;
	++file_idx;
    }
    return true;
}

void Data_Manager::export_plot_data(std::string file_name, std::vector<Plot_Data*>& plot_data)
{
    if (!get_valid_file_name_and_ensure_directory(file_name))
	return;
    
    std::ofstream out_file;
    out_file.open(file_name);
    if (out_file.is_open()) {

	size_t max_size = 0;
	for (auto pd : plot_data) {
	    max_size = pd->size() > max_size ? pd->size() : max_size;
	}
	
	for (size_t i = 0; i < max_size; ++i) {
	    size_t pd_i = 0;
	    for (; pd_i < plot_data.size() - 1; ++pd_i) {
		if (plot_data[pd_i]->size() > i) {
		    out_file << std::to_string(plot_data[pd_i]->y[i]) << ", ";
		}
	    }
	    if (plot_data[pd_i]->size() > i) {
		out_file << std::to_string(plot_data[pd_i]->y[i]) << '\n';
	    }
	}
	out_file.close();
    }
    else {
	logger.log_error("Unable to open file '%s'", file_name.c_str());
    }
}

void Data_Manager::export_functions(std::string file_name, std::vector<Function*>& functions)
{
    if (!get_valid_file_name_and_ensure_directory(file_name))
	return;
    
    std::ofstream out_file;
    out_file.open(file_name);
    if (out_file.is_open()) {

	for (size_t fi = 0; fi < functions.size(); ++fi) {
	    out_file << functions[fi]->content_element.name << "\n";
	    out_file << functions[fi]->get_string_value() << "\n\n";
	}
	out_file.close();
    }
    else {
	logger.log_error("Unable to open file '%s'", file_name.c_str());
    }
}

Plot_Data *get_new_default_x_for_plot_data(Plot_Data *plot_data)
{
    Plot_Data* new_x = new Plot_Data;
    new_x->y.resize(plot_data->y.size());
    for (size_t i = 0; i < new_x->size(); ++i) {
	new_x->y[i] = i;
    }
    return new_x;
}
