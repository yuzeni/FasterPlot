#include "function_fitting.hpp"

#include "data_manager.hpp"
#include "utils.hpp"

#include <cmath>
#include <limits>
#include <string>
#include <vector>

double Sinusoidal_Function::operator()(double x) const
{
    return a + b * std::cos(omega * x) + c * std::sin(omega * x);
}

std::string Sinusoidal_Function::get_string_value() const
{
    return std::to_string(a) + " + " + std::to_string(b) + " * cos(" + std::to_string(omega) + " * x) + " + std::to_string(c) + " * sin(" + std::to_string(omega) + " * x)";
}

std::string Sinusoidal_Function::get_string_no_value() const
{
    return "a + b * cos(omega * x) + c * sin(omega * x)";
}

double Sinusoidal_Function::get_parameter(std::string_view name) const
{
    switch(hash_string_view(name)) {
    case cte_hash_c_str("a"): return a;
    case cte_hash_c_str("b"): return b;
    case cte_hash_c_str("c"): return c;
    case cte_hash_c_str("omega"): return omega;
    }
    return std::numeric_limits<double>::quiet_NaN();
}

double Linear_Function::operator()(double x) const
{
    return a * x + b;
}

std::string Linear_Function::get_string_value() const
{
    return std::to_string(a) + " * x + " + std::to_string(b);
}

std::string Linear_Function::get_string_no_value() const
{
    return "a * x + b";
}

double Linear_Function::get_parameter(std::string_view name) const
{
    switch(hash_string_view(name)) {
    case cte_hash_c_str("a"): return a;
    case cte_hash_c_str("b"): return b;
    }
    return std::numeric_limits<double>::quiet_NaN();
}

static void get_SS_n__with_x(double* SS_n, int n, Plot_Data* data)
{
    double SS_i = 0, S_i = 0, S_i_m1 = 0;
    SS_n[0] = SS_i;
    for(int i = 1; i < n; ++i) {
	S_i = S_i_m1 + 0.5 * (data->y[i] + data->y[i - 1]) * (data->x->y[i] - data->x->y[i-1]);
	SS_i = SS_i + 0.5 * (S_i + S_i_m1) * (data->x->y[i] - data->x->y[i-1]);
	S_i_m1 = S_i;
	SS_n[i] = SS_i;
    }
}

static void get_SS_n__without_x(double* SS_n, int n, Plot_Data* data)
{
    double SS_i = 0, S_i = 0, S_i_m1 = 0;
    SS_n[0] = SS_i;
    for(int i = 1; i < n; ++i) {
	S_i = S_i_m1 + 0.5 * (data->y[i] + data->y[i - 1]) * (double(i) - double(i-1));
	SS_i = SS_i + 0.5 * (S_i + S_i_m1) * (double(i) - double(i-1));
	S_i_m1 = S_i;
	SS_n[i] = SS_i;
    }
}


struct Vector_4
{
    double v[4];

    Vector_4(double v[4]) {
	for(int i = 0; i < 4; ++i)
	    this->v[i] = v[i];
    }
};

struct Vector_3
{
    double v[3];

    Vector_3(double v[3]) {
	for(int i = 0; i < 3; ++i)
	    this->v[i] = v[i];
    }
};

struct Matrix_4x4
{
    double m[4][4];
    
    Matrix_4x4(double m[4][4]) {
	for(int i = 0; i < 4; ++i) {
	    for(int j = 0; j < 4; ++j) {
		this->m[i][j] = m[i][j];
	    }
	}
    }
    
    Matrix_4x4 get_inverse()
    {
	double _1_div_det = 1.0 / get_determinant();
	Matrix_4x4 adj = get_adjugate();
	for(int i = 0; i < 4; ++i) {
	    for(int j = 0; j < 4; ++j) {
		adj.m[i][j] *= _1_div_det;
	    }
	}
	return adj;
    }
    
