 
#include "command_parser.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>

#include "function_fitting.hpp"
#include "global_vars.hpp"
#include "utils.hpp"
#include "lexer.hpp"
#include "object_operations.hpp"
#include "data_manager.hpp"

void Command_Object::delete_new_object(Data_Manager& data_manager) {
    if (new_object) {
	switch(type) {
	case OT_plot_data: data_manager.delete_plot_data(obj.plot_data); break;
	case OT_function: data_manager.delete_function(obj.function); break;
	}
    }
}

static void handle_command_impl(Data_Manager &data_manager, Lexer &lexer);

static bool expect_next_token(Lexer &lexer)
{
    ++lexer.tkn_idx;
    if(lexer.get_tokens().empty()) {
	logger.log_error("Expected another token at the end of the command.");
	return false;
    }
    return true;
}

static Command_Object get_command_object(Data_Manager &data_manager, Lexer &lexer)
{
    Command_Object object;
    object.tkn = lexer.tkn();

    Token object_tkn = lexer.tkn();

    switch(object_tkn.type) {
    case tkn_data:
	if (!expect_next_token(lexer))
	    return {};
	if (lexer.tkn().type != tkn_new && lexer.tkn().type != tkn_int) {

	}
	object.type = OT_plot_data;
	switch (lexer.tkn().type) {
	case tkn_new:
	    data_manager.new_plot_data();
	    object.new_object = true;
	    object.obj.plot_data = data_manager.plot_data.back();
	    if (lexer.tkn(1).type == tkn_string) {
		++lexer.tkn_idx;
		object.obj.plot_data->info.header = lexer.tkn(0).sv;
	    }
	    break;
	case tkn_int:
	    if(lexer.tkn().i < int64_t(data_manager.plot_data.size())) {
		object.obj.plot_data = data_manager.plot_data[lexer.tkn().i];
	    }
	    else {
		lexer.parsing_error(lexer.tkn(), "The plot data with index '%d' does not exist.", lexer.tkn().i);
		return {};
	    }
	    if (lexer.tkn(1).type == tkn_x) {
		object.type = OT_plot_data_ptr;
		object.obj.plot_data_ptr = &object.obj.plot_data->x;
		++lexer.tkn_idx;
	    }
	    break;
	case tkn_iterator:
	    if (lexer.tkn(1).type == tkn_x) {
		++lexer.tkn_idx;
		object.type = OT_plot_data_ptr_itr;
		object.obj.plot_data_ptr_itr = new std::vector<Plot_Data**>;
		for (auto it : *(lexer.tkn().itr)) {
		    if (it < int64_t(data_manager.plot_data.size())) {
			object.obj.plot_data_ptr_itr->push_back(&data_manager.plot_data[it]);
		    }
		    else {
			lexer.parsing_error(lexer.tkn(), "The data with index '%d' does not exist", it);
		    }
		}
	    }
	    else {
		object.type = OT_plot_data_itr;
		object.obj.plot_data_itr = new std::vector<Plot_Data*>;
		for (auto it : *(lexer.tkn().itr)) {
		    if (it < int64_t(data_manager.plot_data.size())) {
			object.obj.plot_data_itr->push_back(data_manager.plot_data[it]);
		    }
		    else {
			lexer.parsing_error(lexer.tkn(), "The data with index '%d' does not exist", it);
		    }
		}
	    }
	    break;
	default:
	    lexer.parsing_error(lexer.tkn(), "Expected an integer, the keyword 'new', or an iterator.");
	    return {};
	}
	break;
	
    case tkn_function:
	if (!expect_next_token(lexer))
	    return {};
	object.type = OT_function;
	
	switch(lexer.tkn().type) {
	case tkn_new:
	    data_manager.new_function();
	    object.new_object = true;
	    object.obj.function = data_manager.functions.back();
	    if (lexer.tkn(1).type == tkn_string) {
		++lexer.tkn_idx;
		object.obj.function->info.header = lexer.tkn(0).sv;
	    }
	    break;
	case tkn_int:
	    if (lexer.tkn().i < int64_t(data_manager.functions.size())) {
		if (lexer.tkn(1).type == tkn_int) {
		    object.type = OT_value;
		    object.obj.val = data_manager.functions[lexer.tkn().i]->operator()(lexer.tkn(1).i);
		    ++lexer.tkn_idx;
		}
		else if (lexer.tkn(1).type == tkn_real) {
		    object.type = OT_value;
		    object.obj.val = data_manager.functions[lexer.tkn().i]->operator()(lexer.tkn(1).d);
		    ++lexer.tkn_idx;
		}
		else if (lexer.tkn(1).type == tkn_ident) {
		    object.type = OT_value_ptr;
		    object.obj.val_ptr = data_manager.functions[lexer.tkn().i]->get_parameter_ref(lexer.tkn(1).sv);
		    ++lexer.tkn_idx;
		}
		else {
		    object.obj.function = data_manager.functions[lexer.tkn().i];
		}
	    }
	    else {
		lexer.parsing_error(lexer.tkn(), "The function with index '%d' does not exist.", lexer.tkn().i);
		return {};
	    }
	    break;
	case tkn_iterator:
	    object.type = OT_function_itr;
	    object.obj.function_itr = new std::vector<Function*>;
	    for (auto it : *(lexer.tkn().itr)) {
		if (it < int64_t(data_manager.functions.size()))
		    object.obj.function_itr->push_back(data_manager.functions[it]);
		else
		    lexer.parsing_error(lexer.tkn(), "The function with index '%d' does not exist", it);
	    }
	    break;
	default:
	    lexer.parsing_error(lexer.tkn(), "Expected an integer, the keyword 'new', or an iterator.");
	    return {};
	}
        break;
	
    case tkn_string:
    case tkn_true:
    case tkn_false:
    case tkn_sinusoid:
    case tkn_linear:
    case tkn_points:
    case tkn_lines:
    case tkn_index:
    case tkn_script:
	object.type = OT_token;
	break;
	
    case tkn_int:
	object.type = OT_value;
	object.obj.val = object.tkn.i;
	break;
	
    case tkn_real:
	object.type = OT_value;
	object.obj.val = object.tkn.d;
	break;

    case tkn_x:
	object.type = OT_token;
	if (lexer.tkn(1).type == tkn_points && lexer.tkn(2).type == tkn_int && lexer.tkn(3).type == tkn_data && lexer.tkn(4).type == tkn_int) {
	    object.type = OT_value_ptr;
	    
	    if (lexer.tkn(4).i < 0 || lexer.tkn(4).i > int64_t(data_manager.plot_data.size())) {
		lexer.parsing_error(lexer.tkn(4), "Out of bound.");
		return {};
	    }
	    
	    Plot_Data* plot_data = data_manager.plot_data[lexer.tkn(4).i];
	    if (!plot_data->x) {
		lexer.parsing_error(lexer.tkn(3), "Has no X.");
		return {};
	    }
	    if (lexer.tkn(2).i < 0 || lexer.tkn(2).i > int64_t(plot_data->size())) {
		lexer.parsing_error(lexer.tkn(2), "Out of bound.");
		return {};
	    }
	    
	    object.obj.val_ptr = &plot_data->x->y[lexer.tkn(2).i];
	    
	    lexer.tkn_idx += 4;
	}
	break;
	
    case tkn_y:
	object.type = OT_token;
	if (lexer.tkn(1).type == tkn_points && lexer.tkn(2).type == tkn_int && lexer.tkn(3).type == tkn_data && lexer.tkn(4).type == tkn_int) {
	    object.type = OT_value_ptr;
	    
	    if (lexer.tkn(4).i < 0 || lexer.tkn(4).i > int64_t(data_manager.plot_data.size())) {
		lexer.parsing_error(lexer.tkn(4), "Out of bound.");
		return {};
	    }
	    
	    Plot_Data* plot_data = data_manager.plot_data[lexer.tkn(4).i];
	    if (lexer.tkn(2).i < 0 || lexer.tkn(2).i > int64_t(plot_data->size())) {
		lexer.parsing_error(lexer.tkn(2), "Out of bound.");
		return {};
	    }
	    
	    object.obj.val_ptr = &plot_data->y[lexer.tkn(2).i];
	    
	    lexer.tkn_idx += 4;
	}
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
    case tkn_delete:
	op.type = OP_delete;
	break;
    case tkn_export:
	op.type = OP_export;
	break;
    case tkn_run:
	op.type = OP_run;
	break;
    case tkn_save:
	op.type = OP_save;
	break;
    case tkn_zero:
	op.type = OP_zero;
	break;
    case tkn_help:
	op.type = OP_help;
	break;
    }
    return op;
}

