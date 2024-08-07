#pragma once

#include <vector>

#include "function_parsing.hpp"
#include "gui_elements.hpp"

class Function
{
public:
    Plot_Info info;
    Content_Tree_Element content_element;
    Plot_Data* fit_from_data = nullptr;
    size_t index = 0;

    virtual ~Function() {};
    
    void update_content_tree_element(size_t index);
    void get_all_param_ref(std::vector<double*>& param_list);
    
    virtual double operator()(double x) const = 0;
    virtual std::string get_string_value() const = 0;
    virtual std::string get_string_no_value() const = 0;
    virtual double* get_parameter_ref(std::string_view name) = 0;
    virtual int get_parameter_idx(std::string_view name) = 0;
    virtual void fit_to_data(Plot_Data* plot_data, int iterations, std::vector<double*>& param_list, bool warm_start = true) = 0;
    virtual double* get_parameter_ref(int idx) = 0;
};


class Sinusoidal_Function : public Function
{
public:

    Sinusoidal_Function(){}
    
    // y = a + b * sin(c * x + d)
    double a = 0, b = 1, c = 1, d = 0;

    double operator()(double x) const override;
    std::string get_string_value() const override;
    std::string get_string_no_value() const override;
    double* get_parameter_ref(std::string_view name) override;
    int get_parameter_idx(std::string_view name) override;
    void fit_to_data(Plot_Data* plot_data, int iterations, std::vector<double*>& param_list, bool warm_start = true) override;
    double* get_parameter_ref(int idx) override;
    
private:

    void sinusoid_fit_approximation(Plot_Data* data);
};

class Linear_Function : public Function
{
public:

    Linear_Function(){}
    
    // y = a * x + b
    double a = 1, b = 0;

    double operator()(double x) const override;
    std::string get_string_value() const override;
    std::string get_string_no_value() const override;
    double *get_parameter_ref(std::string_view name) override;
    int get_parameter_idx(std::string_view name) override;
    void fit_to_data(Plot_Data *plot_data, int iterations, std::vector<double *> &param_list, bool warm_start = true) override;
    double *get_parameter_ref(int idx) override;

  private:

    void linear_fit_approximation(Plot_Data *data);
};

struct Parameter
{
    double val;
    std::string name;
    double fit_change_rate;
};

class Generic_Function : public Function
{
public:

    Generic_Function(){}
    Generic_Function(Lexer& lexer);
    
    std::vector<Parameter> params;
    Function_Op_Tree op_tree;

    double operator()(double x) const override { return op_tree.evaluate(*this, x); }
    std::string get_string_value() const override { return op_tree.get_string_value(*this); }
    std::string get_string_no_value() const override { return op_tree.get_string_no_value(*this); }
    double* get_parameter_ref(std::string_view name) override;
    int get_parameter_idx(std::string_view name) override;
    void fit_to_data(Plot_Data* plot_data, int iterations, std::vector<double*>& param_list, bool warm_start = true) override;
    double* get_parameter_ref(int idx) override;

private:

    void generic_fit_approximation(Plot_Data *data);
};
