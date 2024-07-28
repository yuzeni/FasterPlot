#include "flusssensor_tool.hpp"

#include "raylib.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <vector>
#include <string>
#include <cstdint>

#include "utils.hpp"
#include "command_parser.hpp"

constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 600;
constexpr int TARGET_FPS = 60;
constexpr int COORDINATE_SYSTEM_GRID_SPACING = 60;
constexpr int COORDINATE_SYSTEM_FONT_SIZE = 18;
constexpr Color COORDINATE_SYSTEM_FONT_COLOR = Color{0, 0, 0, 150};
constexpr Color COORDINATE_SYSTEM_MAIN_AXIS_COLOR = Color{0, 0, 0, 255};
constexpr Color COORDINATE_SYSTEM_GRID_COLOR = Color{0, 0, 0, 75};
// Raylibs screen coordinate system has the origin in the top left corner.
// X points right and Y points down.
constexpr Coordinate_System app_coordinate_system = {{0, 0}, {1, 0}, {0, 1}};

constexpr Vec2<int> CONTENT_TREE_OFFSET {20, 20};
constexpr int CONTENT_TREE_FONT_SIZE = 20;
constexpr int CONTENT_TREE_HEADER_FONT_SIZE = 22;
constexpr float CONTENT_TREE_VERTICAL_SPACING_MULTIPLIER = 1.05;
constexpr int CONTENT_TREE_CHILD_X_OFFSET = 10;

Font g_app_font_18;
Font g_app_font_20;
Font g_app_font_22;

void Plot_Data::update_content_tree_element(size_t index)
{
    content_element.name = "data " + std::to_string(index) + (!info.header.empty() ? " '" + info.header + "'" : "");
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
}

void Function::update_content_tree_element(size_t index)
{
    content_element.name = "function " + std::to_string(index) + (!info.header.empty() ? " '" + info.header + "'" : "");
    content_element.name_color = info.color;
    content_element.content.clear();
    if (info.visible)
	content_element.content.push_back({"visible"});
    else
	content_element.content.push_back({"hidden"});
    if (fit_from_data) {
	content_element.content.push_back({"fit of "});
	content_element.content.push_back({fit_from_data->content_element.name, false, fit_from_data->info.color});
    }
}

static Vector2 draw_and_check_text_boxed(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint, Content_Tree_Element* elem)
{
    Vector2 size = MeasureTextEx(font, text, fontSize, spacing);
    Color box_color = {255, 255, 255, 255};
    if (CheckCollisionPointRec(GetMousePosition(), Rectangle{position.x, position.y, size.x, size.y})) {
	box_color = {200, 200, 200, 255};
	if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
	    elem->open = !elem->open;
	}
    }
    DrawRectangleV({position.x - 3, position.y - 3}, {size.x + 3, size.y + 3}, box_color);
    DrawTextEx(font, text, position, fontSize, spacing, tint);
    return size;
}

static Vector2 draw_text_boxed(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint)
{
    Vector2 size = MeasureTextEx(font, text, fontSize, spacing);
    DrawRectangleV({position.x - 3, position.y - 3}, {size.x + 3, size.y + 3}, {255, 255, 255, 255});
    DrawTextEx(font, text, position, fontSize, spacing, tint);
    return size;
}

void Content_Tree::draw_element(Vec2<int> &draw_pos, Content_Tree_Element* elem, int font_size, Font font)
{

    Vector2 old_draw_pos = draw_pos;
    if (elem->open)
	draw_pos.x += draw_and_check_text_boxed(font, "v ", Vector2{(float)draw_pos.x, (float)draw_pos.y}, font_size, 0, BLACK, elem).x;
    else
	draw_pos.x += draw_and_check_text_boxed(font, "> ", Vector2{(float)draw_pos.x, (float)draw_pos.y}, font_size, 0, BLACK, elem).x;

    draw_pos.x += draw_text_boxed(font, elem->name.c_str(), Vector2{(float)draw_pos.x, (float)draw_pos.y}, font_size, 0, elem->name_color).x;
    draw_pos.y += std::round(double(font_size) * CONTENT_TREE_VERTICAL_SPACING_MULTIPLIER);
    draw_pos.x = old_draw_pos.x + CONTENT_TREE_CHILD_X_OFFSET;

    if (elem->open) {
	for (size_t i = 0; i < elem->content.size(); ++i) {
	    if (elem->content[i].new_field || i == 0)
		draw_pos.x += draw_text_boxed(font, "- ", Vector2{(float)draw_pos.x, (float)draw_pos.y}, font_size, 0, BLACK).x;
	    
	    draw_text_boxed(font, elem->content[i].str.c_str(), Vector2{(float)draw_pos.x, (float)draw_pos.y}, font_size, 0, elem->content[i].color);
	    
	    if (i + 1 == elem->content.size() || elem->content[i + 1].new_field) {
		draw_pos.y += std::round(double(font_size) * 1.1);
		draw_pos.x = old_draw_pos.x + CONTENT_TREE_CHILD_X_OFFSET;
	    }
	    else {
		draw_pos.x += MeasureTextEx(font, elem->content[i].str.c_str(), font_size, 0).x;
	    }
	}
    }
    draw_pos.x = old_draw_pos.x;
}

