#include "function_fitting.hpp"

#include <cmath>

double Sinusoidal_Function::operator()(double x)
{
    return a + b * std::cos(omega * x) + c * std::sin(omega * x);
}

static void get_SS_n(double* SS_n, int n, std::vector<Vec2<double>>& data)
{
    double SS_i = 0, S_i = 0, S_i_m1 = 0;
    SS_n[0] = SS_i;
    for(int i = 1; i < n; ++i) {
	S_i = S_i_m1 + 0.5 * (data[i].y + data[i - 1].y) * (data[i].x - data[i-1].x);
	SS_i = SS_i + 0.5 * (S_i + S_i_m1) * (data[i].x - data[i-1].x);
	S_i_m1 = S_i;
	SS_n[i] = SS_i;
    }
}

struct Matrix_4x4
{
    double m[4][4];
    
    void invert()
    {
	
    }
    
private:
    double get_determinant(Matrix_4x4 mat)
    {
	double D = m[0][0] * (m[1][1] * (m[2][2] * m[3][3] - m[2][3] * m[3][2])
			      - m[1][2] * (m[2][1] * m[3][3] - m[2][3] * m[3][1])
			      + m[1][3] * (m[2][1] * m[3][2] - m[2][2] * m[3][1]))
		- m[0][1] * (m[1][0] * (m[2][2] * m[3][3] - m[2][3] * m[3][2])
			     - m[1][2] * (m[2][0] * m[3][3] - m[2][3] * m[3][0])
			     + m[1][3] * (m[2][0] * m[3][2] - m[2][2] * m[3][0]))
		+ m[0][2] * (m[1][0] * (m[2][1] * m[3][3] - m[2][3] * m[3][1])
			     - m[1][1] * (m[2][0] * m[3][3] - m[2][3] * m[3][0])
			     + m[1][3] * (m[2][0] * m[3][1] - m[2][1] * m[3][0]))
		- m[0][3] * (m[1][0] * (m[2][1] * m[3][2] - m[2][2] * m[3][1])
			     - m[1][1] * (m[2][0] * m[3][2] - m[2][2] * m[3][0])
			     + m[1][2] * (m[2][0] * m[3][1] - m[2][1] * m[3][0]));
	return D;
    }
    
    Matrix_4x4 get_adjugate(Matrix_4x4 mat)
	{
	    Matrix_4x4 adj = {
		{f * k * p - f * l * o - g * j * p + g * l * n + h * j * o - h * k * n,
		  - b * k * p + b * l * o + c * j * p - c * l * n - d * j * o + d * k * n,
		 b * g * p - b * h * o - c * f * p + c * h * n + d * f * o - d * g * n,
		  - b * g * l + b * h * k + c * f * l - c * h * j - d * f * k + d * g * j},
	
		{ - e * k * p + e * l * o + g * i * p - g * l * m - h * i * o + h * k * m,
		 m[0][0] * k * p - m[0][0] * l * o - c * i * p + c * l * m + d * i * o - d * k * m,
		  - m[0][0] * g * p + m[0][0] * h * o + c * e * p - c * h * m - d * e * o + d * g * m,
		 m[0][0] * g * l - m[0][0] * h * k - c * e * l + c * h * i + d * e * k - d * g * i},
	
		{e * j * p - e * l * n - f * i * p + f * l * m + h * i * n - h * j * m,
		  - m[0][0] * j * p + m[0][0] * l * n + b * i * p - b * l * m - d * i * n + d * j * m,
		 m[0][0] * f * p - m[0][0] * h * n - b * e * p + b * h * m + d * e * n - d * f * m,
		  - m[0][0] * f * l + m[0][0] * h * j + b * e * l - b * h * i - d * e * j + d * f * i},
	
		{ - e * j * o + e * k * n + f * i * o - f * k * m - g * i * n + g * j * m,
		 m[0][0] * j * o - m[0][0] * k * n - b * i * o + b * k * m + c * i * n - c * j * m,
		  - m[0][0] * f * o + m[0][0] * g * n + b * e * o - b * g * m - c * e * n + c * f * m,
		 m[0][0] * f * k - m[0][0] * g * j - b * e * k + b * g * i + c * e * j - c * f * i}
	    };
	};

double Sinusoidal_Function::fit_to_data(std::vector<Vec2<double>> &data)
{
    int n = data.size();
    double* SS_n = new double[n];
    get_SS_n(SS_n, n, data);
    
    
    delete[] SS_n;
}
