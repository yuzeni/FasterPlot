#include "command_parser.hpp"

#include "flusssensor_tool.hpp"
#include "function_fitting.hpp"
#include "raylib.h"
#include <cstdint>
#include <iostream>

#include "utils.hpp"
#include "lexer.hpp"

static bool expect_next_token(Lexer &lexer) {

    if(!lexer.next_token()) {
	log_error("Expected another token at the end of the command.");
	return false;
    }
    return true;
}

static Command_Object get_command_object(Data_Manager &data_manager, Lexer &lexer)
{
    Command_Object object;
    object.tkn = lexer.tkn;

    Token object_tkn = lexer.tkn;

    switch(object_tkn.type) {
    case tkn_data:
	if (!expect_next_token(lexer))
	    return {};
	if (lexer.tkn.type != tkn_new && lexer.tkn.type != tkn_int) {
	    lexer.parsing_error(lexer.tkn, "Expected a number or the keyword 'new'.");
	    return {};
	}
	object.type = OT_plot_data;
	if (lexer.tkn.type == tkn_new) {
	    data_manager.plot_data.push_back(new Plot_Data);
	    object.obj.plot_data = data_manager.plot_data.back();
	}
	else if(lexer.tkn.type == tkn_int) {
	    if(lexer.tkn.i < int64_t(data_manager.plot_data.size())) {
		object.obj.plot_data = data_manager.plot_data[lexer.tkn.i];
	    }
	    else {
		lexer.parsing_error(lexer.tkn, "The plot data with index '%d' does not exist.", lexer.tkn.i);
		return {};
	    }
	}
	break;
    case tkn_function:
	if (!expect_next_token(lexer))
	    return {};
	if (lexer.tkn.type != tkn_new && lexer.tkn.type != tkn_int) {
	    lexer.parsing_error(lexer.tkn, "Expected a number or the keyword 'new'.");
	    return {};
	}
	object.type = OT_function;
	if (lexer.tkn.type == tkn_new) {
	    data_manager.functions.push_back(new Function);
	    object.obj.function = data_manager.functions.back();
	}
	else if(lexer.tkn.type == tkn_int) {
	    if(lexer.tkn.i < int64_t(data_manager.functions.size())) {
		object.obj.function = data_manager.functions[lexer.tkn.i];
	    }
	    else {
		lexer.parsing_error(lexer.tkn, "The function with index '%d' does not exist.", lexer.tkn.i);
		return {};
	    }
	}
        break;
    case tkn_int:
    case tkn_real:
	object.type = OT_value;
	break;
    default:
	lexer.parsing_error(object_tkn, "Expected keywords 'data' or 'function'.");
	return {};
    }
    
    return object;
}

static Command_Operator get_command_operator(Lexer& lexer)
{
    Command_Operator op;
    op.tkn = lexer.tkn;
    
    switch(lexer.tkn.type) {
    case '+':
	op.type = OP_add;
	break;
    case '-':
	op.type = OP_sub;
	break;
    case '*':
	op.type = OP_mul;
	break;
    case '/':
	op.type = OP_div;
	break;
    case tkn_update_add:
	op.type = OP_update_add;
	break;
    case tkn_update_sub:
	op.type = OP_update_sub;
	break;
    case tkn_update_mul:
	op.type = OP_update_mul;
	break;
    case tkn_update_div:
	op.type = OP_update_div;
	break;
    case '=':
	op.type = OP_assign;
	break;
    }
    return op;
}

