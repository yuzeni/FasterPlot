#include "raylib.h"

#include <cmath>
#include <cstdio>
#include <vector>
#include <string>
#include <cstdint>
#include <iostream>

#include "utils.hpp"

constexpr int TARGET_FPS = 60;
constexpr int COORDINATE_SYSTEM_GRID_SPACING = 60;
constexpr int COORDINATE_SYSTEM_FONT_SIZE = 18;
constexpr Color COORDINATE_SYSTEM_FONT_COLOR = Color{0, 0, 0, 150};
constexpr Color COORDINATE_SYSTEM_MAIN_AXIS_COLOR = Color{0, 0, 0, 255};
constexpr Color COORDINATE_SYSTEM_GRID_COLOR = Color{0, 0, 0, 75};

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

enum Plot_Type
{
    PT_DISCRETE = 1,
    PT_INTERP_LINEAR = 1 << 1,
};

struct Data_Plot
{
    std::vector<double>* data;
    Color color;
    Plot_Type plot_type;
    float thickness = 6;
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
    // Vec2<double> pos = {0, 0};
    // Vec2<double> vp_size = {0, 0};
    Coordinate_System coord_sys;

    bool is_undefined() { return coord_sys.basis_x.length() == 0 || coord_sys.basis_y.length() == 0; }
};

// Raylibs screen coordinate system has the origin in the top left corner.
// X points right and Y points down.
constexpr Coordinate_System app_coordinate_system = {{0, 0}, {1, 0}, {0, 1}};

static void draw_coordinate_system(Coordinate_System coord_sys, int target_spacing)
{
    int grid_resolution = std::max(g_screen_width, g_screen_height) / target_spacing;
    
    Vec2<double> axis_length = {ceil(std::max(g_screen_width, g_screen_height) / coord_sys.basis_x.length()),
				ceil(std::max(g_screen_width, g_screen_height) / coord_sys.basis_y.length())};
    Vec2<double> t_origin = coord_sys.origin;
    Vec2<double> t_grid_base_x = coord_sys.basis_x * (axis_length.x / double(grid_resolution));
    Vec2<double> t_grid_base_y = coord_sys.basis_y * (axis_length.y / double(grid_resolution));

    char text_buffer[64];
    for(int i = -grid_resolution; i < grid_resolution; ++i) {
	Color color = i == 0 ? COORDINATE_SYSTEM_MAIN_AXIS_COLOR : COORDINATE_SYSTEM_GRID_COLOR;
	DrawLineEx(t_origin + t_grid_base_y * grid_resolution + t_grid_base_x * i,
		   t_origin + t_grid_base_y * grid_resolution * -1 + t_grid_base_x * i, 1, color);
	DrawLineEx(t_origin + t_grid_base_x * grid_resolution + t_grid_base_y * i,
		   t_origin + t_grid_base_x * grid_resolution * -1 + t_grid_base_y * i, 1, color);
	
	snprintf(text_buffer, 64, "%.3g",  (t_grid_base_x.length() * i) / coord_sys.basis_x.length());
	Vec2<double> text_pos = t_origin + t_grid_base_x * i;
	DrawText(text_buffer, int(text_pos.x), int(text_pos.y), COORDINATE_SYSTEM_FONT_SIZE, COORDINATE_SYSTEM_FONT_COLOR);
	snprintf(text_buffer, 64, "%.3g", (t_grid_base_y.length() * i) / coord_sys.basis_y.length());
	text_pos = t_origin + t_grid_base_y * i;
	DrawText(text_buffer, int(text_pos.x), int(text_pos.y), COORDINATE_SYSTEM_FONT_SIZE, COORDINATE_SYSTEM_FONT_COLOR);
    }
}