void op_binary_assign(Lexer& lexer, Command_Object object, Command_Operator op, Command_Object arg_unary, Command_Object arg_binary, double (*op_fun)(double, double))
{
    switch(object.type) {
    case OT_plot_data:
	
	if (arg_unary.type == OT_function && arg_binary.type == OT_function) {
	    for (size_t ix = 0; ix < object.obj.plot_data->size(); ++ix) {
		double x = object.obj.plot_data->x ? object.obj.plot_data->x->y[ix] : ix;
		object.obj.plot_data->y[ix] = op_fun(arg_unary.obj.function->operator()(x), arg_binary.obj.function->operator()(x));
	    }
	}
	else if (arg_unary.type == OT_plot_data && arg_binary.type == OT_plot_data) {
	    if (arg_unary.obj.plot_data->x != arg_binary.obj.plot_data->x) {
		lexer.parsing_error(op.tkn, "The two data do not share the same X. Therefore they can't be combined.");
		return;
	    }

	    size_t new_obj_size = std::min(arg_unary.obj.plot_data->size(), arg_binary.obj.plot_data->size());
	    object.obj.plot_data->y.resize(new_obj_size, 0);
	    for (size_t ix = 0; ix < new_obj_size; ++ix) {
		object.obj.plot_data->y.push_back(op_fun(arg_unary.obj.plot_data->y[ix], arg_binary.obj.plot_data->y[ix]));
	    }
	    object.obj.plot_data->x = arg_unary.obj.plot_data->x;
	}
	else {
	    Plot_Data* plot_data;
	    Function* function;
	    if (arg_unary.type == OT_plot_data && arg_binary.type == OT_function) {
		plot_data = arg_unary.obj.plot_data;
		function = arg_binary.obj.function;
	    }
	    else if (arg_unary.type == OT_function && arg_binary.type == OT_plot_data) {
		plot_data = arg_binary.obj.plot_data;
		function = arg_unary.obj.function;
	    }
	    else {
		lexer.parsing_error(object.tkn, "Can't assign the result of this expression to this object.");
		return;
	    }

	    object.obj.plot_data->y.resize(plot_data->size(), 0);
	    for (size_t ix = 0; ix < plot_data->size(); ++ix) {
		object.obj.plot_data->y[ix] = op_fun(plot_data->y[ix], function->operator()(plot_data->x ? plot_data->x->y[ix] : ix));
	    }
	    object.obj.plot_data->x = plot_data->x;
	}	
	break;
	
    case OT_function:
	logger.log_error("Functions not implemented yet.");
	break;
	
    case OT_value_ptr:
	double unary_val;
	if (arg_unary.type == OT_value) {
	    unary_val = arg_unary.obj.val;
	}
	else if (arg_unary.type == OT_value_ptr) {
	    unary_val = *arg_unary.obj.val_ptr;
	}
	else {
	    lexer.parsing_error(arg_unary.tkn, "Expected a value.");
	    break;
	}

	double binary_val;
	if (arg_binary.type == OT_value) {
	    binary_val = arg_binary.obj.val;
	}
	else if (arg_binary.type == OT_value_ptr) {
	    binary_val = *arg_binary.obj.val_ptr;
	}
	else {
	    lexer.parsing_error(arg_unary.tkn, "Expected a value.");
	    break;
	}

	*object.obj.val_ptr = op_fun(unary_val, binary_val);
	break;
    }

}

