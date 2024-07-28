
#include "command_parser.hpp"

#include "flusssensor_tool.hpp"
#include "function_fitting.hpp"
#include "raylib.h"
#include <cstdint>
#include <iostream>

#include "utils.hpp"
#include "lexer.hpp"
#include "object_operations.hpp"

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
	switch (lexer.tkn.type) {
	case tkn_new:
	    data_manager.new_plot_data();
	    object.obj.plot_data = data_manager.plot_data.back();
	    break;
	case tkn_int:
	    if(lexer.tkn.i < int64_t(data_manager.plot_data.size())) {
		object.obj.plot_data = data_manager.plot_data[lexer.tkn.i];
	    }
	    else {
		lexer.parsing_error(lexer.tkn, "The plot data with index '%d' does not exist.", lexer.tkn.i);
		return {};
	    }
	    if (lexer.next_tkn.type == tkn_x) {
		object.type = OT_plot_data_ptr;
		object.obj.plot_data_ptr = &object.obj.plot_data->x;
		lexer.next_token();
	    }
	    break;
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
	    data_manager.new_function();
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
    case tkn_sinusoid:
	object.type = OT_value;
	break;
    default:
	return {};
    }
    
    return object;
}

static Command_Operator get_command_operator(Token tkn)
{
    Command_Operator op;
    op.tkn = tkn;
    
    switch(tkn.type) {
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
    case tkn_smooth:
	op.type = OP_smooth;
	break;
    case tkn_interp:
	op.type = OP_interp;
	break;
    case tkn_fit:
	op.type = OP_fit;
	break;
    case tkn_hide:
	op.type = OP_hide;
	break;
    case tkn_show:
	op.type = OP_show;
	break;
    case tkn_extrema:
	op.type = OP_extrema;
	break;
    }
    return op;
}

