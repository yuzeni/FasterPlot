#pragma once

#include <array>
#include <cstddef>

#include "lexer.hpp"

class Generic_Function;

struct Op_Tree_Node
{
    Op_Tree_Node(Token tkn) : tkn(tkn) {}
    ~Op_Tree_Node()
     {
	if (left) {
	    delete left;
	}
	if (right) {
	    delete right;
	}
    }
    Token tkn;
    Op_Tree_Node *left = nullptr;
    Op_Tree_Node *right = nullptr;
    size_t param_idx;
    double const_value;
};

struct Function_Op_Tree
{
    Op_Tree_Node *base_node = nullptr;

    double evaluate(const Generic_Function& generic_function, double x) const;
    std::string get_string_no_value(const Generic_Function& generic_function) const;
    std::string get_string_value(const Generic_Function& generic_function) const;

private:
    
    double evaluate_node(const Generic_Function& generic_function, double x, Op_Tree_Node* node) const;
    void stringify_op_tree(const Generic_Function& generic_function, std::string &str,
			   Op_Tree_Node* node, bool show_values) const;
};

#define NUD_ARGS [[maybe_unused]] Token_enum tkn_type, [[maybe_unused]] Lexer &lexer, \
	[[maybe_unused]] Op_Tree_Node *left, [[maybe_unused]] Generic_Function& generic_function
#define LED_ARGS [[maybe_unused]] Token_enum tkn_type, [[maybe_unused]] Lexer &lexer, \
	[[maybe_unused]] Op_Tree_Node *left, [[maybe_unused]] Generic_Function& generic_function

/* tdop parsing functions */

Op_Tree_Node *nud_error(NUD_ARGS);
Op_Tree_Node *nud_ident(NUD_ARGS);
Op_Tree_Node *nud_int(NUD_ARGS);
Op_Tree_Node *nud_real(NUD_ARGS);
Op_Tree_Node *nud_arg(NUD_ARGS);
Op_Tree_Node *nud_right(NUD_ARGS);
Op_Tree_Node *nud_parenthesis(NUD_ARGS);
Op_Tree_Node *nud_delimiter(NUD_ARGS);

Op_Tree_Node *led_error(LED_ARGS);
Op_Tree_Node *led_normal(LED_ARGS);

/* operator execution fucntions */

#define EXE_ARGS [[maybe_unused]] double left, [[maybe_unused]] double right

inline double exe_error(EXE_ARGS)
{
    logger.log_error("Error during function execution.");
    return 0;
}

inline double exe_or(EXE_ARGS)         { return left || right; }
inline double exe_and(EXE_ARGS)        { return left && right; }
inline double exe_eq(EXE_ARGS)         { return left == right; }
inline double exe_neq(EXE_ARGS)        { return left != right; }
inline double exe_greater(EXE_ARGS)    { return left > right; }
inline double exe_less(EXE_ARGS)       { return left < right; }
inline double exe_less_eq(EXE_ARGS)    { return left <= right; }
inline double exe_greater_eq(EXE_ARGS) { return left >= right; }
inline double exe_add(EXE_ARGS)        { return left + right; }
inline double exe_add_unary(EXE_ARGS)  { return +right; } // this has no effect, maybe delete unary +
inline double exe_sub(EXE_ARGS)        { return left - right; }
inline double exe_sub_unary(EXE_ARGS)  { return -right; }
inline double exe_mul(EXE_ARGS)        { return left * right; }
inline double exe_div(EXE_ARGS)        { return left / right; }
inline double exe_pow(EXE_ARGS)        { return std::pow(left, right); }
inline double exe_not(EXE_ARGS)        { return !right; }
inline double exe_sin(EXE_ARGS)        { return std::sin(right); }
inline double exe_cos(EXE_ARGS)        { return std::cos(right); }
inline double exe_tan(EXE_ARGS)        { return std::tan(right); }
inline double exe_asin(EXE_ARGS)       { return std::asin(right); }
inline double exe_acos(EXE_ARGS)       { return std::acos(right); }
inline double exe_atan(EXE_ARGS)       { return std::atan(right); }
inline double exe_sinh(EXE_ARGS)       { return std::sinh(right); }
inline double exe_cosh(EXE_ARGS)       { return std::cosh(right); }
inline double exe_tanh(EXE_ARGS)       { return std::tanh(right); }
inline double exe_asinh(EXE_ARGS)      { return std::asinh(right); }
inline double exe_acosh(EXE_ARGS)      { return std::acosh(right); }
inline double exe_atanh(EXE_ARGS)      { return std::atanh(right); }

