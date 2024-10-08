#pragma once

#include "raylib.h"

#include <string>
#include <vector>

#include "utils.hpp"

#include "lexer.hpp"

struct Data_Manager;

bool handle_dropped_files();

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

struct Content_Tree_Element_String {

    Content_Tree_Element_String(std::string str) : str(str) {}
    Content_Tree_Element_String(std::string str, bool new_field, Color color)
	: str(str), new_field(new_field), color(color) {}

    std::string str;
    bool new_field = true;
    Color color = BLACK;
};

struct Content_Tree_Element
{
    Content_Tree_Element() {}
    Content_Tree_Element(std::string name) : name(name) {}

    std::string name;
    Color name_color = BLACK;
    bool open = false;
    std::vector<Content_Tree_Element_String> content;
};

struct Content_Tree
{
    Content_Tree()
    {
	base_element.name = "content";
	base_element.open = true;
    }
    
    void draw();
    void add_element(Content_Tree_Element* elem)
    {
	content_elements.push_back(elem);
    }
    void delete_element(Content_Tree_Element* elem);
    void clear() { content_elements.clear(); }
    
private:
    Content_Tree_Element base_element;
    std::vector<Content_Tree_Element*> content_elements;

private:
    void draw_element(Vec2<int> &draw_pos, Content_Tree_Element* elem, int font_size, Font font);
};

struct Text_Input
{
    Text_Input() : keyboard_lock_id(get_uuid()) {}
    void draw();
    void update();
    
private:

    bool keyboard_access();
    
    bool input_active = true;
    Lexer lexer;
    bool show_cursor = true;
    const int keyboard_lock_id;
};
