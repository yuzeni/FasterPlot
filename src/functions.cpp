#include "functions.hpp"

#include "data_manager.hpp"
#include "app_loop.hpp"
#include "function_parsing.hpp"
#include "raylib.h"
#include "utils.hpp"
#include <cmath>
#include <string>

static void function_fit_iterative_naive(Plot_Data *data, Function &function, std::vector<double*>& param_list, int iterations);

void Function::get_all_param_ref(std::vector<double*>& param_list)
{
    int param_idx = 0;
    double* param_ref = get_parameter_ref(param_idx);
    while(param_ref)
    {
	param_list.push_back(param_ref);
	
	param_idx++;
	param_ref = get_parameter_ref(param_idx);
    }
}

void Function::update_content_tree_element(size_t index)
{
    content_element.name = "function " + std::to_string(index) + (!info.header.empty() ? " '" + info.header + "'" : "");
    content_element.name += " (x) = " + get_string_no_value();
    content_element.name_color = info.color;
    content_element.content.clear();
    content_element.content.push_back({"f(x) = " + get_string_value()});
    
    if (info.visible)
	content_element.content.push_back({"visible"});
    else
	content_element.content.push_back({"hidden"});
    if (fit_from_data) {
	content_element.content.push_back({"fit of "});
	content_element.content.push_back({fit_from_data->content_element.name, false, fit_from_data->info.color});
    }
}

/* Sinusoidal Function **************************/

double Sinusoidal_Function::operator()(double x) const { return a + b * std::sin(c * x + d); }
std::string Sinusoidal_Function::get_string_value() const { return std::to_string(a) + " + " + std::to_string(b) + " * sin(" + std::to_string(c) + " * x + " + std::to_string(d) + ")"; }
std::string Sinusoidal_Function::get_string_no_value() const { return "a + b * sin(c * x + d)"; }

double* Sinusoidal_Function::get_parameter_ref(std::string_view name)
{
    switch(hash_string_view(name)) {
    case cte_hash_c_str("a"): return &a;
    case cte_hash_c_str("b"): return &b;
    case cte_hash_c_str("c"): return &c;
    case cte_hash_c_str("d"): return &d;
    }
    return nullptr;
}

int Sinusoidal_Function::get_parameter_idx(std::string_view name)
{
    switch(hash_string_view(name)) {
    case cte_hash_c_str("a"): return 0;
    case cte_hash_c_str("b"): return 1;
    case cte_hash_c_str("c"): return 2;
    case cte_hash_c_str("d"): return 3;
    }
    return -1;
}

void Sinusoidal_Function::fit_to_data(Plot_Data *plot_data, int iterations, std::vector<double *> &param_list, bool warm_start)
{
    if (warm_start) {
	sinusoid_fit_approximation(plot_data);
    }
    function_fit_iterative_naive(plot_data, *this, param_list, iterations);
}

double* Sinusoidal_Function::get_parameter_ref(int idx)
{
    switch(idx) {
    case 0: return &a;
    case 1: return &b;
    case 2: return &c;
    case 3: return &d;
    default: return nullptr;
    }    
}

// Sinusoidal Fit Algorithm (for first approximation):
// https://stackoverflow.com/questions/77350332/sine-curve-to-fit-data-cloud-using-c

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

    double omega = std::sqrt(-c_vec.v[0]);

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
    b = std::sqrt(abc.v[1] * abc.v[1] + abc.v[2] * abc.v[2]);
    c = omega;
    d = std::atan(abc.v[1] / abc.v[2]);
}

/* Linear Function **************************/

double Linear_Function::operator()(double x) const { return a * x + b; }
std::string Linear_Function::get_string_value() const { return std::to_string(a) + " * x + " + std::to_string(b); }
std::string Linear_Function::get_string_no_value() const { return "a * x + b"; }

double* Linear_Function::get_parameter_ref(std::string_view name)
{
    switch(hash_string_view(name)) {
    case cte_hash_c_str("a"): return &a;
    case cte_hash_c_str("b"): return &b;
    }
    return nullptr;
}

int Linear_Function::get_parameter_idx(std::string_view name)
{
    switch(hash_string_view(name)) {
    case cte_hash_c_str("a"): return 0;
    case cte_hash_c_str("b"): return 1;
    }
    return -1;
}

void Linear_Function::fit_to_data(Plot_Data *plot_data, int iterations, std::vector<double *> &param_list, bool warm_start)
{
    if (warm_start) {
	linear_fit_approximation(plot_data);
    }
    function_fit_iterative_naive(plot_data, *this, param_list, iterations);
}

