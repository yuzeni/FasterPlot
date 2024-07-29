#include "lexer.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <optional>
#include <string_view>

#include "utils.hpp"

#ifdef TRACY_ENABLE
#  include "tracy/Tracy.hpp"
#endif


static const char *token_name_table[tkn_SIZE - 256]{
    "eof",
    "none",
    "parse_error",
    
    "ident",
    "int",
    "real",
    "string",
    "true",
    "false",

    "fit",
    "sinusoid",
    "data",
    "function",
    "new",
    "points",
    "lines",
    "index",
    "x",
    "y",
    "show",
    "hide",
    "smooth",
    "interp",
    "extrema",
    "delete",
    
    "+=",
    "-=",
    "*=",
    "/=",
    "**=",
    "%=",
    "**",
    "..",
};

constexpr const char *get_mul_char_token_name(Token_enum tkn)
{
    return token_name_table[tkn - 256];
}

std::string get_token_name_str(Token_enum tkn)
{
    if(tkn >= 256)
	return token_name_table[tkn - 256];
    return std::string{char(tkn)};
}


// little bit ugly (I would like to iterate over keywords), but fast
Token_enum keyword_compare(const std::string_view sv)
{
#ifdef TRACY_ALL
    ZoneScoped;
#endif

    // NOTE: There may be collisions for anything longer than 8.
    //       And currently all keywords are smaller than that.
    if (sv.size() > 8)
	return tkn_ident;

    switch(hash_string_view(sv)) {
    case cte_hash_c_str("true"): return tkn_true;
    case cte_hash_c_str("false"): return tkn_false;
    case cte_hash_c_str("fit"): return tkn_fit;
    case cte_hash_c_str("sinusoid"): return tkn_sinusoid;
    case cte_hash_c_str("data"): return tkn_data;
    case cte_hash_c_str("function"): return tkn_function;
    case cte_hash_c_str("new"): return tkn_new;
    case cte_hash_c_str("points"): return tkn_points;
    case cte_hash_c_str("lines"): return tkn_lines;
    case cte_hash_c_str("index"): return tkn_index;
    case cte_hash_c_str("x"): return tkn_x;
    case cte_hash_c_str("y"): return tkn_y;
    case cte_hash_c_str("show"): return tkn_show;
    case cte_hash_c_str("hide"): return tkn_hide;
    case cte_hash_c_str("smooth"): return tkn_smooth;
    case cte_hash_c_str("interp"): return tkn_interp;
    case cte_hash_c_str("extrema"): return tkn_extrema;
    case cte_hash_c_str("delete"): return tkn_delete;
    default: return tkn_ident;
    }
}

std::optional<Token> check_for_operator(char **p, char *eof, Token_enum type, const char *op)
{
    if((*p)+strlen(op) >= eof)
	return {};
    size_t i;
    for(i = 0; i < strlen(op); ++i)
	if(op[i] != (*p)[i])
	    return {};
    (*p) += strlen(op);
    return Token{type, *p - i, *p};
}

static bool is_whitespace(char c)
{
    return c == ' ' || c == '\n' || c == '\t' || c == '\f' || c == '\r' || c == '\v';
}

