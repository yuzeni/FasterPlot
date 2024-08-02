#pragma once

#include <string_view>

#include "utils.hpp"

// y = a + b * cos(omega * x) + c sin(omega * x)
struct Sinusoidal_Function
{
    double a = 0, b = 0, c = 0, d = 0;

    bool is_defined() const { return a || b || c || d; }
    double operator()(double x) const;
    std::string get_string_value() const;
    std::string get_string_no_value() const;
    double get_parameter(std::string_view name) const;
    void get_fit_init_values(Plot_Data* data);
    double* get_parameter_ref(int idx);
    double get_fit_parameter_change_rate(int idx);
    
private:

    void sinusoid_fit_approximation(Plot_Data* data);
};

// y = a * x + b
struct Linear_Function
{
    double a = 1, b = 2;

    bool is_defined() const { return a || b; }
    double operator()(double x) const;
    std::string get_string_value() const;
    std::string get_string_no_value() const;
    double get_parameter(std::string_view name) const;
    void get_fit_init_values(Plot_Data* data);
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
void function_fit_iterative_naive(Plot_Data* plot_data, Function &function, int iterations);
