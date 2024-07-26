#pragma once

#include "lexer.hpp"

enum Object_Type
{
    OT_undefined,
    OT_plot_data,
    OT_function,
    OT_value,
};

struct Plot_Data;
struct Function;
struct Command_Object
{
    Object_Type type = OT_undefined;
    Token tkn;
    union {
	Plot_Data* plot_data;
	Function* function;
    } obj;

    bool is_undefined() { return type == OT_undefined; }
};

enum Operator_Type {
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
    OP_SIZE
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
};

inline int op_arg_cnt_table[OP_SIZE] {
    2,
    2,
    2,
    2,
    1,
    1,
    1,
    1,
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
struct Data_Manager;
void handle_command(Data_Manager &data_manager);