    double get_determinant()
    {
	double det = m[0][0] * (m[1][1] * (m[2][2] * m[3][3] - m[2][3] * m[3][2])
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
	return det;
    }

    Matrix_4x4 get_adjugate()
    {
	double adj[4][4]{
	    {m[1][1]*m[2][2]*m[3][3]-m[1][1]*m[2][3]*m[3][2]-m[1][2]*m[2][1]*m[3][3]+m[1][2]*m[2][3]*m[3][1]+m[1][3]*m[2][1]*m[3][2]-m[1][3]*m[2][2]*m[3][1],
	     -m[0][1]*m[2][2]*m[3][3]+m[0][1]*m[2][3]*m[3][2]+m[0][2]*m[2][1]*m[3][3]-m[0][2]*m[2][3]*m[3][1]-m[0][3]*m[2][1]*m[3][2]+m[0][3]*m[2][2]*m[3][1],
	     m[0][1]*m[1][2]*m[3][3]-m[0][1]*m[1][3]*m[3][2]-m[0][2]*m[1][1]*m[3][3]+m[0][2]*m[1][3]*m[3][1]+m[0][3]*m[1][1]*m[3][2]-m[0][3]*m[1][2]*m[3][1],
	     -m[0][1]*m[1][2]*m[2][3]+m[0][1]*m[1][3]*m[2][2]+m[0][2]*m[1][1]*m[2][3]-m[0][2]*m[1][3]*m[2][1]-m[0][3]*m[1][1]*m[2][2]+m[0][3]*m[1][2]*m[2][1]},
	
	    {-m[1][0]*m[2][2]*m[3][3]+m[1][0]*m[2][3]*m[3][2]+m[1][2]*m[2][0]*m[3][3]-m[1][2]*m[2][3]*m[3][0]-m[1][3]*m[2][0]*m[3][2]+m[1][3]*m[2][2]*m[3][0],
	     m[0][0]*m[2][2]*m[3][3]-m[0][0]*m[2][3]*m[3][2]-m[0][2]*m[2][0]*m[3][3]+m[0][2]*m[2][3]*m[3][0]+m[0][3]*m[2][0]*m[3][2]-m[0][3]*m[2][2]*m[3][0],
	     -m[0][0]*m[1][2]*m[3][3]+m[0][0]*m[1][3]*m[3][2]+m[0][2]*m[1][0]*m[3][3]-m[0][2]*m[1][3]*m[3][0]-m[0][3]*m[1][0]*m[3][2]+m[0][3]*m[1][2]*m[3][0],
	     m[0][0]*m[1][2]*m[2][3]-m[0][0]*m[1][3]*m[2][2]-m[0][2]*m[1][0]*m[2][3]+m[0][2]*m[1][3]*m[2][0]+m[0][3]*m[1][0]*m[2][2]-m[0][3]*m[1][2]*m[2][0]},
	
	    {m[1][0]*m[2][1]*m[3][3]-m[1][0]*m[2][3]*m[3][1]-m[1][1]*m[2][0]*m[3][3]+m[1][1]*m[2][3]*m[3][0]+m[1][3]*m[2][0]*m[3][1]-m[1][3]*m[2][1]*m[3][0],
	     -m[0][0]*m[2][1]*m[3][3]+m[0][0]*m[2][3]*m[3][1]+m[0][1]*m[2][0]*m[3][3]-m[0][1]*m[2][3]*m[3][0]-m[0][3]*m[2][0]*m[3][1]+m[0][3]*m[2][1]*m[3][0],
	     m[0][0]*m[1][1]*m[3][3]-m[0][0]*m[1][3]*m[3][1]-m[0][1]*m[1][0]*m[3][3]+m[0][1]*m[1][3]*m[3][0]+m[0][3]*m[1][0]*m[3][1]-m[0][3]*m[1][1]*m[3][0],
	     -m[0][0]*m[1][1]*m[2][3]+m[0][0]*m[1][3]*m[2][1]+m[0][1]*m[1][0]*m[2][3]-m[0][1]*m[1][3]*m[2][0]-m[0][3]*m[1][0]*m[2][1]+m[0][3]*m[1][1]*m[2][0]},
	
	    {-m[1][0]*m[2][1]*m[3][2]+m[1][0]*m[2][2]*m[3][1]+m[1][1]*m[2][0]*m[3][2]-m[1][1]*m[2][2]*m[3][0]-m[1][2]*m[2][0]*m[3][1]+m[1][2]*m[2][1]*m[3][0],
	     m[0][0]*m[2][1]*m[3][2]-m[0][0]*m[2][2]*m[3][1]-m[0][1]*m[2][0]*m[3][2]+m[0][1]*m[2][2]*m[3][0]+m[0][2]*m[2][0]*m[3][1]-m[0][2]*m[2][1]*m[3][0],
	     -m[0][0]*m[1][1]*m[3][2]+m[0][0]*m[1][2]*m[3][1]+m[0][1]*m[1][0]*m[3][2]-m[0][1]*m[1][2]*m[3][0]-m[0][2]*m[1][0]*m[3][1]+m[0][2]*m[1][1]*m[3][0],
	     m[0][0]*m[1][1]*m[2][2]-m[0][0]*m[1][2]*m[2][1]-m[0][1]*m[1][0]*m[2][2]+m[0][1]*m[1][2]*m[2][0]+m[0][2]*m[1][0]*m[2][1]-m[0][2]*m[1][1]*m[2][0]}
	};

	return Matrix_4x4{adj};
    }

    Vector_4 mul_vector_4(Vector_4 vec) {

	double v[4] {0,0,0,0};
	Vector_4 result{v};
	
	for(int i = 0; i < 4; ++i) {
	    for(int j = 0; j < 4; ++j) {
		result.v[j] += vec.v[i] * m[j][i];
	    }
	}
	return result;
    }
};

struct Matrix_3x3
{
    double m[3][3];
    
