#pragma once

#include <string_view>

#include "utils.hpp"

// y = a + b * sin(c * x + d)
struct Sinusoidal_Function
{
    double a = 0, b = 1, c = 1, d = 0;

    double operator()(double x) const;
    std::string get_string_value() const;
    std::string get_string_no_value() const;
    void get_fit_init_values(Plot_Data* data);
    double* get_parameter_ref(std::string_view name);
    int get_parameter_idx(std::string_view name);
    double* get_parameter_ref(int idx);
    double get_fit_parameter_change_rate(int idx);
    
private:

    void sinusoid_fit_approximation(Plot_Data* data);
};

// y = a * x + b
struct Linear_Function
{
    double a = 1, b = 0;

    double operator()(double x) const;
    std::string get_string_value() const;
    std::string get_string_no_value() const;
    void get_fit_init_values(Plot_Data* data);
    double* get_parameter_ref(std::string_view name);
    int get_parameter_idx(std::string_view name);
    double* get_parameter_ref(int idx);
    double get_fit_parameter_change_rate(int idx);
};

struct Undefined_Function {};

enum Function_Type
{
    FT_undefined,
    FT_linear,
    FT_sinusoid,
    FT_SIZE,
};

struct Function;
void function_fit_iterative_naive(Plot_Data *data, Function &function, std::vector<double*>& param_list, std::vector<double>& param_change_rate, int iterations);