static bool is_alpha(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

static bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static bool is_simple_char(char c)
{
    return (c >= '!' && c <= '/') || (c >= ':' && c <= '@') || (c >= '[' && c <= '`') || (c >= '{' && c <= '~');
}

bool is_open_bracket(char c)
{
    return c == '(' || c == '[' || c == '{';
}

bool is_close_bracket(char c)
{
    return c == ')' || c == ']' || c == '}';
}

bool is_delim_tkn_left(Token_enum type)
{
    if(type >= 256)
	return false;
    char c = (char)type;
    return is_open_bracket(c) || c == ',';
}

bool is_delim_tkn_right(Token_enum type)
{
    if(type >= 256)
	return false;
    char c = (char)type;
    return is_close_bracket(c) || c == ',';
}

Token::Token(Token_enum type, char *ptr, char* ptr_end, void *data)
    : ptr(ptr), size(ptr_end - ptr), type(type), i(0)
{
    bool d_a = false;
    if (data) {
	switch (type) {
	case tkn_ident:
	case tkn_string: d_a = true; sv = *reinterpret_cast<std::string_view*>(data); break;
	case tkn_int:    d_a = true; i = *reinterpret_cast<int64_t*>(data); break;
	case tkn_real:   d_a = true; d = *reinterpret_cast<double*>(data);  break;
	default: log_error("The token '%s' does not support data.", get_token_name_str(type).c_str());
	}
    }
    
    if(d_a && !data)
	log_error("The data ptr for the token was empty");
}

void Lexer::load_input_from_string(std::string source)
{
    input = source;
    after_load_init();
}

void Lexer::after_load_init()
{
    p = (char*)input.c_str();
    p_begin = p;
    eof = p + input.size();
}

void Lexer::tokenize()
{
    while(tkns.empty() || (tkns.back().type != tkn_eof && tkns.back().type != tkn_parse_error)) {
	get_next_token();
    }
}

static bool float_point_check(char *p, char *eof)
{
    if (*p == '.') {
	if (p + 1 != eof && p[1] == '.')
	    return false;
	return true;
    }
    return false;
}

bool Lexer::get_next_token()
{
    if (p == eof)
	p = nullptr;
    if (!p)
	return push(Token{tkn_eof, eof, eof});
	
    // skip whitespace and comments
    for (;;) {
	while (p != eof && is_whitespace(*p)) {
		++p;
	}
	if (p[0] == '/' && p[1] == '*') {
	    p += 2;
	    while (p != eof && (p[0] != '*' || p[1] != '/'))
		++p;
	    ++p;
	    if (p != eof)
		++p;
	    continue;
	}
	if (p[0] == '/' && p[1] == '/') {
	    p += 2;
	    while (p != eof && (*p != '\n'))
		++p;
	    if(p != eof)
		++p;
	    continue;
	}
	break;
    }
    
    // check for identifiers and keywords
    if (p != eof && (is_alpha(*p))) { // no leading _ or digit
	char* begin = p;
	++p;
	while (p != eof && (is_alpha(*p) || *p == '_' || is_digit(*p)))
	    ++p;
	std::string_view sv(begin, p);
	bool result = push(Token{tkn_ident, begin, p, &sv});
	tkns.back().type = keyword_compare(tkns.back().sv);
	return result;
    }
    
    // TODO: handle INF, INFINITY, NAN, etc.
    // check for numbers
    if (p != eof && (is_digit(*p) || ((float_point_check(p, eof) || (p[0] == '+') || (p[0] == '-')) && is_digit(*(p + 1))))) {
	bool has_point = (*p == '.');
	bool has_exp = false;
	char* begin = p;
	++p;
	while (p != eof && (is_digit(*p) || float_point_check(p, eof) || is_alpha(*p))) {
	    if (has_point && (*p == '.'))
		break;
	    has_point |= (*p == '.');
	    has_exp |= (*p == 'e' || *p == 'E');
	    ++p;
	}
	if (has_point || has_exp) {
	    double number = strtod(begin, &p);
	    if(errno == ERANGE)
		log_error("parsed number was out of range");
	    return push(Token{tkn_real, begin, p, &number});
	}
	else {
	    int64_t number = strtoll(begin, &p, 0);
	    if(errno == ERANGE)
		log_error("parsed number was out of range");
	    return push(Token{tkn_int, begin, p, &number});
	}
    }
    
    // check for strings
    if (p != eof && *p == '\"') {
	++p;
	char* begin = p;
	while (p != eof && *p != '\"')
	    ++p;
	std::string_view sv(begin, p);
	++p;
	return push(Token{tkn_string, begin, p, &sv});
    }
    
    // check for operators | before number check because of operator '..'
    for(uint32_t i = tkn_update_add; i <= tkn_range; ++i) {
	auto opt_tkn = check_for_operator(&p, eof, Token_enum(i), get_mul_char_token_name(Token_enum(i)));
	if(opt_tkn)
	    return push(opt_tkn.value());
    }

    // check for all remaining single chars
    if (p < eof && is_simple_char(*p)) {
	++p;
	return push(Token{Token_enum(*(p-1)), p-1, p});
    }

    if (p == eof) return push(Token{tkn_eof, eof, eof});

    // when stuck, just get the next token.
    parsing_error(tkns.back(), "Failed to parse the character after this token.");
    ++p;
    return get_next_token();
}

void Lexer::print_token(Token &tkn, bool show_content) const
{
    std::cout << '\n';
    if (tkn.type < 256) {
	std::cout << "\033[92m" << (char)tkn.type << "\033[0m";
    }
    else if (show_content) {
	switch (tkn.type) {
	case tkn_ident:
	case tkn_string: std::cout << "\033[91m" << tkn.sv << "\033[0m"; break;
	case tkn_int: std::cout << "\033[94m" << tkn.i << "\033[0m"; break;
	case tkn_real: std::cout << "\033[94m" << tkn.d << "\033[0m"; break;
	default: std::cout << "\033[93m" << get_mul_char_token_name(tkn.type) << "\033[0m";
	}
    }
    else std::cout << "\033[95m" << get_mul_char_token_name(tkn.type) << "\033[0m";
    std::cout << " ";
}