void op_unary_assign(Lexer& lexer, Command_Object object, Command_Object arg_unary)
{
    switch(object.type) {
    case OT_plot_data:
	if (arg_unary.type == OT_plot_data) {
	    object.obj.plot_data->x = arg_unary.obj.plot_data->x;
	    object.obj.plot_data->y = arg_unary.obj.plot_data->y;
	    return;
	}
	break;
    case OT_function:
	if (arg_unary.type == OT_function) {
	    object.obj.function->type = arg_unary.obj.function->type;
	    object.obj.function->func = arg_unary.obj.function->func;
	    return;
	}
	else if (arg_unary.type == OT_token) {
	    switch (arg_unary.tkn.type) {
	    case tkn_sinusoid:
		object.obj.function->type = FT_sinusoid;
		object.obj.function->init();
		return;
	    case tkn_linear:
		object.obj.function->type = FT_linear;
		object.obj.function->init();
		return;
	    }
	}
    case OT_plot_data_ptr:
	if (arg_unary.type == OT_plot_data) {
	    *object.obj.plot_data_ptr = arg_unary.obj.plot_data;
	    return;
	}
    case OT_value_ptr:
	switch (arg_unary.type) {
	case OT_value_ptr:
	    *object.obj.val_ptr = *arg_unary.obj.val_ptr;
	    return;
	case OT_value:
	    *object.obj.val_ptr = arg_unary.obj.val;
	    return;
	}
    }

    lexer.parsing_error(arg_unary.tkn, "Cannot assign this object.");
}

void op_fit_assign(Lexer &lexer, Command_Object object, Command_Operator op, Command_Object arg_unary, Command_Object arg_binary)
{

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

    if (lexer.tkn(1).type != tkn_int) {
	lexer.parsing_error(lexer.tkn(1), "Expected an integer argument.");
	return;
    }
    ++lexer.tkn_idx;

    switch(arg_unary.tkn.type) {
    case tkn_sinusoid:
	object.obj.function->type = FT_sinusoid;
	break;
    case tkn_linear:
	object.obj.function->type = FT_linear;
	break;
    default:
	lexer.parsing_error(arg_unary.tkn, "The argument '%s' is not supported by the operation '%s'",
			    get_token_name_str(arg_unary.tkn.type).c_str(), operator_type_name_table[op.type]);
	return;
    }

    std::vector<double*> param_list;
    std::vector<double> param_change_rate;
    object.obj.function->get_all_param_ref_and_fit_change_rate(param_list, param_change_rate);
    object.obj.function->fit_to_data(arg_binary.obj.plot_data, lexer.tkn(0).i, param_list, param_change_rate);
}