void op_binary_assign(Lexer& lexer, Command_Object object, Command_Operator op, Command_Object arg_unary, Command_Object arg_binary, double (*op_fun)(double, double))
{
    if (object.type == OT_function) {
	log_error("Can't assign an addition result to a function.");
	return;
    }

    object.obj.plot_data->y.clear();
    
    if (arg_unary.type == OT_function && arg_binary.type == OT_function) {
	if (!object.obj.plot_data->x) {
	    log_error("Can't combine functions and assign the result to a plot, if the plot does not have an X set.");
	    return;
	}
	for (size_t ix = 0; ix < object.obj.plot_data->x->y.size(); ++ix) {
	    double x = object.obj.plot_data->x->y[ix];
	    object.obj.plot_data->y.push_back(op_fun(arg_unary.obj.function->operator()(x), arg_binary.obj.function->operator()(x)));
	}
    }
    else if (arg_unary.type == OT_plot_data && arg_binary.type == OT_plot_data) {
	if (arg_unary.obj.plot_data->x != arg_binary.obj.plot_data->x) {
	    lexer.parsing_error(op.tkn, "The two data do not share the same X. Therefore they can't be combined.");
	    return;
	}
	Plot_Data* x = arg_unary.obj.plot_data->x;
	for (size_t ix = 0; ix < x->y.size(); ++ix) {
	    object.obj.plot_data->y.push_back(op_fun(arg_unary.obj.plot_data->y[ix], arg_binary.obj.plot_data->y[ix]));
	}
	object.obj.plot_data->x = x;
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
	    object.obj.plot_data->y.push_back(op_fun(plot_data->y[ix], function->operator()(plot_data->x->y[ix])));
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
    else if (object.type == OT_plot_data_ptr && arg_unary.type == OT_plot_data) {
	*object.obj.plot_data_ptr = arg_unary.obj.plot_data;
    }
    else {
	log_error("Plots and functions cannot be assigned to each other.");
    }
}

void op_fit_assign(Lexer& lexer, Command_Object object, Command_Operator op, Command_Object arg_unary, Command_Object arg_binary) {

    if (object.type != OT_function) {
	lexer.parsing_error(object.tkn, "Expected function, but got '%s'.", object_type_name_table[object.type]);
	return;
    }

    if (arg_binary.type != OT_plot_data) {
	lexer.parsing_error(arg_binary.tkn, "The argument '%s' is not supported by the operation '%s'",
			    object_type_name_table[arg_binary.type], operator_type_name_table[op.type]);
	return;
    }

    object.obj.function->fit_from_data = arg_binary.obj.plot_data;

    if (arg_unary.tkn.type == tkn_sinusoid) {
	fit_sinusoid_plot_data(arg_binary.obj.plot_data, object.obj.function);
    }
    else {
	lexer.parsing_error(arg_unary.tkn, "The argument '%s' is not supported by the operation '%s'",
			    get_token_name_str(arg_unary.tkn.type).c_str(), operator_type_name_table[op.type]);
	return;
    }
}

void op_extrema_assign(Lexer& lexer, Data_Manager& data_manager, Command_Object object, Command_Operator op, Command_Object arg_unary, Command_Object arg_binary) {

    if (object.type != OT_plot_data) {
	lexer.parsing_error(object.tkn, "Expected data, but got '%s'.", object_type_name_table[object.type]);
	return;
    }

    if (arg_unary.type != OT_plot_data) {
	lexer.parsing_error(arg_unary.tkn, "Expected data, but got '%s'.", object_type_name_table[arg_unary.type]);
	return;
    }    

    if (arg_binary.type != OT_value || arg_binary.tkn.type != tkn_int) {
	lexer.parsing_error(arg_binary.tkn, "The argument type '%s' is not supported by the operation '%s'",
			    get_token_name_str(arg_binary.tkn.type).c_str(), operator_type_name_table[op.type]);
	return;
    }

    data_manager.new_plot_data();
    object.obj.plot_data->x = data_manager.plot_data.back();
    
    get_extrema_plot_data(object.obj.plot_data, arg_unary.obj.plot_data, arg_binary.tkn.i);
}

void execute_assign_operation(Lexer& lexer, Data_Manager& data_manager, Command_Object object, Command_Operator op, Command_Object arg_unary, Command_Object arg_binary) {

    switch(op.type) {
    case OP_add:
	op_binary_assign(lexer, object, op, arg_unary, arg_binary, add_op);
	break;
    case OP_sub:
	op_binary_assign(lexer, object, op, arg_unary, arg_binary, sub_op);
	break;
    case OP_mul:
	op_binary_assign(lexer, object, op, arg_unary, arg_binary, mul_op);
	break;
    case OP_div:
	op_binary_assign(lexer, object, op, arg_unary, arg_binary, div_op);
	break;
    case OP_update_add:
	op_binary_assign(lexer, object, op, object, arg_unary, add_op);
	break;
    case OP_update_sub:
	op_binary_assign(lexer, object, op, object, arg_unary, sub_op);
	break;
    case OP_update_mul:
	op_binary_assign(lexer, object, op, object, arg_unary, mul_op);
	break;
    case OP_update_div:
	op_binary_assign(lexer, object, op, object, arg_unary, div_op);
	break;
    case OP_assign:
	op_unary_assign(object, arg_unary);
	break;
    case OP_fit:
	op_fit_assign(lexer, object, op, arg_unary, arg_binary);
	break;
    case OP_extrema:
	op_extrema_assign(lexer, data_manager, object, op, arg_unary, arg_binary);
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

static double execute_operation_value(Command_Operator op, Command_Object arg_unary, Command_Object arg_binary, Lexer& lexer) {

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

static void print_unary(Command_Object arg_unary, Lexer& lexer)
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

static void print_op_binary(Command_Operator op, Command_Object arg_unary, Command_Object arg_binary, Lexer& lexer)
{
    std::cout << execute_operation_value(op, arg_unary, arg_binary, lexer) << '\n';
}

static Command_Object expect_command_object(Data_Manager& data_manager, Lexer& lexer) {

    if (!expect_next_token(lexer))
	return {};
    Command_Object arg = get_command_object(data_manager, lexer);
    if (arg.is_undefined()) {
	lexer.parsing_error(lexer.tkn, "Expected an argument.");
	return {};
    }
    return arg;
}

void handle_command(Data_Manager &data_manager, std::string cmd)
{
    // if (IsKeyPressed(KEY_ENTER)) {
	// std::string cmd;
	// std::getline(std::cin, cmd);
    Lexer lexer;
    lexer.load_input_from_string(cmd);
	
    if (!expect_next_token(lexer))
	return;

    Command_Object object, arg_unary, arg_binary;

    // = expression (print expression)
    Command_Operator op = get_command_operator(lexer.tkn);
    switch(op.type) {
    case OP_assign:
	arg_unary = expect_command_object(data_manager, lexer);
	if (arg_unary.is_undefined())
	    return;
	    
	if (!expect_next_token(lexer))
	    return;
	op = get_command_operator(lexer.tkn);
	if (op.is_undefined()) {
	    print_unary(arg_unary, lexer);
	    return;
	}
	else {
	    arg_binary = expect_command_object(data_manager, lexer);
	    if (arg_binary.is_undefined())
		return;
	    print_op_binary(op, arg_unary, arg_binary, lexer);
	}
	return;
    case OP_smooth:
    case OP_interp:
	arg_unary = expect_command_object(data_manager, lexer);
	if (arg_unary.is_undefined())
	    return;

	if (arg_unary.type != OT_plot_data) {
	    lexer.parsing_error(lexer.tkn, "Expected data, but got '%s'.", object_type_name_table[arg_unary.type]);
	    return;
	}
	    
	arg_binary = expect_command_object(data_manager, lexer);
	if (arg_binary.is_undefined())
	    return;
	if (arg_binary.type != OT_value || arg_binary.tkn.type != tkn_int) {
	    lexer.parsing_error(lexer.tkn, "Expected an integer value, but got '%s' '%s'.",
				object_type_name_table[arg_unary.type], get_token_name_str(arg_binary.tkn.type).c_str());
	    return;
	}

	if (op.type == OP_smooth) {
	    if (!smooth_plot_data(arg_unary.obj.plot_data, arg_binary.tkn.i))
		lexer.parsing_error(op.tkn, "Error occured trying to smooth the data, perhaps X or Y were empty.");
	}
	else if (op.type == OP_interp) {
	    if (!interp_plot_data(data_manager, arg_unary.obj.plot_data, arg_binary.tkn.i))
		lexer.parsing_error(op.tkn, "Error occured trying to interpolate the data, perhaps X or Y were empty.");
	}
	return;
    case OP_show:
    case OP_hide:
	arg_unary = expect_command_object(data_manager, lexer);
	if (arg_unary.is_undefined())
	    return;
	if (arg_unary.type != OT_plot_data && arg_unary.type != OT_function) {
	    lexer.parsing_error(lexer.tkn, "Expected data or function, but got '%s'.", object_type_name_table[arg_unary.type]);
	    return;
	}

	if (arg_unary.type == OT_plot_data)
	    arg_unary.obj.plot_data->info.visible = op.type == OP_show ? true : false;
	else if (arg_unary.type == OT_function)
	    arg_unary.obj.function->info.visible = op.type == OP_show ? true : false;
	return;
    }

    // object = expression (assign expression to object)
    object = get_command_object(data_manager, lexer);
    if (object.is_undefined()) {
	lexer.parsing_error(object.tkn, "Undefined object.");
	return;
    }

    if (!expect_next_token(lexer))
	return;

    op = get_command_operator(lexer.tkn);
    if (get_command_operator(lexer.next_tkn).type != OP_undefined) {
	if (!expect_next_token(lexer))
	    return;
	op = get_command_operator(lexer.tkn);
    }

    if (op_arg_cnt_table[op.type] >= 1) {
	arg_unary = expect_command_object(data_manager, lexer);
	if (arg_unary.is_undefined()) {
	    lexer.parsing_error(arg_unary.tkn, "Undefined object.");
	    return;
	}
    }
    if (op_arg_cnt_table[op.type] >= 2) {
	arg_binary = expect_command_object(data_manager, lexer);
	if (arg_binary.is_undefined()) {
	    lexer.parsing_error(arg_unary.tkn, "Undefined object.");
	    return;
	}
    }

    execute_assign_operation(lexer, data_manager, object, op, arg_unary, arg_binary);

}
