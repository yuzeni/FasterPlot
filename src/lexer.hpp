#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <algorithm>
#include <string_view>

#include "global_vars.hpp"
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
    tkn_iterator,

    /* keywords */
    tkn_fit,
    tkn_sinusoid,
    tkn_data,
    tkn_function,
    tkn_new,
    tkn_points, // add // show points data 1
    tkn_lines, // add // hide lines data 1
    tkn_index,
    tkn_x,
    tkn_y,
    tkn_show,
    tkn_hide,
    tkn_smooth,
    tkn_interp,
    tkn_extrema, // data new = extrema data 1 10
    tkn_delete,
    tkn_export,

    /* mutliple char operators */
    tkn_update_add,
    tkn_update_sub,
    tkn_update_mul,
    tkn_update_div,
    tkn_update_pow,
    tkn_update_mod,
    tkn_pow,
    tkn_range,
    
    tkn_SIZE,
};

std::string get_token_name_str(Token_enum tkn);

struct Token {
    Token() {}
    Token(Token_enum type, char* ptr, char* ptr_end, void* data = nullptr);
    char* ptr = nullptr;
    ptrdiff_t size = 0;
    uint32_t flag = 0;
    Token_enum type = tkn_none;
    union {
	std::string_view sv;
	int64_t i = 0;
	double d;
	std::vector<int64_t>* itr; // highly prone to leaking !!
    };
    void delete_itr() { if(type == tkn_iterator) delete itr; }
};

class Lexer {
public:
    
    void load_input_from_string(std::string source) { input = source; }
    void tokenize();
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
	logger.log_error("%s\n%s", buffer, error_section.c_str());
	++error_cnt;
    }

    int parsing_error_cnt = 0;
    int error_cnt = 0;

    std::string& get_input() { return input; }
    std::vector<Token>& get_tokens() { return tkns; }

    Token& tkn(size_t offset = 0) { return tkns[std::clamp(tkn_idx + offset, size_t(0), tkns.size())]; }
    size_t tkn_idx = 0; // helper variable for the consumer of tkns;

private:

    bool get_next_token();
    bool push(Token tkn) { tkns.push_back(tkn); return true; }
    void post_tokenization();

    std::string input;
    std::vector<Token> tkns;
    char* p_begin = nullptr;
    char* p = nullptr;
    char* eof = nullptr;

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
