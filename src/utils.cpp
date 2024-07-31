#include "utils.hpp"

#include "data_manager.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <utility>

std::pair<char *, size_t> parse_file_cstr(const char *file_name)
{
    std::ifstream file(file_name, std::ios::binary);
    
    if(!file.is_open()) {
	logger.log_error("Failed to open file '%s'\n.", file_name);
	return {nullptr, 0};
    }
    
    file.seekg(0, file.end);
    size_t file_size = file.tellg();
    file.seekg(0, file.beg);

    char* data = new char[file_size + 1];
    file.read(data, file_size);

    if(!file) {
	logger.log_error("Error reading file '%s'. Only %d could be read.\n", file_name, file.gcount());
	return {nullptr, 0};
    }
    
    data[file_size] = '\0';
    return { data, file_size };
}

static bool is_whitespace(char c)
{
    return c == ' ' || c == '\n' || c == '\t' || c == '\f' || c == '\r' || c == '\v';
}

static bool is_new_line(char c)
{
    return c == '\n' || c == '\f' || c == '\r';
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static void fix_comma_notation(bool inside_string, char* p) {
    if (inside_string) {
	if (*p == ',')
	    *p = '.';
    }
}

std::vector<Plot_Data*> parse_numeric_csv_file(const std::string& file_name)
{
    std::vector<Plot_Data*> data_list;
    
    auto load_result = parse_file_cstr(file_name.c_str());
    char* p = load_result.first;
    size_t file_size = load_result.second;
     const char* p_end = p + file_size;

    size_t data_column = 0;
    if (data_list.size() < data_column + 1)
	data_list.push_back(new Plot_Data);
    

    bool inside_string = false;

    char* prev_p = p;
    
    while (p < p_end) {
	
	prev_p = p;
	
	while (p < p_end && is_whitespace(*p)) {
	    if (is_new_line(*p))
		data_column = 0;
	    ++p;
	}

	if (p < p_end && *p == '\"') {
	    inside_string = !inside_string;
	    ++p;
	}

	if (p < p_end && *p == '#') {
	    while (p < p_end && *p != '\n') {
		++p;
	    }
	}

	// change coma to dots, if inside a string (fixing , notation in numbers)
	fix_comma_notation(inside_string, p);
	if (p < p_end && (is_digit(*p) || (((p[0] == '.') || (p[0] == '+') || (p[0] == '-')) && is_digit(*(p + 1))))) {
	    bool has_point = (*p == '.');
	    char* begin = p;
	    ++p;

	    fix_comma_notation(inside_string, p);
	    while (p < p_end && (is_digit(*p) || *p == '.' || is_alpha(*p))) {
		if (has_point && (*p == '.'))
		    break;
		has_point |= (*p == '.');
		++p;
		fix_comma_notation(inside_string, p);
	    }

	    // parse time code
	    if (p < p_end && *p == ':') {
		double time_s = 0;
		time_s += std::strtod(begin, &p) * 60 * 60; // hours
		++p;
		begin = p;
		p += 2;
		time_s += std::strtod(begin, &p) * 60; // minutes
		++p;
		begin = p;
		p += 2;
		*p = ':';
		time_s += std::strtod(begin, &p); // seconds
		++p;
		begin = p;
		p += 3;
		time_s += std::strtod(begin, &p) * 0.001; // milliseconds

		data_list[data_column]->y.push_back(time_s);
	    }
	    // parse number
	    else {
		data_list[data_column]->y.push_back(std::strtod(begin, &p));
		if (errno == ERANGE)
		    std::cout << "Parsed number was out of range\n";
	    }
	}
	
	if (p < p_end && *p == ',') {
	    ++data_column;
	    while (data_list.size() - 1 < data_column)
		data_list.push_back(new Plot_Data);
	    ++p;
	}

	if (p < p_end && p == prev_p && inside_string) {
	    char* begin = p;
	    while (p < p_end && inside_string) {
		++p;
		if (p < p_end && *p == '\"') {
		    inside_string = !inside_string;
		}
		fix_comma_notation(inside_string, p);
	    }
	    data_list[data_column]->info.header += std::string(begin, p - begin);
	    ++p;
	}
    }
    return data_list;
}

uint64_t hash_string_view(std::string_view s_v, uint64_t hash) {
    for(size_t i = 0; i < s_v.size(); ++i)
	hash = (hash * 33) ^ s_v[i];
    return hash;
}

Vector2 draw_text_boxed(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint)
{
    Vector2 size = MeasureTextEx(font, text, fontSize, spacing);
    DrawRectangleV({position.x - 3.f, position.y - 3.f}, {size.x + 3.f, size.y + 3.f}, {255, 255, 255, 255});
    DrawTextEx(font, text, position, fontSize, spacing, tint);
    return size;
}

std::string get_file_extension(std::string file_name)
{
    size_t i = file_name.size() - 1;
    while (i >= 0 && file_name[i] != '.') {
	--i;
    }
    return file_name.substr(i, file_name.size() - i);
}
