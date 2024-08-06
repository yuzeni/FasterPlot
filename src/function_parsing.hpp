#pragma once

#include <array>
#include <cstddef>

#include "lexer.hpp"

class Generic_Function;

struct Op_Tree_Node
{
    Op_Tree_Node(Token tkn) : tkn(tkn) {}
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

    /* grouping */
    table['(']             = {15, 0, led_error, nud_parenthesis};
    table[')']             = {0, 0,  led_error, nud_delimiter};
    
    return table;
}

inline constexpr auto tkn_semantics_table = get_tkn_semantics_table();

Op_Tree_Node *parse_expression(Lexer &lexer, Generic_Function &generic_function, int rbp = 0);

