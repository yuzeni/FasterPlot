#pragma once

#include "raylib.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <string>
#include <utility>

namespace utils {
    constexpr uint64_t default_str_hash_value = 5381;
};

std::pair<char *, size_t> parse_file_cstr(const char *file_name);

struct Plot_Data;
std::vector<Plot_Data*> parse_numeric_csv_file(std::string file_name);

struct Log_Msg
{
    std::string msg;
    Color color;
};

constexpr int LOG_OUTPUT_ERROR_MSG_SIZE = 1024;

struct Logger {

    void add_msg(Log_Msg log_msg) { log_msgs.push_back(log_msg); }
    
    template <typename... Args>
    void log_error(const char *msg, Args... args)
    {
	char buffer[LOG_OUTPUT_ERROR_MSG_SIZE];
	int true_size = snprintf(buffer, LOG_OUTPUT_ERROR_MSG_SIZE, msg, args...);
	if(true_size > LOG_OUTPUT_ERROR_MSG_SIZE) {
	    log_error("Last error message did not fit in the buffer of size %d.", LOG_OUTPUT_ERROR_MSG_SIZE);
	}
	else {
	    log_msgs.push_back({"ERROR: ", RED});
	    log_msgs.push_back({buffer, BLACK});
	}
    }

    std::vector<Log_Msg> log_msgs;
};

// #define ERROR_MSG_SIZE 1024

// #define UTILS_ERROR_COLOR "\033[91m"
// #define UTILS_END_COLOR   "\033[0m"

// template <typename... Args>
// void log_error(const char *msg, Args... args)
// {
//     char buffer[ERROR_MSG_SIZE];
//     int true_size = snprintf(buffer, ERROR_MSG_SIZE, msg, args...);
//     if(true_size > ERROR_MSG_SIZE) {
// 	log_error("Last error message did not fit in the buffer of size %d.", ERROR_MSG_SIZE);
//     }
//     else {
// 	printf(UTILS_ERROR_COLOR "ERROR:" UTILS_END_COLOR " %s\n", buffer);
//     }
// }

// #undef ERROR_MSG_SIZE

template <typename T>
struct Vec2
{
    T x;
    T y;
    
    operator Vector2() const { return Vector2{float(x), float(y)}; }
    Vec2<T> operator +(T val) const { return {x + val, y + val}; }
    Vec2<T> operator -(T val) const { return {x - val, y - val}; }
    Vec2<T> operator *(T val) const { return {x * val, y * val}; }
    Vec2<T> operator /(T val) const { return {x / val, y / val}; }
    Vec2<T> operator +(Vec2<T> other) const { return {x + other.x, y + other.y}; }
    Vec2<T> operator -(Vec2<T> other) const { return {x - other.x, y - other.y}; }
    Vec2<T> operator *(Vec2<T> other) const { return {x * other.x, y * other.y}; }
    Vec2<T> operator /(Vec2<T> other) const { return {x / other.x, y / other.y}; }
    T length() const { return std::sqrt(x*x + y*y); }
    void normalize() { T l = length(); x /= l, y /= l; };
};

consteval uint64_t cte_hash_c_str(const char *str, uint64_t hash = utils::default_str_hash_value)
{
    int c;
    while((c = *(str++)))
	hash = (hash * 33) ^ c;
    return hash;
}

uint64_t hash_string_view(std::string_view s_v, uint64_t hash = utils::default_str_hash_value);

template <typename T>
T add_flag(T var, T flag)
{
    uint64_t var_int = uint64_t(var);
    var_int |= uint64_t(flag);
    return T(var_int);
}

template <typename T>
T remove_flag(T var, T flag)
{
    uint64_t var_int = uint64_t(var);
    if (var_int & uint64_t(flag))
	    var_int -= uint64_t(flag);
    return T(var_int);
}

template <typename T>
bool check_flag(T var, T flag)
{
    if (uint64_t(var) & uint64_t(flag))
	return true;
    return false;
}

Vector2 draw_text_boxed(Font font, const char *text, Vector2 position, float fontSize, float spacing, Color tint);
