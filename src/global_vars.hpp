#pragma once

#include "raylib.h"
#include "utils.hpp"
#include <cstdint>

inline Font g_app_font_18;
inline Font g_app_font_20;
inline Font g_app_font_22;

inline int g_keyboard_lock = 0;

inline Logger logger;

struct All_Commands
{
    void add(const std::string& cmd)
    {
	if (command_idx + 1 < int64_t(all_commands.size())) {
	    all_commands.erase(all_commands.begin() + command_idx + 1, all_commands.end());
	}
	all_commands.push_back(cmd);
	command_idx = all_commands.size() - 1;
    }
    
    void pop()
    {
	if (!all_commands.empty()) {
	    if (command_idx + 1 == int64_t(all_commands.size()))
		--command_idx;
	}
	all_commands.pop_back();
    }
    
    bool decr_command_idx()
    {
	if (command_idx > -1) {
	    --command_idx;
	    return true;
	}
	return false;
    }
    
    bool incr_command_idx()
    {
	if(command_idx + 1 < int64_t(all_commands.size())) {
	    ++command_idx;
	    return true;
	}
	return false;
    }
    
    void clear()
    {
	all_commands.clear();
	command_idx = -1;
    }
    
    const std::vector<std::string>& get_commands() const { return all_commands; }
    bool has_commands() const { return !all_commands.empty(); }
    int64_t get_index() const { return command_idx; }

private:

    int64_t command_idx = -1;
    std::vector<std::string> all_commands;
    
} inline g_all_commands;

inline size_t g_all_commands_index = 0;

constexpr std::string DEFAULT_EXPORT_FILE_NAME = "export";
