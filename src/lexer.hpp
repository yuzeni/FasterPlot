#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string_view>
#include <string>
#include <vector>

#include "utils.hpp"

enum Token_enum : uint32_t {
    tkn_eof = 256,
    tkn_none,
    tkn_parse_error,
    
    /* literals */

    tkn_ident,
    tkn_int,
    tkn_real,
    tkn_string,
    tkn_true, // add
    tkn_false, // add

    /* keywords */
    tkn_fit,
    tkn_sinusoid,
    tkn_data,
    tkn_function,
    tkn_new,
    tkn_points, // add // data 1 points = true
    tkn_lines, // add
    tkn_x, // add // data 1 x = data 0
    tkn_hide, // add // data 1 hide = true
    tkn_smooth, // smooth data 1, 10
    tkn_interp, // interp data 1, 1.3
    

    /* mutliple char operators */

    tkn_update_add,
    tkn_update_sub,
    tkn_update_mul,
    tkn_update_div,
    tkn_update_pow,
    tkn_update_mod,
    tkn_pow,
    
    tkn_SIZE,
};

std::string get_token_name_str(Token_enum tkn);

struct Token {
    Token() {}
    Token(Token_enum type, char* ptr, void* data = nullptr);
    char* ptr = nullptr;
    uint32_t flag = 0;
    Token_enum type = tkn_none;
    union {
	std::string_view sv;
	int64_t  i = 0;
	double   d;
    };
};

class Lexer {
public:
    
    void load_input_from_string(std::string source);
    bool next_token(); // wrapper function
    Token& peek_next_token();
    bool not_eof() { return tkn.type != tkn_eof; }
    void print_token(Token& tkn, bool show_content = false) const;

    template<typename... Args>
    void parsing_error(Token& tkn, const char* msg, Args... args)
    {
	print_error_expression(tkn.ptr, msg, args...);
	++parsing_error_cnt;
    }

    template<typename... Args>
    void print_error_expression(const char* from_p, const char* msg, Args... args)
    {
	std::string error_section = input;
	error_section += '\n';
	for(ptrdiff_t i = 0; i < from_p - input.c_str(); ++i)
	    error_section += ' ';
	error_section += '^';
	
	char buffer[1024];
	snprintf(buffer, 1024, msg, args...);
	log_error("%s\n%s\n", buffer, error_section.c_str());
	++error_cnt;
    }

    int parsing_error_cnt = 0;
    int error_cnt = 0;

    Token tkn;

private:

    bool push(Token tkn) { this->tkn = tkn; return true; }

    std::string input;
    char* p_begin = nullptr;
    char* p = nullptr;
    char* eof = nullptr;

    void after_load_init();
    bool push_next_token();
};

bool is_open_bracket(char c);
bool is_open_bracket(char c);
bool is_close_bracket(char c);
bool is_delim_tkn_left(Token_enum type);
bool is_delim_tkn_right(Token_enum type);

constexpr bool is_assignment_operator_tkn(Token_enum type)
{
    return type == '=' || (type >= tkn_update_add && type <= tkn_update_mod);
}

