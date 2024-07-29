#include "flusssensor_tool.hpp"

#include "lexer.hpp"
#include "raylib.h"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <vector>
#include <string>
#include <cstdint>

#include "utils.hpp"
#include "command_parser.hpp"

// app
constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 600;
constexpr int TARGET_FPS = 60;
Font g_app_font_18;
Font g_app_font_20;
Font g_app_font_22;
bool g_keyboard_lock = false;

// coordinate system
constexpr int COORDINATE_SYSTEM_GRID_SPACING = 60;
constexpr int COORDINATE_SYSTEM_FONT_SIZE = 18;
constexpr Font* COORDINATE_SYSTEM_FONT = &g_app_font_18;
constexpr Color COORDINATE_SYSTEM_FONT_COLOR = Color{0, 0, 0, 150};
constexpr Color COORDINATE_SYSTEM_MAIN_AXIS_COLOR = Color{0, 0, 0, 255};
constexpr Color COORDINATE_SYSTEM_GRID_COLOR = Color{0, 0, 0, 75};
constexpr Coordinate_System app_coordinate_system = {{0, 0}, {1, 0}, {0, 1}};

// content tree
constexpr Vec2<int> CONTENT_TREE_OFFSET {20, 20};
constexpr int CONTENT_TREE_FONT_SIZE = 20;
constexpr Font* CONTENT_TREE_FONT = &g_app_font_20;
constexpr int CONTENT_TREE_HEADER_FONT_SIZE = 22;
constexpr Font* CONTENT_TREE_HEADER_FONT = &g_app_font_22;
constexpr float CONTENT_TREE_VERTICAL_SPACING_MULTIPLIER = 1.05;
constexpr int CONTENT_TREE_CHILD_X_OFFSET = 10;

// text input
constexpr Vec2<int> TEXT_INPUT_OFFSET{20, 20};
constexpr Font *TEXT_INPUT_FONT = &g_app_font_20;
constexpr int TEXT_INPUT_FONT_SIZE = 20;
constexpr float TEXT_INPUT_MIN_BOX_SIZE = 100;
constexpr double TEXT_INPUT_CURSO_BLINK_TIME = 0.5; // in seconds

// plot data
constexpr int PLOT_DATA_FONT_SIZE = 18;
constexpr Font* PLOT_DATA_FONT = &g_app_font_18;

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
    content_element.content.push_back({"size = " + std::to_string(y.size())});
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
    DrawRectangleV({position.x - 3.f, position.y - 3.f}, {size.x + 3.f, size.y + 3.f}, {255, 255, 255, 255});
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

    draw_element(draw_pos, &base_element, CONTENT_TREE_HEADER_FONT_SIZE, *CONTENT_TREE_HEADER_FONT);
    draw_pos.x += CONTENT_TREE_CHILD_X_OFFSET;

    if (base_element.open) {
	for(const auto elem : content_elements)
	    draw_element(draw_pos, elem, CONTENT_TREE_FONT_SIZE, *CONTENT_TREE_FONT);
    }
}

