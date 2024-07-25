#pragma once

#include <vector>

#include "utils.hpp"

// y = a + b * cos(omega * x) + c sin(omega * x);
struct Sinusoidal_Function
{
    double a, b, c, omega;
    
    double operator()(double x);
    double fit_to_data(std::vector<Vec2<double>>& data);
};