void op_extrema_assign(Lexer &lexer, Data_Manager &data_manager, Command_Object object, Command_Operator op, Command_Object arg_unary)
{

    if (object.type != OT_plot_data) {
	lexer.parsing_error(object.tkn, "Expected data, but got '%s'.", object_type_name_table[object.type]);
	return;
    }

    if (arg_unary.type != OT_plot_data) {
	lexer.parsing_error(arg_unary.tkn, "Expected data, but got '%s'.", object_type_name_table[arg_unary.type]);
	return;
    }    

    data_manager.new_plot_data();
    object.new_object = true;
    object.obj.plot_data->x = data_manager.plot_data.back();
    
    get_extrema_plot_data(object.obj.plot_data, arg_unary.obj.plot_data);
}

void execute_assign_operation(Lexer &lexer, Data_Manager &data_manager, Command_Object object, Command_Operator op, Command_Object arg_unary, Command_Object arg_binary)
{
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
	op_unary_assign(lexer, object, arg_unary);
	break;
    case OP_fit:
	op_fit_assign(lexer, object, op, arg_unary, arg_binary);
	break;
    case OP_extrema:
	op_extrema_assign(lexer, data_manager, object, op, arg_unary);
	break;
    }
}

double op_binary_value(Command_Object arg_unary, Command_Object arg_binary, double (*op_fun)(double, double), Lexer& lexer)
{

    double unary_val;
    if (arg_unary.type == OT_value) {
	unary_val = arg_unary.obj.val;
    }
    else if (arg_unary.type == OT_value_ptr) {
	unary_val = *arg_unary.obj.val_ptr;
    }
    else {
	lexer.parsing_error(arg_unary.tkn, "Expected a value.");
	return std::numeric_limits<double>().quiet_NaN();
    }

    double binary_val;
    if (arg_binary.type == OT_value) {
	binary_val = arg_binary.obj.val;
    }
    else if (arg_binary.type == OT_value_ptr) {
	binary_val = *arg_binary.obj.val_ptr;
    }
    else {
	lexer.parsing_error(arg_unary.tkn, "Expected a value.");
	return std::numeric_limits<double>().quiet_NaN();
    }

    return op_fun(unary_val, binary_val);
}

static double execute_operation_value(Command_Operator op, Command_Object arg_unary, Command_Object arg_binary, Lexer &lexer)
{

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
    switch(arg_unary.type) {
    case OT_value:
	logger.log_info("%f\n", arg_unary.obj.val);
	break;
    case OT_value_ptr:
	logger.log_info("%f\n", *arg_unary.obj.val_ptr);
	break;
    default:
	lexer.parsing_error(arg_unary.tkn, "Expected a value.");
	break;
    }
}

static void print_op_binary(Command_Operator op, Command_Object arg_unary, Command_Object arg_binary, Lexer& lexer)
{
    logger.log_info("%f\n", execute_operation_value(op, arg_unary, arg_binary, lexer));
}

static Command_Object expect_command_object(Data_Manager& data_manager, Lexer& lexer) {

    if (!expect_next_token(lexer))
	return {};
    Command_Object arg = get_command_object(data_manager, lexer);
    if (arg.is_undefined()) {
	lexer.parsing_error(lexer.tkn(), "Expected an argument.");
	return {};
    }
    return arg;
}

static std::pair<size_t, size_t> expand_iterators(Data_Manager &data_manager, Lexer &lexer, int sub_level)
{
    std::vector<Token> &tkns = lexer.get_tokens();
    size_t iterator_size = 0;
    size_t iterator_error_size = 0; // the number of iterator elements which produced erronious results.

    for (size_t tkn_idx = 0; tkn_idx < tkns.size(); ++tkn_idx) {
	if (tkns[tkn_idx].type == tkn_iterator) {
	    Token iterator = tkns[tkn_idx];
	    iterator.size = iterator.itr->size();
	    
	    tkns.erase(tkns.begin() + tkn_idx);
	    for (int64_t it : *(iterator.itr)) {
		tkns.insert(tkns.begin() + tkn_idx, Token{tkn_int, iterator.ptr, iterator.ptr + iterator.size, &it});
		iterator_error_size += size_t(!handle_command(data_manager, lexer, sub_level + 1));
		++iterator_size;
		tkns.erase(tkns.begin() + tkn_idx);
	    }
	    tkns.insert(tkns.begin() + tkn_idx, iterator); // give the iterator back
	}
    }

    return {iterator_size, iterator_error_size};
}

static bool check_and_skip_newline(std::string &str, size_t& idx)
{
    if (str[idx] == '\n') {
	return true;
    }
    
    if (str[idx] == '\r') {
	++idx;
	while (idx < str.size() && str[idx] == '\r') {
	    ++idx;
	}
	
	if (str[idx] == '\n') {
	    return true;
	}
    }
    return false;
}

static bool check_newline(std::string &str, size_t idx)
{
    return check_and_skip_newline(str, idx);
}