void Content_Tree::draw()
{
    Vec2<int> draw_pos = CONTENT_TREE_OFFSET;

    draw_element(draw_pos, &base_element, CONTENT_TREE_HEADER_FONT_SIZE, g_app_font_22);
    draw_pos.x += CONTENT_TREE_CHILD_X_OFFSET;

    if (base_element.open) {
	for(const auto elem : content_elements)
	    draw_element(draw_pos, elem, CONTENT_TREE_FONT_SIZE, g_app_font_20);
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
	DrawTextEx(g_app_font_18, text_buffer, Vector2{float(text_pos.x), float(text_pos.y)}, COORDINATE_SYSTEM_FONT_SIZE, 0, COORDINATE_SYSTEM_FONT_COLOR);
	snprintf(text_buffer, 64, "%.4g", (t_grid_base_y.length() * double(i)) / camera.coord_sys.basis_y.length() - camera.origin_offset.y);
	text_pos = t_origin + t_grid_base_y * (double(i) + double(i == 0 ? 0.25 : 0));
	DrawTextEx(g_app_font_18, text_buffer, Vector2{float(text_pos.x), float(text_pos.y)}, COORDINATE_SYSTEM_FONT_SIZE, 0, COORDINATE_SYSTEM_FONT_COLOR);
    }
}

void Data_Manager::draw_plot_data()
{
    for(const auto& pd : plot_data) {
	if (!pd->x || !pd->info.visible)
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
	if (!func->is_defined() || !func->x || !func->info.visible)
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
    camera.coord_sys.origin = {double(plot_padding.x), double(GetScreenHeight() - plot_padding.y)};
    default_x.content_element.name = "default";
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
    
    content_tree.add_element(&plot_data.back()->content_element);
}

void Data_Manager::new_function()
{
    functions.push_back(new Function);
    
    functions.back()->x = &default_x;
    functions.back()->info.color = graph_color_array[graph_color_array_idx];
    ++graph_color_array_idx;
    if (graph_color_array_idx >= graph_color_array_cnt)
	graph_color_array_idx = 0;
    
    content_tree.add_element(&functions.back()->content_element);
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

    static int old_g_screen_height = GetScreenHeight();
    camera.coord_sys.origin.y += GetScreenHeight() - old_g_screen_height;
    old_g_screen_height = GetScreenHeight();

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

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
	Vector2 mouse_delta = GetMouseDelta();
	camera.coord_sys.origin.x += mouse_delta.x;
	camera.coord_sys.origin.y += mouse_delta.y;
    }

    if (!IsKeyDown(KEY_LEFT_CONTROL)) {
	camera.coord_sys.basis_x = camera.coord_sys.basis_x + camera.coord_sys.basis_x * (GetMouseWheelMoveV().y / 10.f);
    }
	
    if (!IsKeyDown(KEY_LEFT_SHIFT)) {
	camera.coord_sys.basis_y = camera.coord_sys.basis_y + camera.coord_sys.basis_y * (GetMouseWheelMoveV().y / 10.f);
    }

    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
	Vec2<double> normalize = Vec2<double>{camera.coord_sys.basis_x.length(), -camera.coord_sys.basis_y.length()};
	camera.coord_sys.origin = camera.coord_sys.origin + camera.origin_offset * normalize;
	camera.origin_offset = (camera.coord_sys.origin - Vec2<double>{double(GetMousePosition().x), double(GetMousePosition().y)}) / normalize;
	camera.coord_sys.origin = camera.coord_sys.origin - camera.origin_offset * normalize;
		
    }
    
    if (IsKeyPressed(KEY_SPACE)) {
	Vec2<double> normalize = Vec2<double>{camera.coord_sys.basis_x.length(), -camera.coord_sys.basis_y.length()};
	camera.coord_sys.origin = camera.coord_sys.origin + camera.origin_offset * normalize;
	camera.origin_offset = {0, 0};
	camera.coord_sys.origin = {double(plot_padding.x), double(GetScreenHeight() - plot_padding.y)};
	fit_camera_to_plot();
    }

    draw_vp_camera_coordinate_system(camera, COORDINATE_SYSTEM_GRID_SPACING);
    draw_plot_data();
    draw_functions();
    
    for (size_t i = 0; i < plot_data.size(); ++i)
	plot_data[i]->update_content_tree_element(i);
    for (size_t i = 0; i < functions.size(); ++i)
	functions[i]->update_content_tree_element(i);
    content_tree.draw();
}

void Data_Manager::fit_camera_to_plot()
{
    double max_x = 0;
    double min_x = 0;
    double max_y = 0;
    double min_y = 0;
	
    for(const auto& pd : plot_data) {
	if (!pd->x || !pd->info.visible)
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
    
    camera.coord_sys.basis_x = { double(GetScreenWidth() - plot_padding.x * 2) / (max_x - min_x), 0 };
    camera.coord_sys.basis_y = { 0 , -double(GetScreenHeight() - plot_padding.y * 2) / (max_y - min_y)};
    
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
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Flusssensor Tool");
    SetTargetFPS(TARGET_FPS);

    g_app_font_18 = LoadFontEx("resources/Roboto-Regular.ttf", 18, nullptr, 0);
    g_app_font_20 = LoadFontEx("resources/Roboto-Regular.ttf", 20, nullptr, 0);
    g_app_font_22 = LoadFontEx("resources/Roboto-Regular.ttf", 22, nullptr, 0);

    Data_Manager data_manager;

    while (!WindowShouldClose()) {

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
