#pragma once

#include "raylib.h"

#include <cmath>
#include <cstdio>
#include <vector>
#include <string>
#include <utility>

std::pair<char *, size_t> parse_file_cstr(const char *file_name);

struct Plot_Data;
std::vector<Plot_Data*> parse_numeric_csv_file(std::string file_name);

#define ERROR_MSG_SIZE 1024

#define UTILS_ERROR_COLOR "\033[91m"
#define UTILS_END_COLOR   "\033[0m"

template <typename... Args>
void log_error(const char *msg, Args... args)
{
    char buffer[ERROR_MSG_SIZE];
    int true_size = snprintf(buffer, ERROR_MSG_SIZE, msg, args...);
    if(true_size > ERROR_MSG_SIZE) {
	log_error("Last error message did not fit in the buffer of size %d.", ERROR_MSG_SIZE);
    }
    else {
	printf(UTILS_ERROR_COLOR "ERROR:" UTILS_END_COLOR " %s", buffer);
    }
}

#undef ERROR_MSG_SIZE

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