void re_run_all_commands(Data_Manager &data_manager)
{
    for (int64_t i = 0; i <= g_all_commands.get_index(); ++i) {
	const Command& cmd = g_all_commands.get_commands()[i];
	
	Lexer lexer;
	lexer.get_input() = cmd.cmd;
	lexer.tokenize();
	
	if (check_flag(cmd.flags, CF_hidden) || check_flag(cmd.flags, CF_only_on_save)) {
	    logger.log_info(UTILS_BRIGHT_BLACK "not executed" UTILS_END_COLOR " > ");
	    for (auto& tkn : lexer.get_tokens()) {
		lexer.log_token(tkn);
	    }
	    logger.log_info("\n");
	    continue;
	}
		
	handle_command(data_manager, lexer, 0, false);
    }
}

void handle_command_file(Data_Manager &data_manager, std::string file)
{
    std::string cmd;
    size_t i = 0;
    while (i < file.size()) {
	size_t i_begin = i;
	while(i < file.size() && !check_newline(file, i)) {
	    ++i;
	}
	cmd = file.substr(i_begin, i - i_begin);
	check_and_skip_newline(file, i);
	Lexer lexer;
	lexer.get_input() = cmd;
	lexer.tokenize();
	handle_command(data_manager, lexer);
	++i;
    }
}

