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

#include "dosbox.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <list>
#include <thread>
#include <vector>

#include <SDL.h>
#include <SDL_thread.h>

#include "sdl_mapper.h"
#include "control.h"
#include "joystick.h"
#include "keyboard.h"
#include "mapper.h"
#include "math_utils.h"
#include "mouse.h"
#include "pic.h"
#include "rgb888.h"
#include "setup.h"
#include "string_utils.h"
#include "timer.h"
#include "video.h"
#include "default_bindings.h"

#include <iostream>
#include <sstream>
#include <fstream>

#include "event.h"

extern SDL_Window* GFX_GetWindow();
extern void GFX_UpdateDisplayDimensions(int width, int height);

//  Status Colors
//  ~~~~~~~~~~~~~
//  NFPA 79 standard for illuminated status indicators:
//  (https://www.nfpa.org/assets/files/AboutTheCodes/79/79-A2002-rop.pdf
//  pp.1588-1593)
//
constexpr Rgb888 marginal_color(255, 103, 0); // Amber for marginal conditions
constexpr Rgb888 on_color(0, 1, 0);           // Green for on/ready/in-use
constexpr Rgb888 off_color(0, 0, 0);          // Black for off/stopped/not-in-use

constexpr Rgb888 color_black(0, 0, 0);
constexpr Rgb888 color_grey(127, 127, 127);
constexpr Rgb888 color_white(255, 255, 255);
constexpr Rgb888 color_red(255, 0, 0);
constexpr Rgb888 color_green(0, 255, 0);

enum BB_Types {
	BB_Next,
	BB_Add,
	BB_Del,
	BB_Save,
	BB_Exit
};

enum BC_Types {
	BC_Mod1,
	BC_Mod2,
    BC_Mod3,
	BC_Hold
};

extern uint8_t int10_font_14[256 * 14];

void Mapper::SetJoystickLed([[maybe_unused]] SDL_Joystick *joystick,
						[[maybe_unused]] const Rgb888 &color) const
{
	// Basic joystick LED support was added in SDL 2.0.14
#if SDL_VERSION_ATLEAST(2, 0, 14)
	if (!joystick)
		return;
	if (!SDL_JoystickHasLED(joystick))
		return;

	// apply the color
	SDL_JoystickSetLED(joystick, color.red, color.green, color.blue);
#endif
}

