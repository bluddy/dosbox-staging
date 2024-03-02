/*
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *
 *  Copyright (C) 2020-2024  The DOSBox Staging Team
 *  Copyright (C) 2002-2021  The DOSBox Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef DOSBOX_SDL_MAPPER_H
#define DOSBOX_SDL_MAPPER_H

#include <vector>
#include <array>
#include <SDL.h>

class CEvent;
class CBind;

static constexpr int max_sticks{8};
static constexpr int max_active{16};
// Use 36 for Android (KEYCODE_BUTTON_1..16 are mapped to SDL buttons 20..35)
static constexpr int max_button{36};
static constexpr int max_button_cap{16};
static constexpr int max_axis{10};
static constexpr int max_hat{2};


class Mapper {
    // Singleton pattern
    static Mapper& get() {
        static Mapper instance;
        return instance;
    }

private:
    Mapper() {} // Private constructor for singleton
    void SetJoystickLed([[maybe_unused]] SDL_Joystick *joystick,
                         [[maybe_unused]] const Rgb888 &color);
    void CreateStringBind(std::string const &line);
    void ClearAllBinds();
    void CreateDefaultBinds();
    void AddHandler(MAPPER_Handler *handler, SDL_Scancode const key,
                    uint32_t const mods, std::string const &event_name, std::string const &button_name);
    void SaveBinds() const;
    bool LoadBindsFromFile(const std::string_view mapperfile_path,
                           const std::string_view mapperfile_name);

    void CheckEvent(SDL_Event *event);
    void QueryJoysticks();
    void CreateBindGroups();
    bool IsUsingJoysticks() const;
    void HandleLosingFocus();
    void RunEvent(uint32_t);
    void Run(bool const pressed);
    void Destroy(Section *sec);
    void BindKeys(Section *sec);
    std::vector<std::string> GetEventNames(const std::string &prefix) const;
    void AutoType(std::vector<std::string> &sequence,
                     const uint32_t wait_ms,
                     const uint32_t pace_ms);
    void AutoTypeStopImmediately();
    void Startup(Section* sec);

#ifdef REDUCE_JOYSTICK_POLLING
    void UpdateJoysticks();
#endif

    // Groups of actual joystick bindings
    std::list<std::shared_ptr<CStickBindGroup>> stickbindgroups;

	SDL_Window *window = nullptr;
	SDL_Renderer* renderer  = nullptr;
	SDL_Texture* font_atlas = nullptr;
	bool exit = false;

    // Active event and bind being created right now
    // Weak pointers to prevent loops
	std::weak_ptr<CEvent> active_event;
	std::weak_prt<CBind> active_bind = nullptr;

	CBindList_it abindit = {}; // Location of active bind in list
	bool redraw = false;
	bool addbind = false;  // Whether we're in the add bind state
	Bitu mods = 0;  // Currently active modifiers

    // Detected info about joysticks
	struct {
		CStickBindGroup *stick[MAXSTICKS] = {nullptr};
		unsigned int num = 0;
		unsigned int num_groups = 0;
	} sticks = {};

	Typer typist;
	std::string filename;

    bool autofire{false};

    std::array<VirtJoystick, 2> virtual_joysticks;
    std::vector<std::unique_ptr<CEvent>> events;
    std::vector<std::unique_ptr<CBindGroup>> bindgroups;
    std::list<std::unique_ptr<CKeyBindGroup>> keybindgroups;
    std::vector<std::unique_ptr<CHandlerEvent>> handlergroup;
    CBindList holdlist;
    // Contains all binds. Don't let individual events manage this
    std::list<std::unique_ptr<CBind>> all_binds; 
    CKeyEvent * caps_lock_event=nullptr;
    CKeyEvent * num_lock_event=nullptr;
};


#endif