void op_binary_assign(Command_Object object, Command_Object arg_unary, Command_Object arg_binary, double (*op_fun)(double, double))
{
    if (object.type == OT_function) {
	log_error("Can't assign an addition result to a function.");
	return;
    }
    
    if (arg_unary.type == OT_function && arg_binary.type == OT_function) {
	if (!object.obj.plot_data->x) {
	    log_error("Can't combine functions and assign the result to a plot, if the plot does not have an X set.");
	    return;
	}
	for (size_t ix = 0; ix < object.obj.plot_data->x->y.size(); ++ix) {
	    double x = object.obj.plot_data->x->y[ix];
	    object.obj.plot_data->y[ix] = op_fun(arg_unary.obj.function->operator()(x), arg_binary.obj.function->operator()(x));
	}
    }
    else if (arg_unary.type == OT_plot_data && arg_binary.type == OT_plot_data) {
	for (size_t ix = 0; ix < std::min(arg_unary.obj.plot_data->x->y.size(), arg_binary.obj.plot_data->x->y.size()); ++ix) {
	    if(arg_unary.obj.plot_data->x->y[ix] == arg_binary.obj.plot_data->x->y[ix])
		object.obj.plot_data->y[ix] = op_fun(arg_unary.obj.plot_data->y[ix], arg_binary.obj.plot_data->y[ix]);
	}
	object.obj.plot_data->x = arg_unary.obj.plot_data->x->y.size() < arg_binary.obj.plot_data->x->y.size() ? arg_unary.obj.plot_data->x : arg_binary.obj.plot_data->x;
    }
    else {
	Plot_Data* plot_data;
	Function* function;
	if (arg_unary.type == OT_plot_data) {
	    plot_data = arg_unary.obj.plot_data;
	    function = arg_binary.obj.function;
	}
	else {
	    plot_data = arg_binary.obj.plot_data;
	    function = arg_unary.obj.function;
	}
	for (size_t ix = 0; ix < plot_data->x->y.size(); ++ix)
	    object.obj.plot_data->y[ix] = op_fun(plot_data->y[ix], function->operator()(plot_data->x->y[ix]));
	object.obj.plot_data->x = plot_data->x;
    }
}

void op_unary_assign(Command_Object object, Command_Object arg_unary)
{
    if (object.type == OT_plot_data && arg_unary.type == OT_plot_data) {
	object.obj.plot_data->x = arg_unary.obj.plot_data->x;
	object.obj.plot_data->y = arg_unary.obj.plot_data->y;
    }
    else if (object.type == OT_function && arg_unary.type == OT_function) {
	object.obj.function->type = arg_unary.obj.function->type;
	object.obj.function->func = arg_unary.obj.function->func;
    }
    else {
	log_error("Plots and functions cannot be assigned to each other.");
    }
}

static double add_op(double a, double b) { return a + b; }
static double sub_op(double a, double b) { return a - b; }
static double mul_op(double a, double b) { return a * b; }
static double div_op(double a, double b) { return a / b; }

void execute_assign_operation(Command_Object object, Command_Operator op, Command_Object arg_unary, Command_Object arg_binary) {

    switch(op.type) {
    case OP_add:
	op_binary_assign(object, arg_unary, arg_binary, add_op);
	break;
    case OP_sub:
	op_binary_assign(object, arg_unary, arg_binary, sub_op);
	break;
    case OP_mul:
	op_binary_assign(object, arg_unary, arg_binary, mul_op);
	break;
    case OP_div:
	op_binary_assign(object, arg_unary, arg_binary, div_op);
	break;
    case OP_update_add:
	op_binary_assign(object, object, arg_unary, add_op);
	break;
    case OP_update_sub:
	op_binary_assign(object, object, arg_unary, sub_op);
	break;
    case OP_update_mul:
	op_binary_assign(object, object, arg_unary, mul_op);
	break;
    case OP_update_div:
	op_binary_assign(object, object, arg_unary, div_op);
	break;
    case OP_assign:
	op_unary_assign(object, arg_unary);
	break;
    }
}

double op_binary_value(Command_Object arg_unary, Command_Object arg_binary, double (*op_fun)(double, double), Lexer& lexer)
{
    if (arg_unary.type != OT_value) {
	lexer.parsing_error(arg_unary.tkn, "Expected a value.");
	return 0;
    }
    
    if (arg_binary.type != OT_value) {
	lexer.parsing_error(arg_binary.tkn, "Expected a value.");
	return 0;
    }

    double a1 = 0;
    if (arg_unary.tkn.type == tkn_int)
	a1 = arg_unary.tkn.i;
    else if (arg_unary.tkn.type == tkn_real)
	a1 = arg_unary.tkn.d;

    double a2 = 0;
    if (arg_binary.tkn.type == tkn_int)
	a2 = arg_binary.tkn.i;
    else if (arg_binary.tkn.type == tkn_real)
	a2 = arg_binary.tkn.d;

    return op_fun(a1, a2);
}

