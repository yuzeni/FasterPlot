#include "function_parsing.hpp"

#include <limits>
#include <string>

#include "functions.hpp"
#include "lexer.hpp"
#include "utils.hpp"

/* Parsing **************************/

// call only on a unary op or argument
Op_Tree_Node *parse_expression(Lexer &lexer, Generic_Function& generic_function, int rbp)
{
    Op_Tree_Node* left = nullptr;
    Semantic_code tkn_sema;
    
    if (lexer.tkn(0).type == tkn_eof) {
	goto exit;
    }
    
    tkn_sema = tkn_semantics_table[lexer.tkn(0).type];
    left = tkn_sema.nud(lexer.tkn(0).type, lexer, nullptr, generic_function);
    
    if(lexer.tkn(0).type == tkn_eof || !left) {
	goto exit;
    }
    
    tkn_sema = tkn_semantics_table[lexer.tkn(0).type];
    while(rbp < tkn_sema.lbp )
    {
	Op_Tree_Node* new_left = nullptr;
	new_left = tkn_sema.led(lexer.tkn(0).type, lexer, left, generic_function);
	left = new_left;
	
	if(lexer.tkn(0).type == tkn_eof || !left) {
	    goto exit;
	}
	
	tkn_sema = tkn_semantics_table[lexer.tkn(0).type];
    }

exit:
    if (left == nullptr) {
	lexer.parsing_error(lexer.tkn(0), "Encountered an error.");
    }
    return left;
}

Op_Tree_Node *nud_error(NUD_ARGS)
{
    lexer.parsing_error(lexer.tkn(0), "The token '%s' has no unary method.", get_token_name_str(tkn_type).c_str());
    ++lexer.tkn_idx;
    return nullptr;
}

Op_Tree_Node *nud_ident(NUD_ARGS)
{
    Op_Tree_Node* node = new Op_Tree_Node{lexer.tkn(0)};
    ++lexer.tkn_idx;
    
    int param_idx = generic_function.get_parameter_idx(node->tkn.sv);
    if (param_idx == -1) {
	generic_function.params.push_back({ .val = std::numeric_limits<double>().quiet_NaN(),
		                            .name = std::string(node->tkn.sv) });
	node->param_idx = generic_function.params.size() - 1;
    }
    else {
	node->param_idx = param_idx;
    }
    
    return node;
}

Op_Tree_Node *nud_int(NUD_ARGS)
{
    Op_Tree_Node* node = new Op_Tree_Node{lexer.tkn(0)};
    node->const_value = lexer.tkn(0).i;
    ++lexer.tkn_idx;
    return node;
}

Op_Tree_Node *nud_real(NUD_ARGS)
{
    Op_Tree_Node* node = new Op_Tree_Node{lexer.tkn(0)};
    node->const_value = lexer.tkn(0).d;
    ++lexer.tkn_idx;
    return node;
}

Op_Tree_Node *nud_arg(NUD_ARGS)
{
    Op_Tree_Node* node = new Op_Tree_Node{lexer.tkn(0)};
    ++lexer.tkn_idx;
    return node;
}

Op_Tree_Node *nud_right(NUD_ARGS)
{
    Op_Tree_Node* node = new Op_Tree_Node{lexer.tkn(0)};
    ++lexer.tkn_idx;
    node->right = parse_expression(lexer, generic_function, tkn_semantics_table[tkn_type].rbp);
    return node;
}

Op_Tree_Node *nud_parenthesis(NUD_ARGS)
{
    ++lexer.tkn_idx;
    Op_Tree_Node* content = parse_expression(lexer, generic_function);
    if (content) {
	++lexer.tkn_idx;
	return content;
    }
    return nullptr;
}

Op_Tree_Node *nud_delimiter(NUD_ARGS)
{
    ++lexer.tkn_idx;
    return nullptr;
}

Op_Tree_Node *led_error(LED_ARGS)
{
    lexer.parsing_error(lexer.tkn(0), "The token '%s' has no binary method.", get_token_name_str(tkn_type).c_str());
    ++lexer.tkn_idx;
    return nullptr;
}

Op_Tree_Node *led_normal(LED_ARGS)
{
    Op_Tree_Node* node = new Op_Tree_Node{lexer.tkn(0)};
    node->left = left;
    ++lexer.tkn_idx;
    node->right = parse_expression(lexer, generic_function, tkn_semantics_table[tkn_type].rbp);
    return node;
}

/* Function Operation Tree **************************/

std::string Function_Op_Tree::get_string_no_value(const Generic_Function& generic_function) const
{
    std::string str;
    stringify_op_tree(generic_function, str, base_node, false);
    return str;
}

std::string Function_Op_Tree::get_string_value(const Generic_Function& generic_function) const
{
    std::string str;
    stringify_op_tree(generic_function, str, base_node, true);
    return str;    
}

void Function_Op_Tree::stringify_op_tree(const Generic_Function& generic_function, std::string &str, Op_Tree_Node* node, bool show_values) const
{
    auto stringify_value = [&]() {
	switch(node->tkn.type) {
	case tkn_int:
	case tkn_real:
	    str += std::to_string(node->const_value);
	    break;
	case tkn_ident:
	    if (show_values) {
		str += std::to_string(generic_function.params[node->param_idx].val);
	    }
	    else {
		str += generic_function.params[node->param_idx].name;
	    }
	    break;
	case tkn_x:
	    str += 'x';
	    break;
	case tkn_true:
	    str += "true";
	    break;
	case tkn_false:
	    str += "false";
	    break;
	default:
	    str += "error";
	    break;
	}
    };

    bool is_operation = node->left || node->right;

    if (is_operation) {
	str += '(';
    }
     
    if (node->left) {
	stringify_op_tree(generic_function, str, node->left, show_values);
    }
    
    if (is_operation) { // node must be an operation
	str += ' ' + get_token_name_str(node->tkn.type) + ' ';
    }
    else {
	stringify_value();
    }

    if (node->right) {
	stringify_op_tree(generic_function, str, node->right, show_values);
    }

    if (is_operation) {
	str += ')';
    }
}

double Function_Op_Tree::evaluate_node(const Generic_Function& generic_function, double x, Op_Tree_Node* node) const
{
    if (node) {
	Semantic_code tkn_sema = tkn_semantics_table[node->tkn.type];
    
	if (node->left && node->right) {
	    return tkn_sema.exe_led(evaluate_node(generic_function, x, node->left), evaluate_node(generic_function, x, node->right));
	}
	else if (node->left || node->right) {
	    return tkn_sema.exe_nud(evaluate_node(generic_function, x, node->left), evaluate_node(generic_function, x, node->right));
	}
	
	switch(node->tkn.type) {
	case tkn_int:
	case tkn_real:
	    return node->const_value;
	case tkn_ident:
	    return generic_function.params[node->param_idx].val;
	case tkn_x:
	    return x;
	case tkn_true:
	    return 1;
	case tkn_false:
	    return 0;
	case tkn_pi:
	    return UTILS_PI;
	case tkn_euler:
	    return UTILS_EULER;
	default:
	    break;
	}
    }
    
    return std::numeric_limits<double>().quiet_NaN();
}

double Function_Op_Tree::evaluate(const Generic_Function& generic_function, double x) const
{
    return evaluate_node(generic_function, x, base_node);
}