bool handle_command(Data_Manager &data_manager, Lexer& lexer, int sub_level, bool add_command)
{
    size_t error_cnt = logger.error_cnt;

    // only log the primary command.
    if (sub_level == 0 && add_command) {
	g_all_commands.add(lexer.get_input());
    }

    // indent for every sub level.
    for (int i = 0; i < sub_level; ++i) {
	logger.log_info("  ");
    }
    
    logger.log_info("> ");
    for (auto& tkn : lexer.get_tokens()) {
	lexer.log_token(tkn);
    }
    logger.log_info("\n");
    
    Command_Object object, arg_unary, arg_binary, arg_tertiary;
    Command_Operator op = get_command_operator(lexer.tkn());

    if (op.type != OP_delete && op.type != OP_export) {
	auto expand_result = expand_iterators(data_manager, lexer, sub_level);
	if (expand_result.first) {
	    return true;
	}
    }
    
    switch(op.type) {
    case OP_assign:
	arg_unary = expect_command_object(data_manager, lexer);
	if (arg_unary.is_undefined())
	    goto exit;
	    
	if (!expect_next_token(lexer))
	    goto exit;
	op = get_command_operator(lexer.tkn());
	if (op.is_undefined()) {
	    print_unary(arg_unary, lexer);
	    goto exit;
	}
	else {
	    arg_binary = expect_command_object(data_manager, lexer);
	    if (arg_binary.is_undefined())
		goto exit;
	    print_op_binary(op, arg_unary, arg_binary, lexer);
	}
	goto exit;
	
    case OP_smooth:
    case OP_interp:
	arg_unary = expect_command_object(data_manager, lexer);
	if (arg_unary.is_undefined())
	    goto exit;

	if (arg_unary.type != OT_plot_data) {
	    lexer.parsing_error(lexer.tkn(), "Expected data, but got '%s'.", object_type_name_table[arg_unary.type]);
	    goto exit;
	}
	    
	arg_binary = expect_command_object(data_manager, lexer);
	if (arg_binary.is_undefined())
	    goto exit;
	if (arg_binary.tkn.type != tkn_int) {
	    lexer.parsing_error(lexer.tkn(), "Expected an integer value, but got '%s' '%s'.",
				object_type_name_table[arg_unary.type], get_token_name_str(arg_binary.tkn.type).c_str());
	    goto exit;
	}

	if (op.type == OP_smooth) {
	    if (!smooth_plot_data(arg_unary.obj.plot_data, arg_binary.tkn.i))
		lexer.parsing_error(op.tkn, "Error occured trying to smooth the data, perhaps X or Y were empty.");
	}
	else if (op.type == OP_interp) {
	    if (!interp_plot_data(data_manager, arg_unary.obj.plot_data, arg_binary.tkn.i))
		lexer.parsing_error(op.tkn, "Error occured trying to interpolate the data, perhaps X or Y were empty.");
	}
	goto exit;
	
    case OP_show:
    case OP_hide:
	arg_unary = expect_command_object(data_manager, lexer);
	if (arg_unary.is_undefined())
	    goto exit;
	
	if (arg_unary.type == OT_plot_data) {
	    if (arg_unary.obj.plot_data->info.visible && op.type == OP_show) {
		data_manager.fit_camera_to_plot(arg_unary.obj.plot_data);
	    }
	    else {
		arg_unary.obj.plot_data->info.visible = (op.type == OP_show ? true : false);
	    }
	}
	else if (arg_unary.type == OT_function) {
	    if (arg_unary.obj.plot_data->info.visible && op.type == OP_show) {
		data_manager.fit_camera_to_plot(arg_unary.obj.function);
	    }
	    else {
		arg_unary.obj.function->info.visible = (op.type == OP_show ? true : false);
	    }
	}
	else if (arg_unary.type == OT_token) {
	    arg_binary = expect_command_object(data_manager, lexer);
	    if (arg_binary.is_undefined())
		goto exit;

	    Plot_Type plot_type;
	    switch (arg_unary.tkn.type) {
	    case tkn_points:
		plot_type = PT_DISCRETE;
		break;
	    case tkn_lines:
		plot_type = PT_INTERP_LINEAR;
		break;
	    case tkn_index:
		plot_type = PT_SHOW_INDEX;
		break;
	    default:
		plot_type = Plot_Type(0);
	    }

	    if (arg_binary.type == OT_plot_data) {
		if (op.type == OP_show)
		    arg_binary.obj.plot_data->info.plot_type = add_flag(arg_binary.obj.plot_data->info.plot_type, plot_type);
		else if (op.type == OP_hide)
		    arg_binary.obj.plot_data->info.plot_type = remove_flag(arg_binary.obj.plot_data->info.plot_type, plot_type);
	    }
	    else if (arg_binary.type == OT_function) {
		if (plot_type != PT_SHOW_INDEX) {
		    if (op.type == OP_show)
			arg_binary.obj.function->info.plot_type = add_flag(arg_binary.obj.function->info.plot_type, plot_type);
		    else if (op.type == OP_hide)
			arg_binary.obj.function->info.plot_type = remove_flag(arg_binary.obj.function->info.plot_type, plot_type);
		}
		else {
		    lexer.parsing_error(arg_binary.tkn, "The operator '%' with binary argument '%' doesn't work with '%s'.",
					operator_type_name_table[op.type], object_type_name_table[arg_binary.type], get_token_name_str(arg_unary.tkn.type).c_str());
		}
	    }
	    else {
		lexer.parsing_error(arg_binary.tkn, "The operator '%' doesn't work with '%s'.",
				    operator_type_name_table[op.type], object_type_name_table[arg_binary.type]);
	    }
	}
	else {
	    lexer.parsing_error(arg_unary.tkn, "The operator '%' doesn't work with '%s'.",
				operator_type_name_table[op.type], object_type_name_table[arg_unary.type]);
	}
	goto exit;
	
    case OP_delete:
	arg_unary = expect_command_object(data_manager, lexer);
	if (arg_unary.is_undefined())
	    goto exit;

	switch(arg_unary.type) {
	case OT_plot_data:
	    data_manager.delete_plot_data(arg_unary.obj.plot_data);
	    break;
	    
	case OT_function:
	    data_manager.delete_function(arg_unary.obj.function);
	    break;
	    
	case OT_plot_data_itr:
	    for (auto pd : *(arg_unary.obj.plot_data_itr)) {
		data_manager.delete_plot_data(pd);
	    }
	    break;
	    
	case OT_function_itr:
	    for (auto func : *(arg_unary.obj.function_itr)) {
		data_manager.delete_function(func);
	    }
	    break;
	    
	case OT_token:
	    switch (arg_unary.tkn.type) {
	    case tkn_points:
		if (lexer.tkn(1).type == tkn_iterator) {
		    ++lexer.tkn_idx;
		    std::vector<int64_t>* arg_point_itr = lexer.tkn().itr;
		    std::sort(arg_point_itr->begin(), arg_point_itr->end(), std::less<int64_t>());
		    
		    arg_tertiary = expect_command_object(data_manager, lexer);
		    switch(arg_tertiary.type) {
		    case OT_plot_data:
			if ((*arg_point_itr)[0] >= 0 && arg_point_itr->back() < int64_t(arg_tertiary.obj.plot_data->size())) {
			    arg_tertiary.obj.plot_data->y.erase(arg_tertiary.obj.plot_data->y.begin() + (*arg_point_itr)[0],
								arg_tertiary.obj.plot_data->y.begin() + arg_point_itr->back() + 1);
			    if (arg_tertiary.obj.plot_data->x) {
				
				Plot_Data* new_x = data_manager.new_plot_data();
				new_x->y = arg_tertiary.obj.plot_data->x->y;
			        new_x->y.erase(new_x->y.begin() + (*arg_point_itr)[0], new_x->y.begin() + arg_point_itr->back() + 1);
				arg_tertiary.obj.plot_data->x = new_x;
			    }
			}
			else {
			    lexer.parsing_error(arg_binary.tkn, "Out of bound.");
			}
			break;
		    case OT_plot_data_itr:
			for (auto pd : *(arg_tertiary.obj.plot_data_itr)) {
			    if ((*arg_point_itr)[0] >= 0 && arg_point_itr->back() < int64_t(pd->size())) {
				pd->y.erase(pd->y.begin() + (*arg_point_itr)[0], pd->y.begin() + arg_point_itr->back() + 1);
				
				if (pd->x) {
				    // theses cannot the recognized as new objects, but should be fine, since we are also recording errors.
				    Plot_Data* new_x = data_manager.new_plot_data();
				    new_x->y = pd->x->y;
				    new_x->y.erase(new_x->y.begin() + (*arg_point_itr)[0], new_x->y.begin() + arg_point_itr->back() + 1);
				    pd->x = new_x;
				    
				    // pd->x->y.erase(pd->x->y.begin() + (*arg_point_itr)[0], pd->x->y.begin() + arg_point_itr->back() + 1);
				}
			    }
			    else {
				lexer.parsing_error(arg_binary.tkn, "Out of bound for data '%d'", pd->index);
			    }
			}
			break;
		    default:
			lexer.parsing_error(arg_binary.tkn, "Only data is supported as argument.");
		    }
		}
		else {
		    arg_binary = expect_command_object(data_manager, lexer);
		    if (arg_binary.is_undefined())
			goto exit;
		    if (arg_binary.tkn.type != tkn_int) {
			lexer.parsing_error(arg_binary.tkn, "Expected an integer or iterator argument.");
			goto exit;
		    }
		    
		    arg_tertiary = expect_command_object(data_manager, lexer);
		    switch(arg_tertiary.type) {
		    case OT_plot_data:
			if (arg_binary.tkn.i < int64_t(arg_tertiary.obj.plot_data->size())) {
			    arg_tertiary.obj.plot_data->y.erase(arg_tertiary.obj.plot_data->y.begin() + arg_binary.tkn.i);
			    
			    if (arg_tertiary.obj.plot_data->x) {
				Plot_Data* new_x = data_manager.new_plot_data();
				new_x->y = arg_tertiary.obj.plot_data->x->y;
			        new_x->y.erase(new_x->y.begin() + arg_binary.tkn.i);
				arg_tertiary.obj.plot_data->x = new_x;
				// arg_tertiary.obj.plot_data->x->y.erase(arg_tertiary.obj.plot_data->x->y.begin() + arg_binary.tkn.i);
			    }
			}
			else {
			    lexer.parsing_error(arg_binary.tkn, "Out of bound.");
			}
			break;
		    case OT_plot_data_itr:
			for (auto pd : *(arg_tertiary.obj.plot_data_itr)) {
			    if (arg_binary.tkn.i < int64_t(pd->size())) {
				pd->y.erase(pd->y.begin() + arg_binary.tkn.i);
				
				if (pd->x) {
				    Plot_Data* new_x = data_manager.new_plot_data();
				    new_x->y = pd->x->y;
				    new_x->y.erase(new_x->y.begin() + arg_binary.tkn.i);
				    pd->x = new_x;
				    // pd->x->y.erase(pd->x->y.begin() + arg_binary.tkn.i);
				}
			    }
			    else {
				lexer.parsing_error(arg_binary.tkn, "Out of bound for data '%d'", pd->index);
			    }
			}
			break;
		    default:
			lexer.parsing_error(arg_binary.tkn, "Only data is supported as argument.");
		    }
		}
		break;
	    default:
		lexer.parsing_error(arg_unary.tkn, "Value type '%s', is not supported.", get_token_name_str(arg_unary.tkn.type).c_str());
	    }
	    break;
	    
	default:
	    lexer.parsing_error(arg_unary.tkn, "The operator '%' doesn't work with '%s'.",
				operator_type_name_table[op.type], object_type_name_table[arg_unary.type]);
	}

	goto exit;
	
    case OP_export:
	arg_unary = expect_command_object(data_manager, lexer);
	if (arg_unary.is_undefined())
	    goto exit;
	{
	    std::string file_name = DEFAULT_EXPORT_FILE_NAME;

	    if (lexer.tkn(1).type == tkn_string) {
		++lexer.tkn_idx;
		file_name = lexer.tkn().sv;
	    }

	    switch (arg_unary.type) {
	    case OT_plot_data:
	    {
		std::vector<Plot_Data*> plot_data {arg_unary.obj.plot_data};
		data_manager.export_plot_data(file_name, plot_data);
	    }
	    break;
	    case OT_plot_data_itr:
		data_manager.export_plot_data(file_name, *arg_unary.obj.plot_data_itr);
		break;
	    case OT_function:
	    {
		std::vector<Function*> functions {arg_unary.obj.function};
		data_manager.export_functions(file_name, functions);
	    }
	    break;
	    case OT_function_itr:
		data_manager.export_functions(file_name, *arg_unary.obj.function_itr);
		break;
	    default:
		lexer.parsing_error(arg_unary.tkn, "Can't export this object.");
	    }
	    g_all_commands.add_cmd_flag(CF_only_on_save);
	}
	goto exit;

    case OP_run:
	if (lexer.tkn(1).type != tkn_script) {
	    lexer.parsing_error(lexer.tkn(1), "Expected keyword 'script'.");
	    goto exit;
	}
	++lexer.tkn_idx;
	
	arg_binary = expect_command_object(data_manager, lexer);
	if (arg_binary.is_undefined())
	    goto exit;

	if (arg_binary.tkn.type != tkn_string) {
	    lexer.parsing_error(arg_binary.tkn, "Expected the filename of the script.");
	    goto exit;
	}

	if (sub_level == 0 && add_command) {
	    g_all_commands.pop(); // remove the cmd for running the script
	}
	run_command_file(data_manager, std::string(arg_binary.tkn.sv));
	goto exit;
	
    case OP_save:
	if (lexer.tkn(1).type != tkn_script) {
	    lexer.parsing_error(lexer.tkn(1), "Expected keyword 'script'.");
	    goto exit;
	}
	++lexer.tkn_idx;
	{
	    std::string file_name = DEFAULT_SAVE_FILE_NAME;
	    if (lexer.tkn(1).type == tkn_string) {
		++lexer.tkn_idx;
		file_name = lexer.tkn().sv;
	    }

	    if (sub_level == 0 && add_command) {
		g_all_commands.pop(); // remove the cmd for saving the script
	    }
	    save_command_file(file_name);
	}
	goto exit;

    case OP_zero:
	data_manager.zero_coord_sys_origin();
	goto exit;
	
    case OP_help:
	logger.log_help_message();
	goto exit;
	
    case OP_fit:
	arg_unary = expect_command_object(data_manager, lexer);
	if (arg_unary.is_undefined())
	    goto exit;
	
	if (arg_unary.type != OT_function) {
	    lexer.parsing_error(object.tkn, "Expected function, but got '%s'.", object_type_name_table[object.type]);
	    goto exit;
	}

	arg_binary = expect_command_object(data_manager, lexer);
	if (arg_binary.is_undefined())
	    goto exit;

	if (arg_binary.type != OT_plot_data) {
	    lexer.parsing_error(arg_binary.tkn, "The argument '%s' is not supported by the operation '%s'",
				object_type_name_table[arg_binary.type], operator_type_name_table[op.type]);
	    goto exit;
	}
	
	arg_unary.obj.function->fit_from_data = arg_binary.obj.plot_data;

	if (lexer.tkn(1).type != tkn_int) {
	    lexer.parsing_error(lexer.tkn(1), "Expected an integer argument.");
	    goto exit;
	}
	++lexer.tkn_idx;
	int iterations = lexer.tkn(0).i;

	std::vector<double*> param_list;
	std::vector<double> param_change_rate;

	while(lexer.tkn(1).type == tkn_ident)
	{
	    ++lexer.tkn_idx;
	    int param_idx = arg_unary.obj.function->get_parameter_idx(lexer.tkn(0).sv);
	    if (param_idx < 0) {
		lexer.parsing_error(lexer.tkn(0), "This parameter does not exist.");
	    }
	    
	    param_list.push_back(arg_unary.obj.function->get_parameter_ref(param_idx));
	    param_change_rate.push_back(arg_unary.obj.function->get_fit_parameter_change_rate(param_idx));
	}

	if (param_list.empty()) {
	    arg_unary.obj.function->get_all_param_ref_and_fit_change_rate(param_list, param_change_rate);
	}

	arg_unary.obj.function->fit_to_data(arg_binary.obj.plot_data, iterations, param_list, param_change_rate, false);
	goto exit;
    }

    // object = op unary binary (assign expression to object)
    object = get_command_object(data_manager, lexer);
    if (object.is_undefined()) {
	lexer.parsing_error(object.tkn, "Undefined object.");
	goto exit;
    }

    if (!expect_next_token(lexer))
	goto exit;

    op = get_command_operator(lexer.tkn());
    if (get_command_operator(lexer.tkn(1)).type != OP_undefined) {
	if (!expect_next_token(lexer))
	    goto exit;
	op = get_command_operator(lexer.tkn());
    }

    arg_unary = expect_command_object(data_manager, lexer);
    if (arg_unary.is_undefined()) {
	lexer.parsing_error(arg_unary.tkn, "Undefined object.");
	goto exit;
    }
    
    if (get_command_operator(lexer.tkn(1)).type != OP_undefined) {
	if (!expect_next_token(lexer))
	    goto exit;
	if (op.type != OP_assign) {
	    lexer.parsing_error(lexer.tkn(), "This operator is unexpected.");
	    goto exit;
	}
	op = get_command_operator(lexer.tkn());
    }
    
    if (op_arg_cnt_table[op.type] >= 2) {
	arg_binary = expect_command_object(data_manager, lexer);
	if (arg_binary.is_undefined()) {
	    lexer.parsing_error(arg_unary.tkn, "Undefined object.");
	    goto exit;
	}
    }

    execute_assign_operation(lexer, data_manager, object, op, arg_unary, arg_binary);

exit:

    object.delete_iterator();
    arg_unary.delete_iterator();
    arg_binary.delete_iterator();
    arg_tertiary.delete_iterator();

    if (error_cnt != logger.error_cnt)
    {
	object.delete_new_object(data_manager);
	arg_unary.delete_new_object(data_manager);
	arg_binary.delete_new_object(data_manager);
	arg_tertiary.delete_new_object(data_manager);
    }
    
    lexer.tkn_idx = 0;
    data_manager.update_references();
    return error_cnt == logger.error_cnt;
}
