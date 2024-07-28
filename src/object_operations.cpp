#include "object_operations.hpp"

#include <algorithm>

#include "flusssensor_tool.hpp"
#include "function_fitting.hpp"
#include "utils.hpp"

bool interp_plot_data(Data_Manager &data_manager, Plot_Data *plot_data, int n_itr)
{
    if (plot_data->y.empty() || !plot_data->x)
	return false;

    std::vector<double> new_y(plot_data->y.size() * 2);
    std::vector<double> new_x(plot_data->x->y.size() * 2);
    
    double y_a = plot_data->y[0], y_b;
    double x_a = plot_data->x->y[0], x_b;

    new_x[0] = plot_data->x->y[0];
    new_y[0] = plot_data->y[0];
	
    for(size_t ix = 1; ix < plot_data->size(); ++ix)
    {
	y_b = plot_data->y[ix];
	x_b = plot_data->x->y[ix];

	new_x[ix * 2 - 1] = x_a + (x_b - x_a) * 0.5;
	new_x[ix * 2] = x_b;

	new_y[ix * 2 - 1] = y_a + (y_b - y_a) * 0.5;
	new_y[ix * 2] = y_b;
	
	y_a = y_b;
	x_a = x_b;
    }

    new_x[new_x.size() - 1] = plot_data->x->y[plot_data->x->y.size() - 1];
    new_y[new_y.size() - 1] = plot_data->y[plot_data->y.size() - 1];

    data_manager.plot_data.push_back(new Plot_Data);
    data_manager.plot_data.back()->y = new_x;
    plot_data->x = data_manager.plot_data.back();
    plot_data->y = new_y;

    if (n_itr >= 2)
	return interp_plot_data(data_manager, plot_data, n_itr - 1);
    return true;
}

bool smooth_plot_data(Plot_Data *plot_data, int window_size)
{
    if (plot_data->y.empty() || !plot_data->x)
	return false;

    for(size_t ix = 0; ix < plot_data->size(); ++ix)
    {
	double sum = 0;
	for (int iw = - window_size; iw <= window_size; ++iw) {
	    sum += plot_data->y[std::clamp(int(ix) + iw, 0, int(plot_data->size() - 1))];
	}
	plot_data->y[ix] = sum / double(2 * window_size + 1);
    }
    return true;
}

void fit_sinusoid_plot_data(Plot_Data *plot_data, Function *function)
{
    function->type = FT_sinusoid;
    function->func.sinusoid.fit_to_data(plot_data);
}

bool get_extrema_plot_data(Plot_Data *object_plot_data, Plot_Data *plot_data, int window_size)
{
    if (plot_data->y.empty() || !plot_data->x)
	return false;

    std::vector<double> smooth_plot_data_y;
    for(size_t ix = 0; ix < plot_data->size(); ++ix)
    {
	double sum = 0;
	for (int iw = - window_size; iw <= window_size; ++iw) {
	    sum += plot_data->y[std::clamp(int(ix) + iw, 0, int(plot_data->size() - 1))];
	}
	smooth_plot_data_y.push_back(sum / double(2 * window_size + 1));
    }

    object_plot_data->y.clear();

    double prev_val = smooth_plot_data_y[0];
    double val = smooth_plot_data_y[1];
    bool going_up = val > prev_val;
	
    for(size_t ix = 2; ix < smooth_plot_data_y.size(); ++ix)
    {
	val = smooth_plot_data_y[ix];
	prev_val = smooth_plot_data_y[ix - 1];
	if ((going_up && val < prev_val) || (!going_up && val > prev_val)) {
	    object_plot_data->y.push_back(plot_data->y[ix]);
	    object_plot_data->x->y.push_back(plot_data->x->y[ix]);
	}
	going_up = val > prev_val;
    }
    return true;

    // double prev_val = plot_data->y[0];
    // bool going_up;
    
    // double val = 0;
    // for (int iw = - window_size; iw <= window_size; ++iw) {
    // 	val += plot_data->y[std::clamp(iw, 0, int(plot_data->size() - 1))];
    // }
    // val /= double(2 * window_size + 1);
    // going_up = val > prev_val;
    // prev_val = val;
	
    // for(size_t ix = 1; ix < plot_data->size(); ++ix)
    // {
    // 	val = 0;
    // 	for (int iw = - window_size; iw <= window_size; ++iw) {
    // 	    val += plot_data->y[std::clamp(int(ix) + iw, 0, int(plot_data->size() - 1))];
    // 	}
    // 	val /= double(2 * window_size + 1);
	
    // 	if ((going_up && val < prev_val) || (!going_up && val > prev_val)) {
    // 	    object_plot_data->y.push_back(plot_data->y[ix]);
    // 	    object_plot_data->x->y.push_back(plot_data->x->y[ix]);
    // 	}

    // 	going_up = val > prev_val;
    // 	prev_val = val;
    // }
    // return true;
}