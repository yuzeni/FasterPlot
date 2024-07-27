#include "flusssensor_tool.hpp"

#include "raylib.h"

#include <cmath>
#include <cstdio>
#include <vector>
#include <string>
#include <cstdint>
#include <iostream>

#include "utils.hpp"
#include "command_parser.hpp"
#include "function_fitting.hpp"

constexpr int TARGET_FPS = 60;
constexpr int COORDINATE_SYSTEM_GRID_SPACING = 60;
constexpr int COORDINATE_SYSTEM_FONT_SIZE = 18;
constexpr Color COORDINATE_SYSTEM_FONT_COLOR = Color{0, 0, 0, 150};
constexpr Color COORDINATE_SYSTEM_MAIN_AXIS_COLOR = Color{0, 0, 0, 255};
constexpr Color COORDINATE_SYSTEM_GRID_COLOR = Color{0, 0, 0, 75};
// Raylibs screen coordinate system has the origin in the top left corner.
// X points right and Y points down.
constexpr Coordinate_System app_coordinate_system = {{0, 0}, {1, 0}, {0, 1}};

int g_screen_width = 800;
int g_screen_height = 600;
Vector2 g_mouse_pos = Vector2{0, 0};
Vector2 g_mouse_delta = Vector2{0, 0};
Vector2 g_mouse_wheel = Vector2{0, 0};

static void update_global_app_vars()
{
    g_screen_width = GetScreenWidth();
    g_screen_height = GetScreenHeight();
    g_mouse_pos = GetMousePosition();
    g_mouse_delta = GetMouseDelta();
    g_mouse_wheel = GetMouseWheelMoveV();
}

static void draw_vp_camera_coordinate_system(VP_Camera camera, int target_spacing)
{
    int grid_resolution = std::max(g_screen_width, g_screen_height) / target_spacing;
    
    Vec2<double> axis_length = {ceil(std::max(g_screen_width, g_screen_height) / camera.coord_sys.basis_x.length()),
				ceil(std::max(g_screen_width, g_screen_height) / camera.coord_sys.basis_y.length())};
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
	DrawText(text_buffer, int(text_pos.x), int(text_pos.y), COORDINATE_SYSTEM_FONT_SIZE, COORDINATE_SYSTEM_FONT_COLOR);
	snprintf(text_buffer, 64, "%.4g", (t_grid_base_y.length() * double(i)) / camera.coord_sys.basis_y.length() - camera.origin_offset.y);
	text_pos = t_origin + t_grid_base_y * (double(i) + double(i == 0 ? 0.25 : 0));
	DrawText(text_buffer, int(text_pos.x), int(text_pos.y), COORDINATE_SYSTEM_FONT_SIZE, COORDINATE_SYSTEM_FONT_COLOR);
    }
}

void Data_Manager::draw_plot_data()
{
    // draw the plot
    for(const auto& pd : plot_data) {
	if (!pd->x)
	    continue;
	for(size_t ix = 0; ix < pd->size(); ++ix) {
	    Vec2<double> screen_space_point = camera.coord_sys.transform_to(Vec2<double>{pd->x->y[ix], pd->y[ix]} + camera.origin_offset, app_coordinate_system);
	    Vec2<double> prev_screen_space_point;
	    if(pd->info.plot_type & PT_DISCRETE) {
		DrawCircle(std::round(screen_space_point.x), std::round(screen_space_point.y), pd->info.thickness / 2.f, pd->info.color);
	    }
	    if(pd->info.plot_type & PT_INTERP_LINEAR) {
		if(ix > 0)
		    DrawLineEx(prev_screen_space_point, screen_space_point, pd->info.thickness / 3.f, pd->info.color);
	    }
	    prev_screen_space_point = screen_space_point;
	}


    }
}