void Mapper::CreateStringBind(std::string line)
{
	trim(line);
	std::string event_name = strip_word(line);
	auto event_it = std::find(events.begin(), events.end(),
	  [&](const auto& evt) { return nocase_cmp(evt.GetName(), event_name); }
	if (event_it == events.end()) {
		LOG_WARNING("MAPPER: Can't find key binding for '%s' event", eventname);
	} else {
		std::shared_ptr<Bind> bind;
		std::string bindline;
		do {
			bindline = strip_word(line);
			// Try to find a bindgroup that will process this line
			for (auto const &bind_group: bindgroups) {
				auto const bind = bind_group.CreateConfigBind(bindline);
				if (bind) {
					// Make them point to each other
					event->AddBind(bind);
					bind->AddEvent(event);
					bind->SetFlags(bindline);
					break;
				}
			}
		} while (bindline != "")
	}
}

void Mapper::ClearAllBinds() {
	// wait for the auto-typer to complete because it might be accessing events
	typist.Wait();
	for (const auto& event : events) {
		event->ClearBinds();
	}
}

void Mapper::CreateKeyBinding(std::string str, int value) {
    std::ostringstream oss;
}

void Mapper::CreateDefaultBinds() {
	ClearAllBinds();
    std::ostringstream oss;
	Bitu i{0};
	while (DefaultKeys[i].eventend) {
		oss << "key_" << DefaultKeys[i].eventend << " \"key " << DefaultKeys[i].key << "\"";
		CreateStringBind(oss.str());
		i++;
	}

	// Add the standard modifiers
	auto add_mod = [&](int const modnum, int const code) {
		std::ostringstream oss;
		oss << "mod_" << modnum << " \"key " << SDL_SCANCODE_RCTRL << "\"";
		CreateStringBind(oss.str());
	};
	add_mod(1, SDL_SCANCODE_RCTRL);
	add_mod(1, SDL_SCANCODE_LCTRL);
	add_mod(2, SDL_SCANCODE_RALT);
	add_mod(2, SDL_SCANCODE_LALT);
	add_mod(3, SDL_SCANCODE_RGUI);
	add_mod(3, SDL_SCANCODE_LGUI);

	// Make default binds for all handlers
	for (auto const &handler_event : handlergroup) {
		std::string const str{handler_event->MakeDefaultBind()};
		CreateStringBind(str);
	}

	auto bind_button = [&](int const joystick, int const button) {
		std::ostringstream oss;
		oss << "jbutton_" << joystick << "_" << button << " \"stick_" << joystick << " button " << button << "\"";
		CreateStringBind(oss.str());
	};

	/* joystick1, buttons 1-6 */
	for (int button = 0; button < 6; ++button) {
		bind_button(0, i);
	}
	/* joystick2, buttons 1-2 */
	bind_button(1, 0);
	bind_button(1, 1);

	auto bind_axis = [&](int const joystick, int const axis) {
		std::ostringstream oss;
		oss << "jaxis_" << joystick << "_" << axis << "- \"stick_" << joystick << " axis " << axis << "0\" ";
		CreateStringBind(oss.str());
		oss.clear();
		oss << "jaxis_" << joystick << "_" << axis << "+ \"stick_" << joystick << " axis " << axis << "1\" ";
		CreateStringBind(oss.str());
	};
	/* joystick1, axes 1-4 */
	for (int i=0; i<4; i++) {
		bind_axis(0, i);
	}
	bind_axis(1, 0);
	bind_axis(1, 1);

	auto bind_hat = [&](int const joystick, int const hat_in, int const hat_out) {
		std::ostringstream oss;
		oss << "jhat_" << joystick << "_0_" << hat_in << " \"stick_" << joystick << " hat 0 " << hat_out << "\"";
		CreateStringBind(oss.str());
	};
	/* joystick1, hat */
	bind_hat(0, 0, 1);
	bind_hat(0, 1, 2);
	bind_hat(0, 2, 4);
	bind_hat(0, 3, 8);

	LOG_MSG("MAPPER: Loaded default key bindings");
}

void Mapper::AddHandler(MAPPER_Handler *handler, SDL_Scancode const key,
                       uint32_t const mods, std::string const &event_name,
                       std::string const &button_name)
{
	const bool already_exists{
		std::find(handlergroup.begin(), handlergroup.end(),
			[&](const auto& handler_event) {
				return (handler_event->button_name == button_name);
			})};

	if (!already_exists) {
		std::ostringstream oss;
		oss << "hand_" << event_name;
		handlergroup.push_back(
			std::make_unique<CHandlerEvent>(oss.str(), handler, key, mods, button_name));
	}
}

void Mapper::SaveBinds() const {
	std::ofstream savefile{filename};

	if (!savefile.is_open()) {
		LOG_MSG("MAPPER: Can't open %s for saving the key bindings", filename);
		return;
	}
	for (const auto& event : events) {
		savefile << event->GetName() << " ";
		for (auto const &bind: event->bindlist) {
			savefile << "\"";
			savefile << bind->GetConfigName();
			savefile << bind->GetBindName();
			savefile << "\"";
		}
		savefile << "\n";
	}
	change_action_text("Mapper file saved.", color_white);
	LOG_MSG("MAPPER: Wrote key bindings to %s", filename);
}

bool Mapper::LoadBindsFromFile(const std::string_view mapperfile_path,
                                 const std::string_view mapperfile_name)
{
	// If the filename is empty the user wants defaults
	if (mapperfile_name == "")
		return false;

	auto try_loading = [](const std_fs::path &mapper_path) -> bool {
		constexpr auto optional = ResourceImportance::Optional;
		auto lines = GetResourceLines(mapper_path, optional);
		if (lines.empty())
			return false;

		ClearAllBinds();
		for (auto &line : lines)
			CreateStringBind(line.data());

		LOG_MSG("MAPPER: Loaded %d key bindings from '%s'",
		        static_cast<int>(lines.size()),
		        mapper_path.string().c_str());

		filename = mapper_path.string();
		return true;
	};

	const auto mapperfiles = std_fs::path("mapperfiles");

	const auto was_loaded = try_loading(mapperfile_path) ||
	                        try_loading(mapperfiles / mapperfile_name);

	// Only report load failures for customized mapperfiles because by
	// default, the mapperfile is not provided
	if (!was_loaded && mapperfile_name != MAPPERFILE)
		LOG_WARNING("MAPPER: Failed loading mapperfile '%s' directly or from resources",
		            mapperfile_name.data());

	return was_loaded;
}

void Mapper::CheckEvent(SDL_Event *event)
{
	for (auto &group : bindgroups)
		if (group->CheckEvent(event))
			return;
}

//  Initializes SDL's joystick subsystem an setups up initial joystick settings.

// If the user wants auto-configuration, then this sets joytype based on queried
// results. If no joysticks are valid then joytype is set to JOY_NONE_FOUND.
// This also resets mapper.sticks.num_groups to 0 and mapper.sticks.num to the
// number of found SDL joysticks.

// 7-21-2023: No longer resetting mapper.sticks.num_groups due to
// https://github.com/dosbox-staging/dosbox-staging/issues/2687
void Mapper::QueryJoysticks()
{
	// Reset our joystick status
	sticks.num = 0;

	JOYSTICK_ParseConfiguredType();

	// The user doesn't want to use joysticks at all (not even for mapping)
	if (joytype == JOY_DISABLED) {
		LOG_INFO("MAPPER: Joystick subsystem disabled");
		return;
	}

	if (SDL_WasInit(SDL_INIT_JOYSTICK) != SDL_INIT_JOYSTICK)
		SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	const bool wants_auto_config{joytype & (JOY_AUTO | JOY_ONLY_FOR_MAPPING)};

	// Record how many joysticks are present and set our desired minimum axis
	const auto num_joysticks = SDL_NumJoysticks();
	if (num_joysticks < 0) {
		LOG_WARNING("MAPPER: SDL_NumJoysticks() failed: %s", SDL_GetError());
		LOG_WARNING("MAPPER: Skipping further joystick checks");
		if (wants_auto_config) {
			joytype = JOY_NONE_FOUND;
		}
		return;
	}

	// We at least have a value number of joysticks
	assert(num_joysticks >= 0);
	sticks.num = static_cast<unsigned int>(num_joysticks);
	if (num_joysticks == 0) {
		LOG_MSG("MAPPER: No joysticks found");
		if (wants_auto_config) {
			joytype = JOY_NONE_FOUND;
		}
		return;
	}

	if (!wants_auto_config)
		return;

	// Everything below here involves auto-configuring
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	const int req_min_axis = std::min(num_joysticks, 2);

	// Check which, if any, of the first two joysticks are useable
	bool useable[2] = {false, false};
	for (int i = 0; i < req_min_axis; ++i) {
		SDL_Joystick *stick = SDL_JoystickOpen(i);
		set_joystick_led(stick, marginal_color);

		useable[i] = (SDL_JoystickNumAxes(stick) >= req_min_axis) ||
		             (SDL_JoystickNumButtons(stick) > 0);

		set_joystick_led(stick, off_color);
		SDL_JoystickClose(stick);
	}

	// Set the type of joystick based on which are useable
	const bool first_usable = useable[0];
	const bool second_usable = useable[1];
	if (first_usable && second_usable) {
		joytype = JOY_2AXIS;
		LOG_MSG("MAPPER: Found two or more joysticks");
	} else if (first_usable) {
		joytype = JOY_4AXIS;
		LOG_MSG("MAPPER: Found one joystick");
	} else if (second_usable) {
		joytype = JOY_4AXIS_2;
		LOG_MSG("MAPPER: Found second joystick is usable");
	} else {
		joytype = JOY_NONE_FOUND;
		LOG_MSG("MAPPER: Found no usable joysticks");
	}
}

void Mapper::CreateBindGroups() {
	bindgroups.clear();
	auto key_bind_group{std::make_unique<CKeyBindGroup>(SDL_NUM_SCANCODES)};
	keybindgroups.push_back(key_bind_group);

	assert(joytype != JOY_UNSET);

	if (joytype == JOY_DISABLED)
		return;

	if (joytype != JOY_NONE_FOUND) {
#if defined (REDUCE_JOYSTICK_POLLING)
		// direct access to the SDL joystick, thus removed from the event handling
		if (sticks.num)
			SDL_JoystickEventState(SDL_DISABLE);
#else
		// enable joystick event handling
		if (mapper.sticks.num)
			SDL_JoystickEventState(SDL_ENABLE);
		else
			return;
#endif
		// Free up our previously assigned joystick slot before assinging below
		if (sticks.stick[sticks.num_groups]) {
			delete sticks.stick[sticks.num_groups];
			sticks.stick[sticks.num_groups] = nullptr;
		}

		//TODO: go over more carefully
		uint8_t joyno = 0;
		switch (joytype) {
		case JOY_DISABLED:
		case JOY_NONE_FOUND: break;
		case JOY_4AXIS:
			sticks.stick[sticks.num_groups++] = new C4AxisBindGroup(joyno, joyno);
			stickbindgroups.push_back(new CStickBindGroup(joyno + 1U, joyno + 1U, true));
			break;
		case JOY_4AXIS_2:
			sticks.stick[mapper.sticks.num_groups++] = new C4AxisBindGroup(joyno + 1U, joyno);
			stickbindgroups.push_back(new CStickBindGroup(joyno, joyno + 1U, true));
			break;
		case JOY_FCS:
			mapper.sticks.stick[mapper.sticks.num_groups++] = new CFCSBindGroup(joyno, joyno);
			stickbindgroups.push_back(new CStickBindGroup(joyno + 1U, joyno + 1U, true));
			break;
		case JOY_CH:
			mapper.sticks.stick[mapper.sticks.num_groups++] = new CCHBindGroup(joyno, joyno);
			stickbindgroups.push_back(new CStickBindGroup(joyno + 1U, joyno + 1U, true));
			break;
		case JOY_AUTO:
		case JOY_ONLY_FOR_MAPPING:
		case JOY_2AXIS:
		default:
			mapper.sticks.stick[mapper.sticks.num_groups++] = new CStickBindGroup(joyno, joyno);
			if ((joyno + 1U) < mapper.sticks.num) {
				delete mapper.sticks.stick[mapper.sticks.num_groups];
				mapper.sticks.stick[mapper.sticks.num_groups++] = new CStickBindGroup(joyno + 1U, joyno + 1U);
			} else {
				stickbindgroups.push_back(new CStickBindGroup(joyno + 1U, joyno + 1U, true));
			}
			break;
		}
	}
}

bool Mapper::IsUsingJoysticks() const
{
	return (sticks.num > 0);
}

#ifdef REDUCE_JOYSTICK_POLLING

void Mapper::UpdateJoysticks() {
	for (Bitu i=0; i<sticks.num_groups; i++) {
		assert(sticks.stick[i]);
		sticks.stick[i]->UpdateJoystick();
	}
}

#endif

void Mapper::LosingFocus()
{
	for (const auto& event : events) {
		if (event.get() != caps_lock_event && event.get() != num_lock_event) {
			event->DeActivateAll();
		}
	}
}

void Mapper::RunEvent(uint32_t /*val*/)
{
	KEYBOARD_ClrBuffer();           // Clear buffer
	GFX_LosingFocus();		//Release any keys pressed (buffer gets filled again).
	MAPPER_DisplayUI();
}

void Mapper::Run(bool const pressed) {
	if (pressed)
		return;
	PIC_AddEvent(MAPPER_RunEvent, 0);	//In case mapper deletes the key object that ran it
}

SDL_Surface* SDL_SetVideoMode_Wrap(int width, int height, int bpp, uint32_t flags);

void Mapper::Destroy(Section *sec) {
	(void) sec; // unused but present for API compliance

	// Stop any ongoing typing as soon as possible (because it access events)
	typist.Stop();

	// Release all the accumulated allocations by the mapper
	events.clear();

	all_binds.clear();

	buttons.clear();

	keybindgroups.clear();

	stickbindgroups.clear();

	// Free any allocated sticks
	for (int i = 0; i < max_sticks; ++i) {
		delete mapper.sticks.stick[i];
		mapper.sticks.stick[i] = nullptr;
	}

	// Empty the remaining lists now that their pointers are defunct
	bindgroups.clear();
	handlergroup.clear();
	holdlist.clear();

	// Decrement our reference pointer to the Joystick subsystem
	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

void Mapper::BindKeys(Section *sec)
{
	// Release any keys pressed, or else they'll get stuck
	GFX_LosingFocus();

	// Get the mapper file set by the user
	const auto section = static_cast<const Section_prop *>(sec);
	const auto mapperfile_value = section->Get_string("mapperfile");
	const auto property = section->Get_path("mapperfile");
	assert(property);
	filename = property->realpath.string();

	QueryJoysticks();

	// Create the graphical layout for all registered key-binds
	if (buttons.empty())
		CreateLayout();

	if (bindgroups.empty())
		CreateBindGroups();

	// Create binds from file or fallback to internals
	if (!load_binds_from_file(filename, mapperfile_value))
		CreateDefaultBinds();

	for (const auto& button : buttons) {
		button->BindColor();
	}

	if (SDL_GetModState() & KMOD_CAPS)
		MAPPER_TriggerEvent(caps_lock_event, false);

	if (SDL_GetModState()&KMOD_NUM)
		MAPPER_TriggerEvent(num_lock_event, false);

	GFX_RegenerateWindow(sec);
}

std::vector<std::string> Mapper::GetEventNames(const std::string &prefix) {
	std::vector<std::string> key_names;
	key_names.reserve(events.size());
	for (auto & e : events) {
		const std::string name = e->GetName();
		const std::size_t found = name.find(prefix);
		if (found != std::string::npos) {
			const std::string key_name = name.substr(found + prefix.length());
			key_names.push_back(key_name);
		}
	}
	return key_names;
}

void Mapper::AutoType(std::vector<std::string> &sequence,
                     const uint32_t wait_ms,
                     const uint32_t pace_ms) {
	typist.Start(&events, sequence, wait_ms, pace_ms);
}

void Mapper::AutoTypeStopImmediately()
{
	typist.StopImmediately();
}

void Mapper::StartUp(Section* sec)
{
	assert(sec);
	Section_prop* section = static_cast<Section_prop*>(sec);

	// Runs after this function ends and for subsequent `config -set "sdl
	// mapperfile=file.map"` commands
	constexpr auto changeable_at_runtime = true;
	section->AddInitFunction(&MAPPER_BindKeys, changeable_at_runtime);

	// Runs one-time on shutdown
	section->AddDestroyFunction(&MAPPER_Destroy);

	// Set up the ctrl-F1 shortcut for the mapper itself
	MAPPER_AddHandler(&MAPPER_Run, SDL_SCANCODE_F1, PRIMARY_MOD, "mapper", "Mapper");
}
