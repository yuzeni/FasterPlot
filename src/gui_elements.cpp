#include "gui_elements.hpp"

#include "global_vars.hpp"
#include "command_parser.hpp"

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