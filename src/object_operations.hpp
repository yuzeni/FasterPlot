#pragma once

struct Plot_Data;
struct Function;
struct Data_Manager;

inline double add_op(double a, double b) { return a + b; }
inline double sub_op(double a, double b) { return a - b; }
inline double mul_op(double a, double b) { return a * b; }
inline double div_op(double a, double b) { return a / b; }

bool interp_plot_data(Data_Manager &data_manager, Plot_Data *plot_data, int n_itr);
bool smooth_plot_data(Plot_Data *plot_data, int window_size);

void fit_sinusoid_plot_data(Plot_Data *plot_data, Function *function);

bool get_extrema_plot_data(Plot_Data* object_plot_data, Plot_Data* plot_data);