double execute_operation_value(Command_Operator op, Command_Object arg_unary, Command_Object arg_binary, Lexer& lexer) {

    switch(op.type) {
    case OP_add:
	return op_binary_value(arg_unary, arg_binary, add_op, lexer);
	break;
    case OP_sub:
	return op_binary_value(arg_unary, arg_binary, sub_op, lexer);
	break;
    case OP_mul:
	return op_binary_value(arg_unary, arg_binary, mul_op, lexer);
	break;
    case OP_div:
	return op_binary_value(arg_unary, arg_binary, div_op, lexer);
	break;
    default:
	lexer.parsing_error(op.tkn, "This operation is not supported for values.");
	return 0;
    }
}

void print_unary(Command_Object arg_unary, Lexer& lexer)
{
    if (arg_unary.type == OT_value) {
	if (arg_unary.tkn.type == tkn_int)
	    std::cout << arg_unary.tkn.i << '\n';
	else if (arg_unary.tkn.type == tkn_real)
	    std::cout << arg_unary.tkn.d << '\n';
    }
    else {
	lexer.parsing_error(arg_unary.tkn, "Expected a number.");
    }
}

void print_op_binary(Command_Operator op, Command_Object arg_unary, Command_Object arg_binary, Lexer& lexer)
{
    std::cout << execute_operation_value(op, arg_unary, arg_binary, lexer) << '\n';
}

void handle_command(Data_Manager &data_manager)
{
    if (IsKeyPressed(KEY_ENTER)) {
	std::string cmd;
	std::getline(std::cin, cmd);
	Lexer lexer;
	lexer.load_input_from_string(cmd);

	
	if (!expect_next_token(lexer))
	    return;

	Command_Object object, arg_unary, arg_binary;

	// = expression (print expression)
 	Command_Operator op = get_command_operator(lexer);
	if (op.type == OP_assign) {
	    if (!expect_next_token(lexer))
		return;
	    arg_unary = get_command_object(data_manager, lexer);
	    if (arg_unary.is_undefined()) {
		lexer.parsing_error(lexer.tkn, "Expected at least one argument.");
		return;
	    }
	    if (!expect_next_token(lexer))
		return;
	    op = get_command_operator(lexer);
	    if (op.is_undefined()) {
		print_unary(arg_unary, lexer);
		return;
	    }
	    else {
		if (!expect_next_token(lexer))
		    return;
		arg_binary = get_command_object(data_manager, lexer);
		if (arg_binary.is_undefined()) {
		    lexer.parsing_error(lexer.tkn, "Expected another argument.");
		    return;
		}
		print_op_binary(op, arg_unary, arg_binary, lexer);
	    }
	    return;
	}

	// object = expression (assign expression to object)
	object = get_command_object(data_manager, lexer);
	if (object.is_undefined())
	    return;

	if (!expect_next_token(lexer))
	    return;

	op = get_command_operator(lexer);
	if (op.type == OP_assign) {
	    if (!expect_next_token(lexer))
		return;
	    op = get_command_operator(lexer);
	}
	if (op.is_undefined()) {
	    op.type = OP_assign;
	}

	

	if (op_arg_cnt_table[op.type] >= 1) {
	    if (!expect_next_token(lexer))
		return;
	    arg_unary = get_command_object(data_manager, lexer);
	    if (arg_unary.is_undefined()) {
		lexer.parsing_error(lexer.tkn, "Expected a unary argument for the operator '%s'.", operator_type_name_table[op.type]);
		return;
	    }
	}
	if (op_arg_cnt_table[op.type] >= 2) {
	    if (!expect_next_token(lexer))
		return;
	    arg_binary = get_command_object(data_manager, lexer);
	    if (arg_unary.is_undefined()) {
		lexer.parsing_error(lexer.tkn, "Expected a unary argument for the operator '%s'.", operator_type_name_table[op.type]);
		return;
	    }
	}

	execute_assign_operation(object, op, arg_unary, arg_binary);
    }
}
