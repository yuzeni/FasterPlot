#pragma once

#include "data_manager.hpp"
#include "lexer.hpp"

struct Data_Manager;

enum Object_Type
{
    OT_undefined,
    OT_plot_data,
    OT_function,
    OT_token,
    OT_plot_data_ptr,
    OT_plot_data_itr,
    OT_function_itr,
    OT_plot_data_ptr_itr,
    OT_value,
    OT_value_ptr,
    OT_SIZE, 
};

inline const char *object_type_name_table[OT_SIZE] {
    "undefined",
    "plot_data",
    "function",
    "value",
    "plot_data_ptr",
    "plot_data_itr",
    "function_itr",
    "plot_data_ptr_itr",
    "val",
    "val_ptr",
};

struct Plot_Data;
struct Command_Object
{
    Object_Type type = OT_undefined;
    Token tkn;
    bool new_object = false;
    union {
	Plot_Data* plot_data;
	Plot_Data** plot_data_ptr;
	Function* function;
	std::vector<Plot_Data*>* plot_data_itr;
	std::vector<Plot_Data**>* plot_data_ptr_itr;
	std::vector<Function*>* function_itr;
	double val;
	double* val_ptr;
    } obj;

    void delete_iterator() {
	switch(type) {
	case OT_plot_data_itr: delete obj.plot_data_itr; break;
	case OT_plot_data_ptr_itr: delete obj.plot_data_ptr_itr; break;
	case OT_function_itr: delete obj.function_itr; break;
	}
    }
    
    void delete_new_object();

    bool is_undefined() { return type == OT_undefined; }
};

enum Operator_Type
{
    OP_undefined,
    OP_add,
    OP_sub,
    OP_mul,
    OP_div,
    OP_update_add,
    OP_update_sub,
    OP_update_mul,
    OP_update_div,
    OP_assign,
    OP_smooth,
    OP_interp,
    OP_fit,
    OP_show,
    OP_hide,
    OP_extrema,
    OP_delete,
    OP_export,
    OP_run,
    OP_save,
    OP_zero,
    OP_help,
    OP_new,
    OP_SIZE,
};

inline const char *operator_type_name_table[OP_SIZE] {
    "undefined",
    "add",
    "sub",
    "mul",
    "div",
    "update_add",
    "update_sub",
    "update_mul",
    "update_div",
    "assign",
    "smooth",
    "interp",
    "fit",
    "show",
    "hide",
    "extrema",
    "delete",
    "export",
    "run",
    "save",
    "zero",
    "help",
    "new",
};

inline int op_arg_cnt_table[OP_SIZE] {
    0,
    2,
    2,
    2,
    2,
    1,
    1,
    1,
    1,
    1,
    2,
    2,
    2,
    1,
    1,
    1,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
};

struct Command_Operator
{
    Operator_Type type = OP_undefined;
    Token tkn;
    bool is_undefined() { return type == OP_undefined; }
};

// command structure:
// object (to be changed or created) = operator (+,-,fit) object_A (unary) object_B (binary)
bool handle_command(Lexer &lexer, int sub_level = 0, bool add_command = true);
void handle_command_file(std::string file);
void re_run_all_commands();
