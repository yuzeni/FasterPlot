#pragma once

#include "utils.hpp"
#include <string_view>

// y = a + b * cos(omega * x) + c sin(omega * x)
struct Sinusoidal_Function
{
    double a = 0, b = 0, c = 0, omega = 0;

    bool is_defined() const { return a || b || c || omega; }
    double operator()(double x) const;
    std::string get_string_value() const;
    std::string get_string_no_value() const;
    void fit_to_data(Plot_Data* data);
    double get_parameter(std::string_view name) const;
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
};

struct Undefined_Function {};

enum Function_Type
{
    FT_undefined,
    FT_linear,
    FT_sinusoid,
    FT_SIZE,
};
