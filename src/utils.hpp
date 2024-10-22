#pragma once

#include "raylib.h"
#include "faster_plot.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>
#include <string>
#include <utility>
#include <fstream>

constexpr double UTILS_PI = 3.1415926535897932384626433832795;
constexpr double UTILS_EULER = 2.7182818284590452353602874713527;
#define sqr(x) (x)*(x)

namespace utils {
    constexpr uint64_t default_str_hash_value = 5381;
};

std::pair<char *, size_t> parse_file_cstr(const char *file_name);

struct Plot_Data;
std::vector<Plot_Data *> parse_numeric_csv_file(const std::string &file_name);

struct Log_Msg
{
    std::string msg;
    Color color;
};

constexpr int LOG_OUTPUT_ERROR_MSG_SIZE = 1024;

#  define UTILS_BLACK           "\033[30m"
#  define UTILS_RED             "\033[31m"
#  define UTILS_GREEN           "\033[32m"
#  define UTILS_YELLOW          "\033[33m"
#  define UTILS_BLUE            "\033[34m"
#  define UTILS_MAGENTA         "\033[35m"
#  define UTILS_CYAN            "\033[36m"
#  define UTILS_WHITE           "\033[37m"
#  define UTILS_BRIGHT_BLACK    "\033[90m"
#  define UTILS_BRIGHT_RED      "\033[91m"
#  define UTILS_BRIGHT_GREEN    "\033[92m"
#  define UTILS_BRIGHT_YELLOW   "\033[93m"
#  define UTILS_BRIGHT_BLUE     "\033[94m"
#  define UTILS_BRIGHT_MAGENTA  "\033[95m"
#  define UTILS_BRIGHT_CYAN     "\033[96m"
#  define UTILS_BRIGHT_WHITE    "\033[97m"
#  define UTILS_BLACK_BG           "\033[107m"
#  define UTILS_RED_BG             "\033[107m"
#  define UTILS_GREEN_BG           "\033[107m"
#  define UTILS_YELLOW_BG          "\033[107m"
#  define UTILS_BLUE_BG            "\033[107m"
#  define UTILS_MAGENTA_BG         "\033[107m"
#  define UTILS_CYAN_BG            "\033[107m"
#  define UTILS_WHITE_BG           "\033[107m"
#  define UTILS_BRIGHT_BLACK_BG    "\033[107m"
#  define UTILS_BRIGHT_RED_BG      "\033[107m"
#  define UTILS_BRIGHT_GREEN_BG    "\033[107m"
#  define UTILS_BRIGHT_YELLOW_BG   "\033[107m"
#  define UTILS_BRIGHT_BLUE_BG     "\033[107m"
#  define UTILS_BRIGHT_MAGENTA_BG  "\033[107m"
#  define UTILS_BRIGHT_CYAN_BG     "\033[107m"
#  define UTILS_BRIGHT_WHITE_BG    "\033[107m"
#  define UTILS_END_COLOR "\033[0m"

#define UTILS_ERROR_COLOR UTILS_BRIGHT_RED

struct Logger {

    // void add_msg(Log_Msg log_msg) { log_msgs.push_back(log_msg); }
    
    template <typename... Args>
    void log_error(const char *msg, Args... args)
    {
        if (log_level >= FPlot::LOGLVL_ERROR) {
            char buffer[LOG_OUTPUT_ERROR_MSG_SIZE];
            int true_size = snprintf(buffer, LOG_OUTPUT_ERROR_MSG_SIZE, msg, args...);
            if(true_size > LOG_OUTPUT_ERROR_MSG_SIZE) {
                log_error("Last error message did not fit in the buffer of size %d.", LOG_OUTPUT_ERROR_MSG_SIZE);
            }
            else {
                printf(UTILS_ERROR_COLOR "ERROR:" UTILS_END_COLOR " %s\n", buffer);
            }
        }
	++error_cnt;
    }

    template <typename... Args>
    void log_info(const char *msg, Args... args)
    {
        if (log_level >= FPlot::LOGLVL_INFO) {
            char buffer[LOG_OUTPUT_ERROR_MSG_SIZE];
            int true_size = snprintf(buffer, LOG_OUTPUT_ERROR_MSG_SIZE, msg, args...);
            if(true_size > LOG_OUTPUT_ERROR_MSG_SIZE) {
                log_error("Last error message did not fit in the buffer of size %d.", LOG_OUTPUT_ERROR_MSG_SIZE);
            }
            else {
                printf("%s", buffer);
            }
        }
    }

    void log_help_message();
    void set_log_level(FPlot::Log_Level log_level) { this->log_level = log_level; };

    size_t error_cnt = 0;
    FPlot::Log_Level log_level = FPlot::LOGLVL_INFO;
};

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

inline uint64_t hash_c_str(const char *str, uint64_t hash = utils::default_str_hash_value)
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


inline bool file_exists(const std::string& file_name)
{
    std::ifstream file(file_name);
    return file.good();
}

std::string get_file_extension(std::string file_name);

inline uint64_t get_uuid()
{
    static uint64_t id = 0;
    ++id;
    return id;
}


template<char check_key>
bool is_key_char_pressed()
{
    int key;
    key = GetCharPressed();
    while (key) {
	if (key == check_key)
	    return true;
	key = GetCharPressed();
    }
    return false;
}
