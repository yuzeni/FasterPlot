#include "utils.hpp"

#include <fstream>
#include <iostream>

std::pair<char *, size_t> parse_file_cstr(const char *file_name)
{
    std::ifstream file(file_name, std::ios::binary);
    
    if(!file.is_open()) {
	std::cout << "Failed to open file " << file_name << '\n';
	return {nullptr, 0};
    }
    
    file.seekg(0, file.end);
    size_t file_size = file.tellg();
    file.seekg(0, file.beg);

    char* data = new char[file_size + 1];
    file.read(data, file_size);

    if(!file) {
	std::cout << "Error reading file: " << file_name << ". Only " << file.gcount() << " could be read\n";
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

void parse_numeric_csv_file(std::vector<std::vector<double>> &data_list, std::string file_name)
{
    auto load_result = parse_file_cstr(file_name.c_str());
    char* p = load_result.first;
    size_t file_size = load_result.second;
     const char* p_end = p + file_size;

    size_t data_column = 0;
    if (data_list.size() < data_column + 1)
	data_list.push_back({});
    

    while (p < p_end) {
	
	while (p < p_end && is_whitespace(*p)) {
	    if (is_new_line(*p))
		data_column = 0;
	    ++p;
	}

	if (p < p_end && (is_digit(*p) || (((p[0] == '.') || (p[0] == '+') || (p[0] == '-')) && is_digit(*(p + 1))))) {
	    bool has_point = (*p == '.');
	    char* begin = p;
	    ++p;
	    while (p < p_end && (is_digit(*p) || *p == '.' || is_alpha(*p))) {
		if (has_point && (*p == '.'))
		    break;
		has_point |= (*p == '.');
		++p;
	    }
	    
	    data_list[data_column].push_back(std::strtod(begin, &p));
	    if (errno == ERANGE)
		std::cout << "Parsed number was out of range\n";
	}

	if (p < p_end && *p == ',') {
	    ++data_column;
	    while (data_list.size() - 1 < data_column)
		data_list.push_back({});
	    ++p;
	}
    }
}
