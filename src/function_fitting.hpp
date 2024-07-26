#pragma once

#include "utils.hpp"

// y = a + b * cos(omega * x) + c sin(omega * x)
struct Sinusoidal_Function
{
    double a = 1, b = 2, c = 3, omega = 4;

    bool is_defined() { return a || b || c || omega; }
    double operator()(double x) const;
    void fit_to_data(Plot_Data* data);
};

// y = a * x + b
struct Linear_Function
{
    double a = 1, b = 2;

    bool is_defined() { return a || b; }
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

struct Function
{
    Function() {}
    
    Function_Type type = FT_undefined;
    
    double operator()(double x) const
    {
	switch(type) {
	case FT_linear:   return func.linear(x);
	case FT_sinusoid: return func.sinusoid(x);
	}
	return 0;
    }
    
    union {
	Undefined_Function undefined = Undefined_Function{};
	Linear_Function linear;
	Sinusoidal_Function sinusoid;
    } func;
};