struct Semantic_code {
    int lbp = 0; // left-binding-power
    int rbp = 0; // right-binding-power
                 // zero means none are taken
                 // specifying lbp as well as rbp is not necessary, but solves left and right associativity problems,
                 // where lbp <= rbp guarantees left- and lbp > rbp right associativity.

    Op_Tree_Node* (*led)(LED_ARGS) = led_error;
    Op_Tree_Node* (*nud)(NUD_ARGS) = nud_error;
    
    double (*exe_led)(EXE_ARGS) = exe_error;
    double (*exe_nud)(EXE_ARGS) = exe_error;
};

consteval std::array<Semantic_code, tkn_SIZE> get_tkn_semantics_table()
{
    std::array<Semantic_code, tkn_SIZE> table;
    
    table.fill({});

    /* literals */
    table[tkn_ident]       = {0, 0, led_error, nud_ident };
    table[tkn_int]         = {0, 0, led_error, nud_int };
    table[tkn_real]        = {0, 0, led_error, nud_real };
    table[tkn_string]      = {0, 0, led_error, nud_arg };
    table[tkn_true]        = {0, 0, led_error, nud_arg };
    table[tkn_false]       = {0, 0, led_error, nud_arg };
    table[tkn_x]           = {0, 0, led_error, nud_arg };
    table[tkn_y]           = {0, 0, led_error, nud_arg };
    table[tkn_pi]          = {0, 0, led_error, nud_arg };
    table[tkn_euler]       = {0, 0, led_error, nud_arg };

    /* set operations */
    table['=']             = {6, 6,   led_normal, nud_error };
    table[tkn_or]          = {7, 7,   led_normal, nud_error, exe_or};
    table[tkn_and]         = {8, 8,   led_normal, nud_error, exe_and};
    table[tkn_eq]          = {9, 9,   led_normal, nud_error, exe_eq};
    table[tkn_neq]         = {9, 9,   led_normal, nud_error, exe_neq};
    table['<']             = {9, 9,   led_normal, nud_error, exe_less};
    table['>']             = {9, 9,   led_normal, nud_error, exe_greater};
    table[tkn_less_eq]     = {9, 9,   led_normal, nud_error, exe_less_eq};
    table[tkn_greater_eq]  = {9, 9,   led_normal, nud_error, exe_greater_eq};
    table['+']             = {10, 10, led_normal, nud_right, exe_add, exe_add_unary};
    table['-']             = {10, 10, led_normal, nud_right, exe_sub, exe_sub_unary};
    table['*']             = {11, 11, led_normal, nud_error, exe_mul};
    table['/']             = {11, 11, led_normal, nud_error, exe_div};
    table[tkn_pow]         = {13, 12, led_normal, nud_error, exe_pow};
    table['!']             = {0, 14,  led_error,  nud_right, exe_error, exe_not};
    table[tkn_sin]         = {0, 15,  led_error,  nud_right, exe_error, exe_sin};
    table[tkn_cos]         = {0, 15,  led_error,  nud_right, exe_error, exe_cos};
    table[tkn_tan]         = {0, 15,  led_error,  nud_right, exe_error, exe_tan};
    table[tkn_asin]        = {0, 15,  led_error,  nud_right, exe_error, exe_asin};
    table[tkn_acos]        = {0, 15,  led_error,  nud_right, exe_error, exe_acos};
    table[tkn_atan]        = {0, 15,  led_error,  nud_right, exe_error, exe_atan};
    table[tkn_sinh]        = {0, 15,  led_error,  nud_right, exe_error, exe_sinh};
    table[tkn_cosh]        = {0, 15,  led_error,  nud_right, exe_error, exe_cosh};
    table[tkn_tanh]        = {0, 15,  led_error,  nud_right, exe_error, exe_tanh};
    table[tkn_asinh]       = {0, 15,  led_error,  nud_right, exe_error, exe_asinh};
    table[tkn_acosh]       = {0, 15,  led_error,  nud_right, exe_error, exe_acosh};
    table[tkn_atanh]       = {0, 15,  led_error,  nud_right, exe_error, exe_atanh};

    
    /* grouping */
    table['(']             = {15, 0, led_error, nud_parenthesis};
    table[')']             = {0, 0,  led_error, nud_delimiter};
    
    return table;
}

inline constexpr auto tkn_semantics_table = get_tkn_semantics_table();

Op_Tree_Node *parse_expression(Lexer &lexer, Generic_Function &generic_function, int rbp = 0);