void Content_Tree::delete_element(Content_Tree_Element *elem)
{
    for (size_t i = 0; i < content_elements.size(); ++i) {
	if (content_elements[i] == elem) {
	    content_elements.erase(content_elements.begin() + i);
	}
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
	if (!pd->x || !pd->info.visible)
	    continue;
	
	for(size_t ix = 0; ix < pd->size(); ++ix)
	{
	    Vec2<double> screen_space_point = camera.coord_sys.transform_to(Vec2<double>{pd->x->y[ix], pd->y[ix]} + camera.origin_offset, app_coordinate_system);
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

Data_Manager::Data_Manager()
{
    camera.coord_sys.origin = {double(plot_padding.x), double(GetScreenHeight() - plot_padding.y)};
    camera.coord_sys.basis_x = {1, 0};
    camera.coord_sys.basis_y = {0, -1};
    default_x.content_element.name = "default";
}

Data_Manager::~Data_Manager()
{
    for (auto pd : plot_data)
	delete pd;
    for (auto f : functions)
	delete f;
}

void Data_Manager::new_plot_data(Plot_Data *data)
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

void Data_Manager::delete_plot_data(Plot_Data *data)
{
    // also checks for references to the deleted object and resolves them
    for (size_t i = 0; i < plot_data.size(); ++i) {
	if (plot_data[i] == data) {
	    plot_data.erase(plot_data.begin() + i);
	    content_tree.delete_element(&data->content_element);
	    delete data;
	}
	
	if (plot_data[i]->x == data)
	    plot_data[i]->x = &default_x;
    }
}

void Data_Manager::new_function()
{
    functions.push_back(new Function);
    
    functions.back()->info.color = graph_color_array[graph_color_array_idx];
    ++graph_color_array_idx;
    if (graph_color_array_idx >= graph_color_array_cnt)
	graph_color_array_idx = 0;
    
    content_tree.add_element(&functions.back()->content_element);
}

void Data_Manager::delete_function(Function *func)
{
    for (size_t i = 0; i < functions.size(); ++i) {
	if (functions[i] == func) {
	    functions.erase(functions.begin() + i);
	    content_tree.delete_element(&func->content_element);
	    delete func;
	}
    }
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
    
    if (IsKeyPressed(KEY_SPACE) && !g_keyboard_lock) {
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

void Data_Manager::zero_coord_sys_origin()
{
    camera.coord_sys.origin = {double(plot_padding.x), double(GetScreenHeight() - plot_padding.y)};
    camera.origin_offset = {0, 0};
}

void Data_Manager::fit_camera_to_plot()
{
    if (plot_data.empty() && functions.empty())
	return;

    double max_x = -HUGE_VAL, max_y = -HUGE_VAL, min_x = HUGE_VAL, min_y = HUGE_VAL;
	
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

    if (max_x != -HUGE_VAL && min_x != HUGE_VAL) {
	camera.coord_sys.basis_x = { double(GetScreenWidth() - plot_padding.x * 2) / (max_x - min_x), 0 };
	camera.origin_offset.x = min_x;
    }
    else {
	camera.coord_sys.basis_x = { 1, 0 };
	camera.origin_offset.x = 0;
    }
    
    if (max_y != -HUGE_VAL && min_y != HUGE_VAL) {
	camera.coord_sys.basis_y = { 0, -double(GetScreenHeight() - plot_padding.y * 2) / (max_y - min_y)};
	camera.origin_offset.y = -min_y;
    }
    else {
	camera.coord_sys.basis_y = { 0, -1};
	camera.origin_offset.x = 0;
    }
}

void Data_Manager::fit_camera_to_plot(Plot_Data *plot_data)
{
    double max_x = -HUGE_VAL, max_y = -HUGE_VAL, min_x = HUGE_VAL, min_y = HUGE_VAL;
	
    for(double val : plot_data->x->y) {
	max_x = val > max_x ? val : max_x;
	min_x = val < min_x ? val : min_x;
    }
    
    for(double val : plot_data->y) {
	max_y = val > max_y ? val : max_y;
	min_y = val < min_y ? val : min_y;
    }
    
    camera.coord_sys.basis_x = { double(GetScreenWidth() - plot_padding.x * 2) / (max_x - min_x), 0 };
    camera.coord_sys.basis_y = { 0 , -double(GetScreenHeight() - plot_padding.y * 2) / (max_y - min_y)};
    camera.origin_offset = { min_x, -min_y };
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

void Text_Input::tokenize_input()
{
    lexer = Lexer{};
    lexer.load_input_from_string(input);
    lexer.tokenize();
}

void Text_Input::update(Data_Manager& data_manager)
{
    int key = GetCharPressed();
    
    if ((key >= 97) && (key <= 122))
	input_active = true;
    
    if (IsKeyPressed(KEY_ENTER))
	input_active = false;

    if (IsKeyPressed(KEY_ESCAPE)) {
	input_active = false;
	input.clear();
    }

    static size_t prev_input_size = 0;

    g_keyboard_lock = false;
    if (input_active) {
	g_keyboard_lock = true;
	
	while (key > 0) {
	    if ((key >= 32) && (key <= 127)) {
		input += (char)key;
	    }
	    key = GetCharPressed();  // get next character in the queue
	}

	if (input.size() != prev_input_size) {
	    tokenize_input();
	    prev_input_size = input.size();
	}
	
	if (IsKeyPressed(KEY_BACKSPACE)) {
	    if (!input.empty()) {
		if (IsKeyDown(KEY_LEFT_CONTROL)) {
		    input.erase(input.begin() + (lexer.get_tokens().back().ptr - lexer.get_input().c_str()), input.end());
		    tokenize_input();
		    prev_input_size = input.size();
		}
		else {
		    input.pop_back();
		}
	    }
	    else {
		input_active = false;
	    }
	}
    }
    else if (!lexer.get_input().empty()) {
	handle_command(data_manager, lexer);
	lexer = Lexer{};
	input.clear();
    }
}

void Text_Input::draw()
{
    Vector2 draw_pos = { TEXT_INPUT_OFFSET.x, float(GetScreenHeight()) - (TEXT_INPUT_OFFSET.y + TEXT_INPUT_FONT_SIZE) };
    if (input_active) {
	char str[2] = {0, 0};

	static int previous_result = 0;
	int result = size_t(GetTime() / TEXT_INPUT_CURSO_BLINK_TIME) % 2;
	if (result != previous_result) {
	    previous_result = result;
	    show_cursor = !show_cursor;
	}
	
	Vector2 size = {0, 0};
	if (!lexer.get_input().empty())
	   size = MeasureTextEx(*TEXT_INPUT_FONT, lexer.get_input().c_str(), TEXT_INPUT_FONT_SIZE, 0);
	if (show_cursor)
	    size.x += MeasureTextEx(*TEXT_INPUT_FONT, "|", TEXT_INPUT_FONT_SIZE, 0).x;
	DrawRectangleV({draw_pos.x - 3.f, draw_pos.y - 3.f}, {std::max(size.x, TEXT_INPUT_MIN_BOX_SIZE) + 8.f, TEXT_INPUT_FONT_SIZE + 3.f}, {230, 230, 230, 255});
	
	if (!lexer.get_input().empty()) {
	    Color color;
	    int tkn_idx = 0;
	    const std::vector<Token>& tokens = lexer.get_tokens();
	    for(size_t i = 0; i < lexer.get_input().size(); ++i) {
		str[0] = lexer.get_input()[i];

		if (!tokens.empty()) {
		    while (lexer.get_input().c_str() + i >= tokens[tkn_idx].ptr + tokens[tkn_idx].size)
			++tkn_idx;
	    
		    if ((lexer.get_input().c_str() + i >= tokens[tkn_idx].ptr) && (lexer.get_input().c_str() + i < tokens[tkn_idx].ptr + tokens[tkn_idx].size)) {
			if (tokens[tkn_idx].type < 256 || (tokens[tkn_idx].type >= tkn_update_add && tokens[tkn_idx].type <= tkn_pow))
			    color = DARKGRAY;
			else if (tokens[tkn_idx].type == tkn_data || tokens[tkn_idx].type == tkn_function)
			    color = DARKGREEN;
			else if (tokens[tkn_idx].type >= tkn_int && tokens[tkn_idx].type <= tkn_false)
			    color = BLUE;
			else if (tokens[tkn_idx].type >= tkn_fit && tokens[tkn_idx].type <= tkn_delete)
			    color = MAROON;
			else
			    color = BLACK;
		    }
		}

		DrawTextEx(*TEXT_INPUT_FONT, str, draw_pos, TEXT_INPUT_FONT_SIZE, 0, color);
		color = BLACK;
		draw_pos.x += MeasureTextEx(*TEXT_INPUT_FONT, str, TEXT_INPUT_FONT_SIZE, 0).x;
	    }
	}
	
	if (show_cursor)
	    DrawTextEx(*TEXT_INPUT_FONT, "|", draw_pos, TEXT_INPUT_FONT_SIZE, 0, BLACK);
    }
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
    SetExitKey(KEY_NULL);

    g_app_font_18 = LoadFontEx("resources/Roboto-Regular.ttf", 18, nullptr, 0);
    g_app_font_20 = LoadFontEx("resources/Roboto-Regular.ttf", 20, nullptr, 0);
    g_app_font_22 = LoadFontEx("resources/Roboto-Regular.ttf", 22, nullptr, 0);

    Data_Manager data_manager;
    Text_Input text_input;

    while (!WindowShouldClose()) {

	load_dropped_files(data_manager);

	text_input.update(data_manager);

	BeginDrawing();
	{
	    ClearBackground(WHITE);
	    data_manager.draw();
	    text_input.draw();
	}
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