void draw_data(VP_Camera& camera, std::vector<Data_Plot> data_list, Vec2<int> plot_padding)
{
    if (camera.is_undefined()) {
	uint64_t max_x = 0;
	double max_y = 0;
	double min_y = 0;
	for(const auto& dp : data_list) {
	    max_x = dp.data->size() > max_x ? dp.data->size() : max_x;
	    for(double val : *dp.data) {
		max_y = val > max_y ? val : max_y;
		min_y = val < min_y ? val : min_y;
	    }
	}
	camera.coord_sys.basis_x = { double(g_screen_width - plot_padding.x * 2) / double(max_x), 0 };
	camera.coord_sys.basis_y = { 0 , -double(g_screen_height - plot_padding.y * 2) / (max_y - min_y)};
    }

    // draw the coordinate system
    draw_coordinate_system(camera.coord_sys, COORDINATE_SYSTEM_GRID_SPACING);

    // draw the plot
    for(const auto& dp : data_list) {
	for(size_t x = 0; x < dp.data->size(); ++x) {
	    Vec2<double> screen_space_point = camera.coord_sys.transform_to({double(x), (*dp.data)[x]}, app_coordinate_system);
	    Vec2<double> prev_screen_space_point;
	    if(dp.plot_type & PT_DISCRETE) {
		DrawCircle(std::round(screen_space_point.x), std::round(screen_space_point.y), dp.thickness / 2.f, dp.color);
	    }
	    if(dp.plot_type & PT_INTERP_LINEAR) {
		if(x > 0)
		    DrawLineEx(prev_screen_space_point, screen_space_point, dp.thickness / 3.f, dp.color);
	    }
	    prev_screen_space_point = screen_space_point;
	}
    }
}

struct Fluss_Daten {
public:

    Fluss_Daten()
    {
	camera.coord_sys.origin = {double(plot_padding.x), double(g_screen_height - plot_padding.y)};
    }

    void add_data(std::string file = "../test_eingabe/simple.csv")
    {
	std::vector<std::vector<double>> data_list;
	parse_numeric_csv_file(data_list, file);

	if (data_list.size() >= 1 && data_A.empty())
	    data_A = data_list[0];

	if (data_list.size() >= 2 && data_B.empty())
	    data_B = data_list[1];
	
	if (data_list.size() >= 3)
	    log_error("Expected  2 or less data colums in the file '%s', but read %d.", file.c_str(), data_list.size());
    }

    bool data_full() { return !data_A.empty() && !data_B.empty(); }
    
    void draw()
    {
	if(!data_full())
	    return;
	
	static int old_g_screen_height = g_screen_height;
	camera.coord_sys.origin.y += g_screen_height - old_g_screen_height;
	old_g_screen_height = g_screen_height;

	if(IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
	    DisableCursor();
	    camera.coord_sys.origin.x += g_mouse_delta.x;
	    camera.coord_sys.origin.y += g_mouse_delta.y;
	}

	if(IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
	    EnableCursor();

	if(!IsKeyDown(KEY_LEFT_CONTROL))
	    camera.coord_sys.basis_x = camera.coord_sys.basis_x + camera.coord_sys.basis_x * (g_mouse_wheel.y / 10.f);
	if(!IsKeyDown(KEY_LEFT_SHIFT))
	    camera.coord_sys.basis_y = camera.coord_sys.basis_y + camera.coord_sys.basis_y * (g_mouse_wheel.y / 10.f);
	
	draw_data(camera, {{&data_A, GREEN, Plot_Type(PT_DISCRETE | PT_INTERP_LINEAR)}, {&data_B, RED, PT_DISCRETE}}, plot_padding);
    }
    
private:
    std::vector<double> data_A;
    std::vector<double> data_B;
    Parametric_Function periodic_fit_A;
    Parametric_Function periodic_fit_B;
    VP_Camera camera;
    const Vec2<int> plot_padding = {50, 50};
};

bool load_dropped_files(Fluss_Daten& fluss_daten) {
    if(IsFileDropped()) {
	FilePathList path_list = LoadDroppedFiles();
	for(uint32_t i = 0; i < path_list.count; ++i) {
	    if(!fluss_daten.data_full())
		fluss_daten.add_data(path_list.paths[i]);
	    else
		log_error("The two colums are full, can't read more data.");
	}
	UnloadDroppedFiles(path_list);
	return true;
    }
    return false;
}

int main()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_ALWAYS_RUN | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(g_screen_width, g_screen_height, "Flusssensor Tool");
    SetTargetFPS(TARGET_FPS);
    SetTraceLogLevel(LOG_ERROR); // set higher log level

    Fluss_Daten fluss_daten;
    // fluss_daten.add_data();

    while (!WindowShouldClose()) {

	update_global_app_vars();

	load_dropped_files(fluss_daten);

	BeginDrawing();
	{
	    ClearBackground(WHITE);
	    fluss_daten.draw();
	}
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