void Data_Manager::draw_functions()
{
    for(const auto& func : functions) {
	if (!func->is_defined() || !func->x)
	    continue;
	for(size_t ix = 0; ix < func->size(); ++ix) {
	    Vec2<double> screen_space_point = camera.coord_sys.transform_to(Vec2<double>{func->x->y[ix], func->operator()(func->x->y[ix])} + camera.origin_offset,
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

Data_Manager::Data_Manager()
{
    camera.coord_sys.origin = {double(plot_padding.x), double(g_screen_height - plot_padding.y)};
}

Data_Manager::~Data_Manager()
{
    for (auto pd : plot_data)
	delete pd;
    for (auto f : functions)
	delete f;
}

void Data_Manager::new_plot_data(Plot_Data* data)
{
    if (data)
	plot_data.push_back(data);
    else
	plot_data.push_back(new Plot_Data);
    plot_data.back()->x = &default_x;
    plot_data.back()->info.color = graph_color_array[graph_color_array_idx];
    ++graph_color_array_idx;
    if (graph_color_array_idx >= graph_color_array_cnt)
	graph_color_array_idx = 0;	
}

void Data_Manager::new_function()
{
    functions.push_back(new Function);
    functions.back()->x = &default_x;
    functions.back()->info.color = graph_color_array[graph_color_array_idx];
    ++graph_color_array_idx;
    if (graph_color_array_idx >= graph_color_array_cnt)
	graph_color_array_idx = 0;	
}
    
void Data_Manager::add_plot_data(std::string file)
{
    std::vector<Plot_Data*> data_list = parse_numeric_csv_file(file);
    for(const auto data : data_list) {
	new_plot_data(data);
    }
}

void Data_Manager::draw()
{
    if (plot_data.empty())
	return;

    static int old_g_screen_height = g_screen_height;
    camera.coord_sys.origin.y += g_screen_height - old_g_screen_height;
    old_g_screen_height = g_screen_height;

    // update default x
    size_t max_x = 0;
    for (const auto pd : plot_data) {
	max_x = pd->y.size() > max_x ? pd->y.size() : max_x;
    }

    if (default_x.y.size() != max_x) {
	default_x.y.resize(max_x);
	for (size_t i = 0; i < default_x.y.size(); ++i)
	    default_x.y[i] = i;
    }

    if (camera.is_undefined())
	fit_camera_to_plot();
    
    // double required_size = app_coordinate_system.transform_to(Vec2<double>{double(g_screen_width), 0}, camera.coord_sys).x
    // 	- app_coordinate_system.transform_to(Vec2<double>{0, 0}, camera.coord_sys).x;
    // if (required_size > default_x.y.size()) {
    // 	default_x.y.resize(required_size * 1.25);
    // 	for (int i = 0; i < default_x.size(); ++i)
    // 	    default_x.y[i] = i;
    // }


    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
	DisableCursor();
	camera.coord_sys.origin.x += g_mouse_delta.x;
	camera.coord_sys.origin.y += g_mouse_delta.y;
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
	EnableCursor();
    }

    if (!IsKeyDown(KEY_LEFT_CONTROL)) {
	camera.coord_sys.basis_x = camera.coord_sys.basis_x + camera.coord_sys.basis_x * (g_mouse_wheel.y / 10.f);
    }
	
    if (!IsKeyDown(KEY_LEFT_SHIFT)) {
	camera.coord_sys.basis_y = camera.coord_sys.basis_y + camera.coord_sys.basis_y * (g_mouse_wheel.y / 10.f);
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
	Vec2<double> normalize = Vec2<double>{camera.coord_sys.basis_x.length(), -camera.coord_sys.basis_y.length()};
	camera.coord_sys.origin = camera.coord_sys.origin + camera.origin_offset * normalize;
	camera.origin_offset = (camera.coord_sys.origin - Vec2<double>{double(g_mouse_pos.x), double(g_mouse_pos.y)}) / normalize;
	camera.coord_sys.origin = camera.coord_sys.origin - camera.origin_offset * normalize;
		
    }
    
    if (IsKeyPressed(KEY_SPACE)) {
	Vec2<double> normalize = Vec2<double>{camera.coord_sys.basis_x.length(), -camera.coord_sys.basis_y.length()};
	camera.coord_sys.origin = camera.coord_sys.origin + camera.origin_offset * normalize;
	camera.origin_offset = {0, 0};
	camera.coord_sys.origin = {double(plot_padding.x), double(g_screen_height - plot_padding.y)};
	fit_camera_to_plot();
    }

    draw_vp_camera_coordinate_system(camera, COORDINATE_SYSTEM_GRID_SPACING);
    draw_plot_data();
    draw_functions();
}

void Data_Manager::fit_camera_to_plot()
{
    double max_x = 0;
    double min_x = 0;
    double max_y = 0;
    double min_y = 0;
	
    for(const auto& pd : plot_data) {
	if (!pd->x)
	    continue;
	for(double val : pd->x->y) {
	    max_x = val > max_x ? val : max_x;
	    min_x = val < min_x ? val : min_x;
	}
	for(double val : pd->y) {
	    max_y = val > max_y ? val : max_y;
	    min_y = val < min_y ? val : min_y;
	}
    }

    for(const auto& func : functions) {
	if (!func->x)
	    continue;
	for(double val : func->x->y) {
	    max_x = val > max_x ? val : max_x;
	    min_x = val < min_x ? val : min_x;
	    double func_y = func->operator()(val);
	    max_y = func_y > max_x ? func_y : max_x;
	    min_y = func_y < min_x ? func_y : min_x;
	}
    }
    
    camera.coord_sys.basis_x = { double(g_screen_width - plot_padding.x * 2) / (max_x - min_x), 0 };
    camera.coord_sys.basis_y = { 0 , -double(g_screen_height - plot_padding.y * 2) / (max_y - min_y)};
    
    // TODO: move camera origin to the right place
}
    
bool load_dropped_files(Data_Manager& fluss_daten) {
    if (IsFileDropped()) {
	FilePathList path_list = LoadDroppedFiles();
	for(uint32_t i = 0; i < path_list.count; ++i)
	    fluss_daten.add_plot_data(path_list.paths[i]);
	UnloadDroppedFiles(path_list);
	return true;
    }
    return false;
}

int main()
{
    SetTraceLogLevel(LOG_NONE);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(g_screen_width, g_screen_height, "Flusssensor Tool");
    SetTargetFPS(TARGET_FPS);

    Data_Manager data_manager;

    while (!WindowShouldClose()) {

	update_global_app_vars();
	load_dropped_files(data_manager);

	handle_command(data_manager);

	BeginDrawing();
	{
	    ClearBackground(WHITE);
	    data_manager.draw();
	}
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
