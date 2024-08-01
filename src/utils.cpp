#include "utils.hpp"

#include "data_manager.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <utility>

void Logger::log_help_message()
{
    printf(UTILS_GREEN "Basics" UTILS_END_COLOR "\n\
- Load a file containing comma seperated values, by dropping it on the window.\n\
- Run a script file containing commands, by dropping it on the window (there is also a run script command).\n\
\n\
" UTILS_GREEN "Reverting commands" UTILS_END_COLOR "\n\
    - Use ctrl + Z to revert any command. Chain them to go back to the very start.\n\
    - Use ctrl + Y to revert, reverting a command.\n\
    When loading a new file, you can only go revert back up until this event.\n\
    \n\
" UTILS_GREEN "Movement" UTILS_END_COLOR "\n\
    - Pan the view, by holding the left mouse button and draging.\n\
    - Move the coordinate system, by holding the right mouse button and dragging.\n\
    - Zoom using the scroll wheel. The zoom is centered around the coordinat system.\n\
    - Zoom the X-axis by scrolling while pressing left-shift.\n\
    - Zoom the Y-axis by scrolling while pressing left-ctrl.\n\
    - Fit the window to the content by pressing space.\n\
    \n\
" UTILS_GREEN "Commands" UTILS_END_COLOR "\n\
    The commands are explained mostly by examples.\n\
    Just start typing in the window and the command interface will appear.\n\
    - Press backspace to delete the last character and hold left-ctrl to delete the whole word (token).\n\
    - Press enter to execute the command.\n\
    - Press ESC to exit the interface.\n\
    The commands as well as infos and errors are written to the shell.\n\
    \n\
    " UTILS_BLUE "iterators" UTILS_END_COLOR "\n\
        Iterators can be used everywhere in place of integer values. They can be understood as loops over the commands.\n\
        Multiple iterators can be used in the same command, like nested loops.\n\
        - 1,2,5,3 " UTILS_BRIGHT_BLACK "(iterates over the comma seperated indices.)" UTILS_END_COLOR "\n\
        - 23..199 " UTILS_BRIGHT_BLACK "(iterates from 23 to 199 (inclusive).)" UTILS_END_COLOR "\n\
        - 4,1,2,23..199,500 " UTILS_BRIGHT_BLACK "(they can be combined)" UTILS_END_COLOR "\n\
        - 400..2 " UTILS_BRIGHT_BLACK "(This iterator iterates backwards, although this usually doesn't have any effect.)" UTILS_END_COLOR "\n\
        \n\
    " UTILS_BLUE "zero" UTILS_END_COLOR "\n\
        Resets to origin to (0,0), while fitting the window to the content.\n\
        - zero\n\
        \n\
    " UTILS_BLUE "fit" UTILS_END_COLOR "\n\
        Fits a function to data.\n\
        - function 0 = fit sinusoid data 3\n\
        - function new = fit sinusoid data 3\n\
        - function new \"my fit of data 0\" = fit sinusoid data 0\n\
        \n\
    " UTILS_BLUE "hide" UTILS_END_COLOR "\n\
        Hides objects...\n\
        - hide function 5..10\n\
        and specific visualizations of objects.\n\
        - hide points data 1 " UTILS_BRIGHT_BLACK "(disables point visualization)" UTILS_END_COLOR "\n\
        - hide index data 1 " UTILS_BRIGHT_BLACK "(disables index visualization)" UTILS_END_COLOR "\n\
        - hide lines data 1 " UTILS_BRIGHT_BLACK "(disables line visualization)" UTILS_END_COLOR "\n\
        \n\
    " UTILS_BLUE "show" UTILS_END_COLOR "\n\
        If the object is hidden, show will make it visible.\n\
        If the object is visible, show will fit the window to the object.\n\
        - show data 5\n\
        Also used to show specific visualizations of object.\n\
        - show points data 1 " UTILS_BRIGHT_BLACK "(enables point visualization)" UTILS_END_COLOR "\n\
        - show index data 1 " UTILS_BRIGHT_BLACK "(enables index visualization)" UTILS_END_COLOR "\n\
        - show lines data 1 " UTILS_BRIGHT_BLACK "(enables line visualization)" UTILS_END_COLOR "\n\
        \n\
    " UTILS_BLUE "smooth" UTILS_END_COLOR "\n\
        Averages data over a specified window size.\n\
        - smooth data 1,2,3 5 " UTILS_BRIGHT_BLACK "(window size 5)" UTILS_END_COLOR "\n\
        - smooth data 8 20 " UTILS_BRIGHT_BLACK "(window size 20)" UTILS_END_COLOR "\n\
        \n\
    " UTILS_BLUE "interp" UTILS_END_COLOR "\n\
        Interpolates the data linearly. An integer argument specifies how many times the data should be interpolated (doubled in size).\n\
        - interp data 0 1 " UTILS_BRIGHT_BLACK "(doubles the points of data 0)" UTILS_END_COLOR "\n\
        - interp data 0 2 " UTILS_BRIGHT_BLACK "(quadruples the points of data 0)" UTILS_END_COLOR "\n\
        \n\
    " UTILS_BLUE "setting the X axis" UTILS_END_COLOR "\n\
        Any data is interpreted as just a set of Y values which are plotted by their index, by default.\n\
        However data can reference other data as its X axis.\n\
        - data 1..4 x = data 0\n\
        \n\
    " UTILS_BLUE "extrema" UTILS_END_COLOR "\n\
        Get the local extrema of data. They must be saved in another data object. Usually applied after smoothing the data.\n\
        - data 10 = extrema data 3\n\
        - data new = extrema data 1,3,5\n\
        \n\
    " UTILS_BLUE "deleting things" UTILS_END_COLOR "\n\
        - delete data 0..2\n\
        - delete funciton 4\n\
        Delete specific data points.\n\
        - delete points 100..1000 data 9\n\
        \n\
    " UTILS_BLUE "exporting data and funcitons" UTILS_END_COLOR "\n\
        Exports data as comma seperated .txt files or functions as their variable and value form.\n\
        They will be located in the exports folder.\n\
        - export data 1,2,3 " UTILS_BRIGHT_BLACK "(default file name export)" UTILS_END_COLOR "\n\
        - export data 1,2,3 \"my_data\" " UTILS_BRIGHT_BLACK "(specified file name)" UTILS_END_COLOR "\n\
        - export function 1,2,3 \"my_functions\"\n\
        \n\
    " UTILS_BLUE "saving all executed commands to a script" UTILS_END_COLOR "\n\
        Saves .script files to the scripts folder.\n\
        - save script " UTILS_BRIGHT_BLACK "(default file name save)" UTILS_END_COLOR "\n\
        - save script \"my script\" " UTILS_BRIGHT_BLACK "(specified file name)" UTILS_END_COLOR "\n\
        Keep in mind that all commands are cleared from the command list, omitting them from being saved, if a new file is loaded.\n\
        This is because a script should not depend on any external files.\n\
        Similarly if a command file is loaded, all its commands will be copied to the command list, not the laoding of the script.\n\
        \n\
    " UTILS_BLUE "running command scripts" UTILS_END_COLOR "\n\
        - run \"my script\"\n\
        Dropping the script onto the window is also supported.\n\
        The script must have the file extension .script.\n\
        \n\
    " UTILS_BLUE "printing data" UTILS_END_COLOR "\n\
        The commands must be prefixed with = to print the result.\n\
        - = x points 20 data 7 " UTILS_BRIGHT_BLACK "(prints the x value of the 20th points of data 7)" UTILS_END_COLOR "\n\
        - = y points 0..4 data 7 " UTILS_BRIGHT_BLACK "(prints the y value of points 0..4 of data 7)" UTILS_END_COLOR "\n\
        - = function 3 4525.67 " UTILS_BRIGHT_BLACK "(prints the result of function 3 at X 4525.67)" UTILS_END_COLOR "\n\
        - = function 5 b " UTILS_BRIGHT_BLACK "(prints the parameter b of function 5)" UTILS_END_COLOR "\n\
        These expressions can also be used with the basic mathematical operators.\n\
        \n\
    " UTILS_BLUE "basic mathematical operations" UTILS_END_COLOR "\n\
        - Assigning things.\n\
          - data 0 = data 1\n\
          - data new = data 1..3\n\
          - function new = functin 8\n\
          - data new = data 3 + data 6 " UTILS_BRIGHT_BLACK "(requires data 3 x = data 6 x)" UTILS_END_COLOR "\n\
          - data 4 = data 3 - data 6 " UTILS_BRIGHT_BLACK "(requires data 3 x = data 6 x)" UTILS_END_COLOR "\n\
          - data new = data 3 * data 1..8 " UTILS_BRIGHT_BLACK "(requires data 3 x = data 1..8 x)" UTILS_END_COLOR "\n\
          - data new = data 3 / data 6 " UTILS_BRIGHT_BLACK "(requires data 3 x = data 6 x)" UTILS_END_COLOR "\n\
          - data new = data 0..10 * data 0..10 " UTILS_BRIGHT_BLACK "(double iteration)" UTILS_END_COLOR "\n\
        \n\
        - Operation assinging things. Supports the same operations as above.\n\
          - data 0 += data 1,2\n\
        \n\
        - Cacluating with single values.\n\
          Currently it is not possible to assign to single values.\n\
          - = function 3 d - function 4 d\n"
	);
}

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
