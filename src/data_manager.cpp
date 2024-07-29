#include "data_manager.hpp"

#include "global_vars.hpp"


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

