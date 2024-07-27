#pragma once

#include "utils.hpp"

// y = a + b * cos(omega * x) + c sin(omega * x)
struct Sinusoidal_Function
{
    double a = 0, b = 0, c = 0, omega = 0;

    bool is_defined() const { return a || b || c || omega; }
    double operator()(double x) const;
    void fit_to_data(Plot_Data* data);
};

// y = a * x + b
struct Linear_Function
{
    double a = 1, b = 2;

    bool is_defined() const { return a || b; }
    double operator()(double x) const;
};

struct Undefined_Function {};

enum Function_Type
{
    FT_undefined,
    FT_linear,
    FT_sinusoid,
    FT_SIZE,
};