    Matrix_3x3(double m[3][3]) {
	for(int i = 0; i < 3; ++i) {
	    for(int j = 0; j < 3; ++j) {
		this->m[i][j] = m[i][j];
	    }
	}
    }
    
    Matrix_3x3 get_inverse()
    {
	double _1_div_det = 1.0 / get_determinant();
	Matrix_3x3 adj = get_adjugate();
	for(int i = 0; i < 3; ++i) {
	    for(int j = 0; j < 3; ++j) {
		adj.m[i][j] *= _1_div_det;
	    }
	}
	return adj;
    }

    double get_determinant()
    {
	double det = m[0][0] * m[1][1] * m[2][2]
	    - m[0][0] * m[1][2] * m[2][1]
	    - m[0][1] * m[1][0] * m[2][2]
	    + m[0][1] * m[1][2] * m[2][0]
	    + m[0][2] * m[1][0] * m[2][1]
	    - m[0][2] * m[1][1] * m[2][0];
	return det;
    }

    Matrix_3x3 get_adjugate()
    {
	double adj[3][3]{
	    {m[1][1] * m[2][2] - m[1][2] * m[2][1], m[0][2] * m[2][1] - m[0][1] * m[2][2], m[0][1] * m[1][2] - m[0][2] * m[1][1]},
	    {m[1][2] * m[2][0] - m[1][0] * m[2][2], m[0][0] * m[2][2] - m[0][2] * m[2][0], m[0][2] * m[1][0] - m[0][0] * m[1][2]},
	    {m[1][0] * m[2][1] - m[1][1] * m[2][0], m[0][1] * m[2][0] - m[0][0] * m[2][1], m[0][0] * m[1][1] - m[0][1] * m[1][0]},
	};

	return Matrix_3x3{adj};
    }

