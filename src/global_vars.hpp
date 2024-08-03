#pragma once

#include "raylib.h"
#include "utils.hpp"
#include <cstdint>

constexpr int TARGET_FPS = 60;
constexpr int SCREEN_WIDTH = 800;
constexpr int SCREEN_HEIGHT = 600;

inline Font g_app_font_18;
inline Font g_app_font_20;
inline Font g_app_font_22;

inline int g_keyboard_lock = 0;

inline Logger logger;

constexpr std::string DEFAULT_EXPORT_FILE_NAME = "export";
constexpr std::string DEFAULT_SAVE_FILE_NAME = "save";

enum Command_Flags {
    CF_none         = 0,
    CF_hidden       = 1,
    CF_only_on_save = 1 << 1,
};

struct Command
{
    std::string cmd;
    Command_Flags flags;
};

struct All_Commands
{
    void add(const std::string& cmd, Command_Flags cmd_flags = CF_none)
    {
	if (command_idx + 1 < int64_t(all_commands.size())) {
	    all_commands.erase(all_commands.begin() + command_idx + 1, all_commands.end());
	}
	all_commands.push_back({cmd, cmd_flags});
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

    void add_cmd_flag(Command_Flags cmd_flag)
    {
	all_commands[command_idx].flags = add_flag(all_commands[command_idx].flags, cmd_flag);
    }
    
    const std::vector<Command>& get_commands() const { return all_commands; }
    bool has_commands() const { return !all_commands.empty(); }
    int64_t get_index() const { return command_idx; }

private:

    int64_t command_idx = -1;
    std::vector<Command> all_commands;
    
} inline g_all_commands;

inline size_t g_all_commands_index = 0;