double* Linear_Function::get_parameter_ref(int idx)
{
    switch(idx) {
    case 0: return &a;
    case 1: return &b;
    default: return nullptr;
    }
}

void Linear_Function::linear_fit_approximation(Plot_Data *data)
{
    double Ax, Ay, Bx, By;

    if (data->size() < 2) {
	logger.log_error("Can't fit to data, because it contains less than 2 values");
	return;
    }
    
    if (data->x) {
	Ax = data->x->y[0];
	Bx = data->x->y[data->size() - 1];
    }
    else {
	Ax = 0;
	Bx = data->size() - 1;
    }

    Ay = data->y[0];
    By = data->y[data->size() - 1];

    a = (Ay - By) / (Ax - Bx);
    b = By - a * Bx;
}


/* Generic Function **************************/

Generic_Function::Generic_Function(Lexer& lexer)
{
    op_tree.base_node = parse_expression(lexer, *this);
}

double* Generic_Function::get_parameter_ref(std::string_view name)
{
    for (auto& param : params) {
	if (hash_string_view(name) == hash_c_str(param.name.c_str())) {
	    return &param.val;
	}
    }
    return nullptr;
}

int Generic_Function::get_parameter_idx(std::string_view name)
{
    for (size_t i = 0; i < params.size(); ++i) {
	if (hash_string_view(name) == hash_c_str(params[i].name.c_str())) {
	    return i;
	}
    }
    return -1;
}

void Generic_Function::fit_to_data(Plot_Data *plot_data, int iterations, std::vector<double *> &param_list, [[maybe_unused]] bool warm_start)
{
    if (warm_start) {
	generic_fit_approximation(plot_data);
    }
    function_fit_iterative_naive(plot_data, *this, param_list, iterations);
}

double* Generic_Function::get_parameter_ref(int idx)
{
    if (idx < int(params.size())) {
	return &params[idx].val;
    }
    else {
	return nullptr;
    }
}

void Generic_Function::generic_fit_approximation(Plot_Data *data)
{
    
}

/* Function Type Independents **************************/

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
    const double delta_x = (0.001 / (data->size()));
    double squared_error_ya = squared_error(data, function);
    double orig_param = *param;
    *param += delta_x;
    double squared_error_yb = squared_error(data, function);
    *param = orig_param;
    return (squared_error_yb - squared_error_ya) / delta_x;
}

static void function_fit_iterative_naive(Plot_Data *data, Function &function, std::vector<double*>& param_list, int iterations)
{
    logger.log_info("Error before iterative optimization: %f\n", squared_error(data, function));
    
    std::vector<double> derivatives(param_list.size(), 0);
    std::vector<double> step_sizes(param_list.size(), 0.00001);

    double time_begin = GetTime();
    double time_begin_begin = time_begin;

    for (int i = 0; i < iterations; ++i)
    {
	if (GetTime() - time_begin > 1.0 / (double(TARGET_FPS) * 0.33)) {
	    app_loop();
	    time_begin = GetTime();
	}
	
	for (size_t i = 0; i < param_list.size(); ++i) {
	    derivatives[i] = squared_error_derivative(data, param_list[i], function);
	}

	// searching for the best 
	for (size_t i = 0; i < param_list.size(); ++i)
	{
	    double orig_param = *param_list[i];
	    double best_error = squared_error(data, function);
	    bool converged = true;
	    
	    double best_step_size = step_sizes[i];
	    for (double step_size = best_step_size * 100; step_size >= best_step_size * 0.001 && step_size > 10e-15; step_size *= 0.1)
	    {
		*param_list[i] -= derivatives[i] * step_size;
		
		double this_error = squared_error(data, function);
		if (best_error > this_error) {
		    converged = false;
		    best_error = this_error;
		    best_step_size = step_size;
		}
		
		*param_list[i] = orig_param;
	    }

	    if (!converged) {
		*param_list[i] -= derivatives[i] * best_step_size;
		step_sizes[i] = best_step_size;
	    }
	    else {
		step_sizes[i] = best_step_size * 0.001;
	    }
	}

	bool all_converged = true;
	for (size_t i = 0; i < param_list.size(); ++i) {
	    if (step_sizes[i] > 10e-14) {
		all_converged = false;
	    }
	}
	if (all_converged) {
	    logger.log_info("Converged at iteration: %d\n", i);
	    break;
	}
	    
    }
    logger.log_info("Error after iterative optimization: %f\n", squared_error(data, function));
    logger.log_info("Elapsed time: %f s\n", GetTime() - time_begin_begin);
}