    Vector_3 mul_vector_3(Vector_3 vec) {

	double v[3] {0,0,0};
	Vector_3 result{v};
	
	for(int i = 0; i < 3; ++i) {
	    for(int j = 0; j < 3; ++j) {
		result.v[j] += vec.v[i] * m[j][i];
	    }
	}
	return result;
    }
};

void Sinusoidal_Function::sinusoid_fit_approximation(Plot_Data* data)
{
    int n = data->size();
    
    double* SS_n = new double[n];
    if (data->x)
	get_SS_n__with_x(SS_n, n, data);
    else
	get_SS_n__without_x(SS_n, n, data);

    double ssn_sqr = 0;
    double ssn_xn_sqr = 0;
    double ssn_xn = 0;
    double ssn = 0;
    double xn_tess = 0;
    double xn_cubed = 0;
    double xn_sqr = 0;
    double xn = 0;
    double ssn_yn = 0;
    double xn_sqr_yn = 0;
    double xn_yn = 0;
    double yn = 0;
    
    for(int i = 0; i < n; ++i)
    {
	double xk  = data->x ? data->x->y[i] : double(i);
	double yk  = data->y[i];
	double SSk = SS_n[i];
	
	ssn_sqr    += SSk * SSk;
	ssn_xn_sqr += SSk * xk * xk;
	ssn_xn     += SSk * xk;
	ssn        += SSk;
	xn_tess    += xk * xk * xk * xk;
	xn_cubed   += xk * xk * xk;
	xn_sqr     += xk * xk;
	xn         += xk;
	ssn_yn     += SSk * yk;
	xn_sqr_yn  += xk * xk * yk;
	xn_yn      += xk * yk;
	yn         += yk;
    }

    delete[] SS_n;
    
    double m4[4][4] {
	{ssn_sqr,    ssn_xn_sqr, ssn_xn,   ssn},
	{ssn_xn_sqr, xn_tess,    xn_cubed, xn_sqr},
	{ssn_xn,     xn_cubed,   xn_sqr,   xn},
	{ssn,        xn_sqr,     xn,       double(n)},
    };
    Matrix_4x4 mat4{m4};

    double v[4] {
	ssn_yn,
	xn_sqr_yn,
	xn_yn,
	yn,
    };
    Vector_4 vec4{v};
    
    Vector_4 c_vec = mat4.get_inverse().mul_vector_4(vec4);

    omega = std::sqrt(-c_vec.v[0]);

    double sin_omega_xn = 0;
    double cos_omega_xn = 0;
    double sin_sqr_omega_xn = 0;
    double sin_omega_xn_cos_omega_xn = 0;
    double cos_sqr_omega_xn = 0;
    double yn_sin_omega_xn = 0;
    double yn_cos_omega_xn = 0;

    for(int i = 0; i < n; ++i)
    {
	double xk  = data->x ? data->x->y[i] : double(i);
	double yk  = data->y[i];
	double sin_omega_xk = std::sin(omega * xk);
	double cos_omega_xk = std::cos(omega * xk);
	
	sin_omega_xn              += sin_omega_xk;
	cos_omega_xn              += cos_omega_xk;
	sin_sqr_omega_xn          += sin_omega_xk * sin_omega_xk;
	sin_omega_xn_cos_omega_xn += sin_omega_xk * cos_omega_xk;
	cos_sqr_omega_xn          += cos_omega_xk * cos_omega_xk;
	yn_sin_omega_xn           += yk * sin_omega_xk;
	yn_cos_omega_xn           += yk * cos_omega_xk;
    }


    double m3[3][3] {
	{double(n),    sin_omega_xn,              cos_omega_xn},
	{sin_omega_xn, sin_sqr_omega_xn,          sin_omega_xn_cos_omega_xn},
	{cos_omega_xn, sin_omega_xn_cos_omega_xn, cos_sqr_omega_xn},
    };
    Matrix_3x3 mat3{m3};

    double v3[3] {
	yn,
	yn_sin_omega_xn,
	yn_cos_omega_xn,
    };
    Vector_3 vec3{v3};
    
    Vector_3 abc = mat3.get_inverse().mul_vector_3(vec3);

    a = abc.v[0];
    b = abc.v[1];
    c = abc.v[2];
}

void Sinusoidal_Function::get_fit_init_values(Plot_Data *data)
{
    sinusoid_fit_approximation(data);
}

void Linear_Function::get_fit_init_values(Plot_Data *data)
{
    return;
}

static double squared_error(Plot_Data* data, Function& function)
{
    double squared_error = 0;
    
    if (data->x) {
	for (size_t i = 0; i < data->size(); ++i) {
	    squared_error += (function(data->x->y[i]) - data->y[i]) * (function(data->x->y[i]) - data->y[i]);
	}
    }
    else {
	for (size_t i = 0; i < data->size(); ++i) {
	    squared_error += (function(i) - data->y[i]) * (function(i) - data->y[i]);
	}
    }
    return squared_error / double(data->size());
}

static double squared_error_derivative(Plot_Data* data, double *param, Function& function)
{
    const double delta_x = 0.00001;
    double squared_error_ya = squared_error(data, function);
    *param += delta_x;
    double squared_error_yb = squared_error(data, function);
    *param -= delta_x; // this might not exactly revert param, not sure.
    return (squared_error_yb - squared_error_ya) / delta_x;
}

void function_fit_iterative_naive(Plot_Data *data, Function &function, int iterations)
{
    logger.log_info("Error before: %f\n", squared_error(data, function));
    const double learning_rate = 0.001;
    // const int iterations = 10;
    
    const std::vector<double*>& param_list = function.get_param_list();
    std::vector<double> derivatives(param_list.size(), 0);

    for (int i = 0; i < iterations; ++i) {
	for (size_t i = 0; i < param_list.size(); ++i) {
	    derivatives[i] = squared_error_derivative(data, param_list[i], function);
	}

	// for (size_t i = 0; i < param_list.size(); ++i) {
	//     *param_list[i] += derivatives[i] * learning_rate; // '-' because we want to approach zero
	// }

	// for sinusoid only
	*param_list[0] -= derivatives[0] * 0.01;
	*param_list[1] -= derivatives[1] * 0.01;
	*param_list[2] -= derivatives[2] * 0.01;
	*param_list[3] -= derivatives[3] * 0.000001;
    }
    logger.log_info("Error after: %f\n", squared_error(data, function));
}




