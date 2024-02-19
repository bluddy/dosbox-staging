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
	BB_Next,BB_Add,BB_Del,
	BB_Save,BB_Exit
};

enum BC_Types {
	BC_Mod1,BC_Mod2,BC_Mod3,
	BC_Hold
};

#define BMOD_Mod1 MMOD1
#define BMOD_Mod2 MMOD2
#define BMOD_Mod3 MMOD3

#define BFLG_Hold 0x0001
#define BFLG_Repeat 0x0004


#define MAXSTICKS 8
#define MAXACTIVE 16
// Use 36 for Android (KEYCODE_BUTTON_1..16 are mapped to SDL buttons 20..35)
#define MAXBUTTON 36
#define MAXBUTTON_CAP 16
#define MAXAXIS       10
#define MAXHAT        2

class CEvent;
class CHandlerEvent;
class CButton;
class CBind;
class CBindGroup;

static void SetActiveEvent(CEvent * event);
static void SetActiveBind(CBind * _bind);
extern uint8_t int10_font_14[256 * 14];

static std::vector<std::unique_ptr<CEvent>> events;
static std::vector<std::unique_ptr<CButton>> buttons;
static std::vector<CBindGroup *> bindgroups;
static std::vector<CHandlerEvent *> handlergroup;
static std::list<CBind *> all_binds;

static CBindList holdlist;


#define MAX_VJOY_BUTTONS 8
#define MAX_VJOY_HAT 16
#define MAX_VJOY_AXIS 8
static struct {
	int16_t axis_pos[MAX_VJOY_AXIS] = {0};
	bool hat_pressed[MAX_VJOY_HAT] = {false};
	bool button_pressed[MAX_VJOY_BUTTONS] = {false};
} virtual_joysticks[2];


bool autofire = false;

static void set_joystick_led([[maybe_unused]] SDL_Joystick *joystick,
                             [[maybe_unused]] const Rgb888 &color)
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

std::list<CStickBindGroup *> stickbindgroups;

void MAPPER_TriggerEvent(const CEvent *event, const bool deactivation_state) {
	assert(event);
	for (auto &bind : event->bindlist) {
		bind->ActivateBind(32767, true, false);
		bind->DeActivateBind(deactivation_state);
	}
}

static void DrawText(int32_t x, int32_t y, const char* text, const Rgb888& color)
{
	SDL_Rect character_rect = {0, 0, 8, 14};
	SDL_Rect dest_rect      = {x, y, 8, 14};
	SDL_SetTextureColorMod(mapper.font_atlas, color.red, color.green, color.blue);
	while (*text) {
		character_rect.y = *text * character_rect.h;
		SDL_RenderCopy(mapper.renderer,
		               mapper.font_atlas,
		               &character_rect,
		               &dest_rect);
		text++;
		dest_rect.x += character_rect.w;
	}
}

static struct {
	CCaptionButton *  event_title;
	CCaptionButton *  bind_title;
	CCaptionButton *  selected;
	CCaptionButton *  action;
	CBindButton * save;
	CBindButton *exit;
	CBindButton * add;
	CBindButton * del;
	CBindButton * next;
	CCheckButton * mod1,* mod2,* mod3,* hold;
} bind_but;

static void change_action_text(const char* text, const Rgb888& col)
{
	bind_but.action->Change(text,"");
	bind_but.action->SetColor(col);
}

static std::string humanize_key_name(const CBindList &binds, const std::string &fallback)
{
	auto trim_prefix = [](const std::string& bind_name) {
		if (starts_with(bind_name, "Left ")) {
			return bind_name.substr(sizeof("Left"));
		}
		if (starts_with(bind_name, "Right ")) {
			return bind_name.substr(sizeof("Right"));
		}
		return bind_name;
	};

	const auto binds_num = binds.size();

	// We have a single bind, just use it
	if (binds_num == 1)
		return binds.front()->GetBindName();

	// Avoid prefix, e.g. "Left Alt" and "Right Alt" -> "Alt"
	if (binds_num == 2) {
		const auto key_name_1 = trim_prefix(binds.front()->GetBindName());
		const auto key_name_2 = trim_prefix(binds.back()->GetBindName());
		if (key_name_1 == key_name_2) {
			if (fallback.empty())
				return key_name_1;
			else
				return fallback + ": " + key_name_1;
		}
	}

	return fallback;
}

static void update_active_bind_ui()
{
	if (mapper.abind == nullptr) {
		bind_but.bind_title->Enable(false);
		bind_but.del->Enable(false);
		bind_but.next->Enable(false);
		bind_but.mod1->Enable(false);
		bind_but.mod2->Enable(false);
		bind_but.mod3->Enable(false);
		bind_but.hold->Enable(false);
		return;
	}

	// Count number of bindings for active event and the number (pos)
	// of active bind
	size_t active_event_binds_num = 0;
	size_t active_bind_pos = 0;
	if (mapper.aevent) {
		const auto &bindlist = mapper.aevent->bindlist;
		active_event_binds_num = bindlist.size();
		for (const auto *bind : bindlist) {
			if (bind == mapper.abind)
				break;
			active_bind_pos += 1;
		}
	}

	std::string mod_1_desc = "";
	std::string mod_2_desc = "";
	std::string mod_3_desc = "";

	// Correlate mod event bindlists to button labels and prepare
	// human-readable mod key names.
	for (auto &event : events) {
		using namespace std::string_literals;

		assert(event);
		const auto bindlist = event->bindlist;

		if (event->GetName() == "mod_1"s) {
			bind_but.mod1->Enable(!bindlist.empty());
			bind_but.mod1->SetText(humanize_key_name(bindlist, "Mod1"));
			const std::string txt = humanize_key_name(bindlist, "");
			mod_1_desc = (txt.empty() ? "Mod1" : txt) + " + ";
			continue;
		}
		if (event->GetName() == "mod_2"s) {
			bind_but.mod2->Enable(!bindlist.empty());
			bind_but.mod2->SetText(humanize_key_name(bindlist, "Mod2"));
			const std::string txt = humanize_key_name(bindlist, "");
			mod_2_desc = (txt.empty() ? "Mod1" : txt) + " + ";
			continue;
		}
		if (event->GetName() == "mod_3"s) {
			bind_but.mod3->Enable(!bindlist.empty());
			bind_but.mod3->SetText(humanize_key_name(bindlist, "Mod3"));
			const std::string txt = humanize_key_name(bindlist, "");
			mod_3_desc = (txt.empty() ? "Mod1" : txt) + " + ";
			continue;
		}
	}

	// Format "Bind: " description
	const auto mods = mapper.abind->mods;
	bind_but.bind_title->Change("Bind %" PRIuPTR "/%" PRIuPTR ": %s%s%s%s",
	                            active_bind_pos + 1, active_event_binds_num,
	                            (mods & BMOD_Mod1 ? mod_1_desc.c_str() : ""),
	                            (mods & BMOD_Mod2 ? mod_2_desc.c_str() : ""),
	                            (mods & BMOD_Mod3 ? mod_3_desc.c_str() : ""),
	                            mapper.abind->GetBindName().c_str());

	bind_but.bind_title->SetColor(color_green);
	bind_but.bind_title->Enable(true);
	bind_but.del->Enable(true);
	bind_but.next->Enable(active_event_binds_num > 1);
	bind_but.hold->Enable(true);
}

static void SetActiveBind(CBind *new_active_bind)
{
	mapper.abind = new_active_bind;
	update_active_bind_ui();
}

static void SetActiveEvent(CEvent * event) {
	mapper.aevent=event;
	mapper.redraw=true;
	mapper.addbind=false;
	bind_but.event_title->Change("   Event: %s", event ? event->GetName() : "none");
	if (!event) {
		change_action_text("Select an event to change.", color_white);
		bind_but.add->Enable(false);
		SetActiveBind(nullptr);
	} else {
		change_action_text("Modify the bindings for this event or select a different event.",
		                   color_white);
		mapper.abindit=event->bindlist.begin();
		if (mapper.abindit!=event->bindlist.end()) {
			SetActiveBind(*(mapper.abindit));
		} else SetActiveBind(nullptr);
		bind_but.add->Enable(true);
	}
}

extern SDL_Window* GFX_GetWindow();
extern void GFX_UpdateDisplayDimensions(int width, int height);

static void DrawButtons() {
	SDL_SetRenderDrawColor(mapper.renderer,
	                       color_black.red,
	                       color_black.green,
	                       color_black.blue,
	                       SDL_ALPHA_OPAQUE);
	SDL_RenderClear(mapper.renderer);
	for (const auto& button : buttons) {
		button->Draw();
	}
	SDL_RenderPresent(mapper.renderer);
}

static CKeyEvent* AddKeyButtonEvent(int32_t x, int32_t y, int32_t dx,
                                    int32_t dy, const char* const title,
                                    const char* const entry, KBD_KEYS key)
{
	char buf[64];
	safe_strcpy(buf, "key_");
	safe_strcat(buf, entry);
	CKeyEvent * event=new CKeyEvent(buf,key);
	new CEventButton(x,y,dx,dy,title,event);
	return event;
}

static CMouseButtonEvent* AddMouseButtonEvent(const int32_t x, const int32_t y,
                                              const int32_t dx, const int32_t dy,
                                              const char* const title,
                                              const char* const entry,
                                              const MouseButtonId button_id)
{
	auto event = new CMouseButtonEvent(entry, button_id);
	new CEventButton(x, y, dx, dy, title, event);
	return event;
}

static CJAxisEvent* AddJAxisButton(int32_t x, int32_t y, int32_t dx, int32_t dy,
                                   const char* const title, Bitu stick, Bitu axis,
                                   bool positive, CJAxisEvent* opposite_axis)
{
	char buf[64];
	sprintf(buf, "jaxis_%d_%d%s",
	        static_cast<int>(stick),
	        static_cast<int>(axis),
	        positive ? "+" : "-");
	CJAxisEvent	* event=new CJAxisEvent(buf,stick,axis,positive,opposite_axis);
	new CEventButton(x,y,dx,dy,title,event);
	return event;
}
static CJAxisEvent * AddJAxisButton_hidden(Bitu stick,Bitu axis,bool positive,CJAxisEvent * opposite_axis) {
	char buf[64];
	sprintf(buf, "jaxis_%d_%d%s",
	        static_cast<int>(stick),
	        static_cast<int>(axis),
	        positive ? "+" : "-");
	return new CJAxisEvent(buf,stick,axis,positive,opposite_axis);
}

static void AddJButtonButton(int32_t x, int32_t y, int32_t dx, int32_t dy,
                             const char* const title, Bitu stick, Bitu button)
{
	char buf[64];
	sprintf(buf, "jbutton_%d_%d",
	        static_cast<int>(stick),
	        static_cast<int>(button));
	CJButtonEvent * event=new CJButtonEvent(buf,stick,button);
	new CEventButton(x,y,dx,dy,title,event);
}
static void AddJButtonButton_hidden(Bitu stick,Bitu button) {
	char buf[64];
	sprintf(buf, "jbutton_%d_%d",
	        static_cast<int>(stick),
	        static_cast<int>(button));
	new CJButtonEvent(buf,stick,button);
}

static void AddJHatButton(int32_t x, int32_t y, int32_t dx, int32_t dy,
                          const char* const title, Bitu _stick, Bitu _hat, Bitu _dir)
{
	char buf[64];
	sprintf(buf, "jhat_%d_%d_%d",
	        static_cast<int>(_stick),
	        static_cast<int>(_hat),
	        static_cast<int>(_dir));
	CJHatEvent * event=new CJHatEvent(buf,_stick,_hat,_dir);
	new CEventButton(x,y,dx,dy,title,event);
}

static void AddModButton(int32_t x, int32_t y, int32_t dx, int32_t dy,
                         const char* const title, int mod)
{
	char buf[64];
	sprintf(buf, "mod_%d", mod);
	CModEvent *event = new CModEvent(buf, mod);
	new CEventButton(x, y, dx, dy, title, event);
}

struct KeyBlock {
	const char * title;
	const char * entry;
	KBD_KEYS key;
};
static KeyBlock combo_f[12]={
	{"F1","f1",KBD_f1},		{"F2","f2",KBD_f2},		{"F3","f3",KBD_f3},
	{"F4","f4",KBD_f4},		{"F5","f5",KBD_f5},		{"F6","f6",KBD_f6},
	{"F7","f7",KBD_f7},		{"F8","f8",KBD_f8},		{"F9","f9",KBD_f9},
	{"F10","f10",KBD_f10},	{"F11","f11",KBD_f11},	{"F12","f12",KBD_f12},
};

static KeyBlock combo_1[14]={
	{"`~","grave",KBD_grave},	{"1!","1",KBD_1},	{"2@","2",KBD_2},
	{"3#","3",KBD_3},			{"4$","4",KBD_4},	{"5%","5",KBD_5},
	{"6^","6",KBD_6},			{"7&","7",KBD_7},	{"8*","8",KBD_8},
	{"9(","9",KBD_9},			{"0)","0",KBD_0},	{"-_","minus",KBD_minus},
	{"=+","equals",KBD_equals},	{"\x1B","bspace",KBD_backspace},
};

static KeyBlock combo_2[12] = {
        {"Q", "q", KBD_q},
        {"W", "w", KBD_w},
        {"E", "e", KBD_e},
        {"R", "r", KBD_r},
        {"T", "t", KBD_t},
        {"Y", "y", KBD_y},
        {"U", "u", KBD_u},
        {"I", "i", KBD_i},
        {"O", "o", KBD_o},
        {"P", "p", KBD_p},
        {"[{", "lbracket", KBD_leftbracket},
        {"]}", "rbracket", KBD_rightbracket},
};

static KeyBlock combo_3[12]={
	{"A","a",KBD_a},			{"S","s",KBD_s},	{"D","d",KBD_d},
	{"F","f",KBD_f},			{"G","g",KBD_g},	{"H","h",KBD_h},
	{"J","j",KBD_j},			{"K","k",KBD_k},	{"L","l",KBD_l},
	{";:","semicolon",KBD_semicolon},				{"'\"","quote",KBD_quote},
	{"\\|","backslash",KBD_backslash},
};

static KeyBlock combo_4[12] = {{"\\|", "oem102", KBD_oem102},
                               {"Z", "z", KBD_z},
                               {"X", "x", KBD_x},
                               {"C", "c", KBD_c},
                               {"V", "v", KBD_v},
                               {"B", "b", KBD_b},
                               {"N", "n", KBD_n},
                               {"M", "m", KBD_m},
                               {",<", "comma", KBD_comma},
                               {".>", "period", KBD_period},
                               {"/?", "slash", KBD_slash},
                               {"/?", "abnt1", KBD_abnt1}};

static CKeyEvent * caps_lock_event=nullptr;
static CKeyEvent * num_lock_event=nullptr;

static void CreateLayout() {
	int32_t i;
	/* Create the buttons for the Keyboard */
	constexpr int32_t button_width  = 28;
	constexpr int32_t button_height = 20;
	constexpr int32_t margin        = 5;
	constexpr auto pos_x = [](int32_t x) { return x * button_width + margin; };
	constexpr auto pos_y = [](int32_t y) { return 10 + y * button_height; };
	AddKeyButtonEvent(pos_x(0), pos_y(0), button_width, button_height, "ESC", "esc", KBD_esc);
	for (i = 0; i < 12; i++) {
		AddKeyButtonEvent(pos_x(2 + i), pos_y(0), button_width, button_height, combo_f[i].title, combo_f[i].entry, combo_f[i].key);
	}
	for (i = 0; i < 14; i++) {
		AddKeyButtonEvent(pos_x(i), pos_y(1), button_width, button_height, combo_1[i].title, combo_1[i].entry, combo_1[i].key);
	}

	AddKeyButtonEvent(pos_x(0), pos_y(2), button_width * 2, button_height, "TAB", "tab", KBD_tab);
	for (i = 0; i < 12; i++) {
		AddKeyButtonEvent(pos_x(2 + i), pos_y(2), button_width, button_height, combo_2[i].title, combo_2[i].entry, combo_2[i].key);
	}

	AddKeyButtonEvent(pos_x(14), pos_y(2), button_width * 2, button_height * 2, "ENTER", "enter", KBD_enter);

	caps_lock_event = AddKeyButtonEvent(pos_x(0), pos_y(3), button_width * 2, button_height, "CLCK", "capslock", KBD_capslock);
	for (i = 0; i < 12; i++) {
		AddKeyButtonEvent(pos_x(2 + i), pos_y(3), button_width, button_height, combo_3[i].title, combo_3[i].entry, combo_3[i].key);
	}

	AddKeyButtonEvent(pos_x(0), pos_y(4), button_width * 2, button_height, "SHIFT", "lshift", KBD_leftshift);
	for (i = 0; i < 12; i++) {
		AddKeyButtonEvent(pos_x(2 + i),
		                  pos_y(4),
		                  button_width,
		                  button_height,
		                  combo_4[i].title,
		                  combo_4[i].entry,
		                  combo_4[i].key);
	}
	AddKeyButtonEvent(pos_x(14), pos_y(4), button_width * 3, button_height, "SHIFT", "rshift", KBD_rightshift);

	/* Bottom Row */
	AddKeyButtonEvent(pos_x(0), pos_y(5), button_width * 2, button_height, MMOD1_NAME, "lctrl", KBD_leftctrl);

#if !defined(MACOSX)
	AddKeyButtonEvent(pos_x(2), pos_y(5), button_width * 2, button_height, MMOD3_NAME, "lgui", KBD_leftgui);
	AddKeyButtonEvent(pos_x(4), pos_y(5), button_width * 2, button_height, MMOD2_NAME, "lalt", KBD_leftalt);
#else
	AddKeyButtonEvent(pos_x(2), pos_y(5), button_width * 2, button_height, MMOD2_NAME, "lalt", KBD_leftalt);
	AddKeyButtonEvent(pos_x(4), pos_y(5), button_width * 2, button_height, MMOD3_NAME, "lgui", KBD_leftgui);
#endif

	AddKeyButtonEvent(pos_x(6), pos_y(5), button_width * 4, button_height, "SPACE", "space", KBD_space);

#if !defined(MACOSX)
	AddKeyButtonEvent(pos_x(10), pos_y(5), button_width * 2, button_height, MMOD2_NAME, "ralt", KBD_rightalt);
	AddKeyButtonEvent(pos_x(12), pos_y(5), button_width * 2, button_height, MMOD3_NAME, "rgui", KBD_rightgui);
#else
	AddKeyButtonEvent(pos_x(10), pos_y(5), button_width * 2, button_height, MMOD3_NAME, "rgui", KBD_rightgui);
	AddKeyButtonEvent(pos_x(12), pos_y(5), button_width * 2, button_height, MMOD2_NAME, "ralt", KBD_rightalt);
#endif

	AddKeyButtonEvent(pos_x(14), pos_y(5), button_width * 2, button_height, MMOD1_NAME, "rctrl", KBD_rightctrl);

	/* Arrow Keys */
#define XO 17
#define YO 0

	AddKeyButtonEvent(pos_x(XO + 0), pos_y(YO), button_width, button_height, "PRT", "printscreen", KBD_printscreen);
	AddKeyButtonEvent(pos_x(XO + 1), pos_y(YO), button_width, button_height, "SCL", "scrolllock", KBD_scrolllock);
	AddKeyButtonEvent(pos_x(XO + 2), pos_y(YO), button_width, button_height, "PAU", "pause", KBD_pause);
	AddKeyButtonEvent(pos_x(XO + 0), pos_y(YO + 1), button_width, button_height, "INS", "insert", KBD_insert);
	AddKeyButtonEvent(pos_x(XO + 1), pos_y(YO + 1), button_width, button_height, "HOM", "home", KBD_home);
	AddKeyButtonEvent(pos_x(XO + 2), pos_y(YO + 1), button_width, button_height, "PUP", "pageup", KBD_pageup);
	AddKeyButtonEvent(pos_x(XO + 0), pos_y(YO + 2), button_width, button_height, "DEL", "delete", KBD_delete);
	AddKeyButtonEvent(pos_x(XO + 1), pos_y(YO + 2), button_width, button_height, "END", "end", KBD_end);
	AddKeyButtonEvent(pos_x(XO + 2), pos_y(YO + 2), button_width, button_height, "PDN", "pagedown", KBD_pagedown);
	AddKeyButtonEvent(pos_x(XO + 1), pos_y(YO + 4), button_width, button_height, "\x18", "up", KBD_up);
	AddKeyButtonEvent(pos_x(XO + 0), pos_y(YO + 5), button_width, button_height, "\x1B", "left", KBD_left);
	AddKeyButtonEvent(pos_x(XO + 1), pos_y(YO + 5), button_width, button_height, "\x19", "down", KBD_down);
	AddKeyButtonEvent(pos_x(XO + 2), pos_y(YO + 5), button_width, button_height, "\x1A", "right", KBD_right);
#undef XO
#undef YO
#define XO 0
#define YO 7
	/* Numeric KeyPad */
	num_lock_event = AddKeyButtonEvent(pos_x(XO), pos_y(YO), button_width, button_height, "NUM", "numlock", KBD_numlock);
	AddKeyButtonEvent(pos_x(XO + 1), pos_y(YO), button_width, button_height, "/", "kp_divide", KBD_kpdivide);
	AddKeyButtonEvent(pos_x(XO + 2), pos_y(YO), button_width, button_height, "*", "kp_multiply", KBD_kpmultiply);
	AddKeyButtonEvent(pos_x(XO + 3), pos_y(YO), button_width, button_height, "-", "kp_minus", KBD_kpminus);
	AddKeyButtonEvent(pos_x(XO + 0), pos_y(YO + 1), button_width, button_height, "7", "kp_7", KBD_kp7);
	AddKeyButtonEvent(pos_x(XO + 1), pos_y(YO + 1), button_width, button_height, "8", "kp_8", KBD_kp8);
	AddKeyButtonEvent(pos_x(XO + 2), pos_y(YO + 1), button_width, button_height, "9", "kp_9", KBD_kp9);
	AddKeyButtonEvent(pos_x(XO + 3), pos_y(YO + 1), button_width, button_height * 2, "+", "kp_plus", KBD_kpplus);
	AddKeyButtonEvent(pos_x(XO), pos_y(YO + 2), button_width, button_height, "4", "kp_4", KBD_kp4);
	AddKeyButtonEvent(pos_x(XO + 1), pos_y(YO + 2), button_width, button_height, "5", "kp_5", KBD_kp5);
	AddKeyButtonEvent(pos_x(XO + 2), pos_y(YO + 2), button_width, button_height, "6", "kp_6", KBD_kp6);
	AddKeyButtonEvent(pos_x(XO + 0), pos_y(YO + 3), button_width, button_height, "1", "kp_1", KBD_kp1);
	AddKeyButtonEvent(pos_x(XO + 1), pos_y(YO + 3), button_width, button_height, "2", "kp_2", KBD_kp2);
	AddKeyButtonEvent(pos_x(XO + 2), pos_y(YO + 3), button_width, button_height, "3", "kp_3", KBD_kp3);
	AddKeyButtonEvent(pos_x(XO + 3), pos_y(YO + 3), button_width, button_height * 2, "ENT", "kp_enter", KBD_kpenter);
	AddKeyButtonEvent(pos_x(XO), pos_y(YO + 4), button_width * 2, button_height, "0", "kp_0", KBD_kp0);
	AddKeyButtonEvent(pos_x(XO + 2), pos_y(YO + 4), button_width, button_height, ".", "kp_period", KBD_kpperiod);

#undef XO
#undef YO

#define XO 5
#define YO 8
	/* Mouse Buttons */
	new CTextButton(pos_x(XO + 0), pos_y(YO - 1), 3 * button_width, 20, "Mouse");

	AddMouseButtonEvent(pos_x(XO + 0),
	                    pos_y(YO),
	                    button_width,
	                    button_height,
	                    "L",
	                    "mouse_left",
	                    MouseButtonId::Left);

	AddMouseButtonEvent(pos_x(XO + 1),
	                    pos_y(YO),
	                    button_width,
	                    button_height,
	                    "M",
	                    "mouse_middle",
	                    MouseButtonId::Middle);

	AddMouseButtonEvent(pos_x(XO + 2),
	                    pos_y(YO),
	                    button_width,
	                    button_height,
	                    "R",
	                    "mouse_right",
	                    MouseButtonId::Right);
#undef XO
#undef YO

#define XO 10
#define YO 7
	/* Joystick Buttons/Texts */
	/* Buttons 1+2 of 1st Joystick */
	AddJButtonButton(pos_x(XO), pos_y(YO), button_width, button_height, "1", 0, 0);
	AddJButtonButton(pos_x(XO + 2), pos_y(YO), button_width, button_height, "2", 0, 1);
	/* Axes 1+2 (X+Y) of 1st Joystick */
	CJAxisEvent* cjaxis = AddJAxisButton(pos_x(XO + 1), pos_y(YO), button_width, button_height, "Y-", 0, 1, false, nullptr);
	AddJAxisButton(pos_x(XO + 1), pos_y(YO + 1), button_width, button_height, "Y+", 0, 1, true, cjaxis);
	cjaxis = AddJAxisButton(pos_x(XO), pos_y(YO + 1), button_width, button_height, "X-", 0, 0, false, nullptr);
	AddJAxisButton(pos_x(XO + 2), pos_y(YO + 1), button_width, button_height, "X+", 0, 0, true, cjaxis);

	CJAxisEvent * tmp_ptr;

	assert(joytype != JOY_UNSET);
	if (joytype == JOY_2AXIS) {
		/* Buttons 1+2 of 2nd Joystick */
		AddJButtonButton(pos_x(XO + 4), pos_y(YO), button_width, button_height, "1", 1, 0);
		AddJButtonButton(pos_x(XO + 4 + 2), pos_y(YO), button_width, button_height, "2", 1, 1);
		/* Buttons 3+4 of 1st Joystick, not accessible */
		AddJButtonButton_hidden(0,2);
		AddJButtonButton_hidden(0,3);

		/* Axes 1+2 (X+Y) of 2nd Joystick */
		cjaxis  = AddJAxisButton(pos_x(XO + 4), pos_y(YO + 1), button_width, button_height, "X-", 1, 0, false, nullptr);
		tmp_ptr = AddJAxisButton(pos_x(XO + 4 + 2), pos_y(YO + 1), button_width, button_height, "X+", 1, 0, true, cjaxis);
		(void)tmp_ptr;
		cjaxis  = AddJAxisButton(pos_x(XO + 4 + 1), pos_y(YO + 0), button_width, button_height, "Y-", 1, 1, false, nullptr);
		tmp_ptr = AddJAxisButton(pos_x(XO + 4 + 1), pos_y(YO + 1), button_width, button_height, "Y+", 1, 1, true, cjaxis);
		(void)tmp_ptr;
		/* Axes 3+4 (X+Y) of 1st Joystick, not accessible */
		cjaxis  = AddJAxisButton_hidden(0, 2, false, nullptr);
		tmp_ptr = AddJAxisButton_hidden(0, 2, true, cjaxis);
		(void)tmp_ptr;
		cjaxis  = AddJAxisButton_hidden(0, 3, false, nullptr);
		tmp_ptr = AddJAxisButton_hidden(0, 3, true, cjaxis);
		(void)tmp_ptr;
	} else {
		/* Buttons 3+4 of 1st Joystick */
		AddJButtonButton(pos_x(XO + 4), pos_y(YO), button_width, button_height, "3", 0, 2);
		AddJButtonButton(pos_x(XO + 4 + 2), pos_y(YO), button_width, button_height, "4", 0, 3);
		/* Buttons 1+2 of 2nd Joystick, not accessible */
		AddJButtonButton_hidden(1, 0);
		AddJButtonButton_hidden(1, 1);

		/* Axes 3+4 (X+Y) of 1st Joystick */
		cjaxis  = AddJAxisButton(pos_x(XO + 4), pos_y(YO + 1), button_width, button_height, "X-", 0, 2, false, nullptr);
		tmp_ptr = AddJAxisButton(pos_x(XO + 4 + 2), pos_y(YO + 1), button_width, button_height, "X+", 0, 2, true, cjaxis);
		(void)tmp_ptr;
		cjaxis  = AddJAxisButton(pos_x(XO + 4 + 1), pos_y(YO + 0), button_width, button_height, "Y-", 0, 3, false, nullptr);
		tmp_ptr = AddJAxisButton(pos_x(XO + 4 + 1), pos_y(YO + 1), button_width, button_height, "Y+", 0, 3, true, cjaxis);
		(void)tmp_ptr;
		/* Axes 1+2 (X+Y) of 2nd Joystick , not accessible*/
		cjaxis  = AddJAxisButton_hidden(1, 0, false, nullptr);
		tmp_ptr = AddJAxisButton_hidden(1, 0, true, cjaxis);
		(void)tmp_ptr;
		cjaxis  = AddJAxisButton_hidden(1, 1, false, nullptr);
		tmp_ptr = AddJAxisButton_hidden(1, 1, true, cjaxis);
		(void)tmp_ptr;
	}

	if (joytype == JOY_CH) {
		/* Buttons 5+6 of 1st Joystick */
		AddJButtonButton(pos_x(XO + 8), pos_y(YO), button_width, button_height, "5", 0, 4);
		AddJButtonButton(pos_x(XO + 8 + 2), pos_y(YO), button_width, button_height, "6", 0, 5);
	} else {
		/* Buttons 5+6 of 1st Joystick, not accessible */
		AddJButtonButton_hidden(0, 4);
		AddJButtonButton_hidden(0, 5);
	}

	/* Hat directions up, left, down, right */
	AddJHatButton(pos_x(XO + 8 + 1), pos_y(YO), button_width, button_height, "UP", 0, 0, 0);
	AddJHatButton(pos_x(XO + 8 + 0), pos_y(YO + 1), button_width, button_height, "LFT", 0, 0, 3);
	AddJHatButton(pos_x(XO + 8 + 1), pos_y(YO + 1), button_width, button_height, "DWN", 0, 0, 2);
	AddJHatButton(pos_x(XO + 8 + 2), pos_y(YO + 1), button_width, button_height, "RGT", 0, 0, 1);

	/* Labels for the joystick */
	CTextButton * btn;
	if (joytype == JOY_2AXIS) {
		new CTextButton(pos_x(XO + 0), pos_y(YO - 1), 3 * button_width, 20, "Joystick 1");
		new CTextButton(pos_x(XO + 4), pos_y(YO - 1), 3 * button_width, 20, "Joystick 2");
		btn = new CTextButton(pos_x(XO + 8), pos_y(YO - 1), 3 * button_width, 20, "Disabled");
		btn->SetColor(color_grey);
	} else if(joytype == JOY_4AXIS || joytype == JOY_4AXIS_2) {
		new CTextButton(pos_x(XO + 0), pos_y(YO - 1), 3 * button_width, 20, "Axis 1/2");
		new CTextButton(pos_x(XO + 4), pos_y(YO - 1), 3 * button_width, 20, "Axis 3/4");
		btn = new CTextButton(pos_x(XO + 8), pos_y(YO - 1), 3 * button_width, 20, "Disabled");
		btn->SetColor(color_grey);
	} else if(joytype == JOY_CH) {
		new CTextButton(pos_x(XO + 0), pos_y(YO - 1), 3 * button_width, 20, "Axis 1/2");
		new CTextButton(pos_x(XO + 4), pos_y(YO - 1), 3 * button_width, 20, "Axis 3/4");
		new CTextButton(pos_x(XO + 8), pos_y(YO - 1), 3 * button_width, 20, "Hat/D-pad");
	} else if ( joytype == JOY_FCS) {
		new CTextButton(pos_x(XO + 0), pos_y(YO - 1), 3 * button_width, 20, "Axis 1/2");
		new CTextButton(pos_x(XO + 4), pos_y(YO - 1), 3 * button_width, 20, "Axis 3");
		new CTextButton(pos_x(XO + 8), pos_y(YO - 1), 3 * button_width, 20, "Hat/D-pad");
	} else if (joytype == JOY_DISABLED) {
		btn = new CTextButton(pos_x(XO + 0), pos_y(YO - 1), 3 * button_width, 20, "Disabled");
		btn->SetColor(color_grey);
		btn = new CTextButton(pos_x(XO + 4), pos_y(YO - 1), 3 * button_width, 20, "Disabled");
		btn->SetColor(color_grey);
		btn = new CTextButton(pos_x(XO + 8), pos_y(YO - 1), 3 * button_width, 20, "Disabled");
		btn->SetColor(color_grey);
	}

	/* The modifier buttons */
	AddModButton(pos_x(0), pos_y(14), 50, 20, "Mod1", 1);
	AddModButton(pos_x(2), pos_y(14), 50, 20, "Mod2", 2);
	AddModButton(pos_x(4), pos_y(14), 50, 20, "Mod3", 3);

	/* Create Handler buttons */
	int32_t xpos = 0;
	int32_t ypos = 10;
	constexpr auto bw = button_width + 5;
	for (const auto &handler_event : handlergroup) {
		new CEventButton(200 + xpos * 3 * bw, pos_y(ypos), bw * 3, button_height,
		                 handler_event->button_name.c_str(), handler_event);
		xpos++;
		if (xpos>3) {
			xpos = 0;
			ypos++;
		}
	}
	/* Create some text buttons */
//	new CTextButton(pos_x(6), 0, 124, 20, "Keyboard Layout");
//	new CTextButton(pos_x(17), 0, 124, 20, "Joystick Layout");

	bind_but.action = new CCaptionButton(0, 335, 0, 0);

	bind_but.event_title=new CCaptionButton(0,350,0,0);
	bind_but.bind_title=new CCaptionButton(0,365,0,0);

	/* Create binding support buttons */

	bind_but.mod1 = new CCheckButton(20, 410, 110, 20, "Mod1", BC_Mod1);
	bind_but.mod2 = new CCheckButton(20, 432, 110, 20, "Mod2", BC_Mod2);
	bind_but.mod3 = new CCheckButton(20, 454, 110, 20, "Mod3", BC_Mod3);
	bind_but.hold = new CCheckButton(150, 410, 60, 20, "Hold", BC_Hold);

	bind_but.add = new CBindButton(250, 380, 100, 20, "Add bind", BB_Add);
	bind_but.del = new CBindButton(250, 400, 100, 20, "Remove bind", BB_Del);
	bind_but.next = new CBindButton(250, 420, 100, 20, "Next bind", BB_Next);

	bind_but.save=new CBindButton(400,450,50,20,"Save",BB_Save);
	bind_but.exit=new CBindButton(450,450,50,20,"Exit",BB_Exit);

	bind_but.bind_title->Change("Bind Title");
}

static void CreateStringBind(char * line) {
	line=trim(line);
	char * eventname=strip_word(line);
	CEvent * event = nullptr;
	for (const auto& evt : events) {
		if (!strcasecmp(evt->GetName(), eventname)) {
			event = evt.get();
			goto foundevent;
		}
	}
	LOG_WARNING("MAPPER: Can't find key binding for '%s' event", eventname);
	return ;
foundevent:
	CBind * bind = nullptr;
	for (char * bindline=strip_word(line);*bindline;bindline=strip_word(line)) {
		for (CBindGroup_it it = bindgroups.begin(); it != bindgroups.end(); ++it) {
			bind=(*it)->CreateConfigBind(bindline);
			if (bind) {
				event->AddBind(bind);
				bind->SetFlags(bindline);
				break;
			}
		}
	}
}

static struct {
	const char *eventend;
	SDL_Scancode key;
} DefaultKeys[] = {{"f1", SDL_SCANCODE_F1},
                   {"f2", SDL_SCANCODE_F2},
                   {"f3", SDL_SCANCODE_F3},
                   {"f4", SDL_SCANCODE_F4},
                   {"f5", SDL_SCANCODE_F5},
                   {"f6", SDL_SCANCODE_F6},
                   {"f7", SDL_SCANCODE_F7},
                   {"f8", SDL_SCANCODE_F8},
                   {"f9", SDL_SCANCODE_F9},
                   {"f10", SDL_SCANCODE_F10},
                   {"f11", SDL_SCANCODE_F11},
                   {"f12", SDL_SCANCODE_F12},

                   {"1", SDL_SCANCODE_1},
                   {"2", SDL_SCANCODE_2},
                   {"3", SDL_SCANCODE_3},
                   {"4", SDL_SCANCODE_4},
                   {"5", SDL_SCANCODE_5},
                   {"6", SDL_SCANCODE_6},
                   {"7", SDL_SCANCODE_7},
                   {"8", SDL_SCANCODE_8},
                   {"9", SDL_SCANCODE_9},
                   {"0", SDL_SCANCODE_0},

                   {"a", SDL_SCANCODE_A},
                   {"b", SDL_SCANCODE_B},
                   {"c", SDL_SCANCODE_C},
                   {"d", SDL_SCANCODE_D},
                   {"e", SDL_SCANCODE_E},
                   {"f", SDL_SCANCODE_F},
                   {"g", SDL_SCANCODE_G},
                   {"h", SDL_SCANCODE_H},
                   {"i", SDL_SCANCODE_I},
                   {"j", SDL_SCANCODE_J},
                   {"k", SDL_SCANCODE_K},
                   {"l", SDL_SCANCODE_L},
                   {"m", SDL_SCANCODE_M},
                   {"n", SDL_SCANCODE_N},
                   {"o", SDL_SCANCODE_O},
                   {"p", SDL_SCANCODE_P},
                   {"q", SDL_SCANCODE_Q},
                   {"r", SDL_SCANCODE_R},
                   {"s", SDL_SCANCODE_S},
                   {"t", SDL_SCANCODE_T},
                   {"u", SDL_SCANCODE_U},
                   {"v", SDL_SCANCODE_V},
                   {"w", SDL_SCANCODE_W},
                   {"x", SDL_SCANCODE_X},
                   {"y", SDL_SCANCODE_Y},
                   {"z", SDL_SCANCODE_Z},

                   {"space", SDL_SCANCODE_SPACE},
                   {"esc", SDL_SCANCODE_ESCAPE},
                   {"equals", SDL_SCANCODE_EQUALS},
                   {"grave", SDL_SCANCODE_GRAVE},
                   {"tab", SDL_SCANCODE_TAB},
                   {"enter", SDL_SCANCODE_RETURN},
                   {"bspace", SDL_SCANCODE_BACKSPACE},
                   {"lbracket", SDL_SCANCODE_LEFTBRACKET},
                   {"rbracket", SDL_SCANCODE_RIGHTBRACKET},
                   {"minus", SDL_SCANCODE_MINUS},
                   {"capslock", SDL_SCANCODE_CAPSLOCK},
                   {"semicolon", SDL_SCANCODE_SEMICOLON},
                   {"quote", SDL_SCANCODE_APOSTROPHE},
                   {"backslash", SDL_SCANCODE_BACKSLASH},
                   {"lshift", SDL_SCANCODE_LSHIFT},
                   {"rshift", SDL_SCANCODE_RSHIFT},
                   {"lalt", SDL_SCANCODE_LALT},
                   {"ralt", SDL_SCANCODE_RALT},
                   {"lctrl", SDL_SCANCODE_LCTRL},
                   {"rctrl", SDL_SCANCODE_RCTRL},
                   {"lgui", SDL_SCANCODE_LGUI},
                   {"rgui", SDL_SCANCODE_RGUI},
                   {"comma", SDL_SCANCODE_COMMA},
                   {"period", SDL_SCANCODE_PERIOD},
                   {"slash", SDL_SCANCODE_SLASH},
                   {"printscreen", SDL_SCANCODE_PRINTSCREEN},
                   {"scrolllock", SDL_SCANCODE_SCROLLLOCK},
                   {"pause", SDL_SCANCODE_PAUSE},
                   {"pagedown", SDL_SCANCODE_PAGEDOWN},
                   {"pageup", SDL_SCANCODE_PAGEUP},
                   {"insert", SDL_SCANCODE_INSERT},
                   {"home", SDL_SCANCODE_HOME},
                   {"delete", SDL_SCANCODE_DELETE},
                   {"end", SDL_SCANCODE_END},
                   {"up", SDL_SCANCODE_UP},
                   {"left", SDL_SCANCODE_LEFT},
                   {"down", SDL_SCANCODE_DOWN},
                   {"right", SDL_SCANCODE_RIGHT},

                   {"kp_1", SDL_SCANCODE_KP_1},
                   {"kp_2", SDL_SCANCODE_KP_2},
                   {"kp_3", SDL_SCANCODE_KP_3},
                   {"kp_4", SDL_SCANCODE_KP_4},
                   {"kp_5", SDL_SCANCODE_KP_5},
                   {"kp_6", SDL_SCANCODE_KP_6},
                   {"kp_7", SDL_SCANCODE_KP_7},
                   {"kp_8", SDL_SCANCODE_KP_8},
                   {"kp_9", SDL_SCANCODE_KP_9},
                   {"kp_0", SDL_SCANCODE_KP_0},

                   {"numlock", SDL_SCANCODE_NUMLOCKCLEAR},
                   {"kp_divide", SDL_SCANCODE_KP_DIVIDE},
                   {"kp_multiply", SDL_SCANCODE_KP_MULTIPLY},
                   {"kp_minus", SDL_SCANCODE_KP_MINUS},
                   {"kp_plus", SDL_SCANCODE_KP_PLUS},
                   {"kp_period", SDL_SCANCODE_KP_PERIOD},
                   {"kp_enter", SDL_SCANCODE_KP_ENTER},

                   // ABNT-arrangement, key between Left-Shift and Z: SDL
                   // scancode 100 (0x64) maps to OEM102 key with scancode 86
                   // (0x56)
                   {"oem102", SDL_SCANCODE_NONUSBACKSLASH},

                   // ABNT-arrangement, key between Left-Shift and Z: SDL
                   // scancode 135 (0x87) maps to first ABNT key with scancode
                   // 115 (0x73)
                   {"abnt1", SDL_SCANCODE_INTERNATIONAL1},

                   {nullptr, SDL_SCANCODE_UNKNOWN}};

static void ClearAllBinds() {
	// wait for the auto-typer to complete because it might be accessing events
	mapper.typist.Wait();

	for (const auto& event : events) {
		event->ClearBinds();
	}
}

static void CreateDefaultBinds() {
	ClearAllBinds();
	char buffer[512];
	Bitu i=0;
	while (DefaultKeys[i].eventend) {
		sprintf(buffer, "key_%s \"key %d\"",
		        DefaultKeys[i].eventend,
		        static_cast<int>(DefaultKeys[i].key));
		CreateStringBind(buffer);
		i++;
	}
	sprintf(buffer, "mod_1 \"key %d\"", SDL_SCANCODE_RCTRL);
	CreateStringBind(buffer);
	sprintf(buffer, "mod_1 \"key %d\"", SDL_SCANCODE_LCTRL);
	CreateStringBind(buffer);
	sprintf(buffer, "mod_2 \"key %d\"", SDL_SCANCODE_RALT);
	CreateStringBind(buffer);
	sprintf(buffer, "mod_2 \"key %d\"", SDL_SCANCODE_LALT);
	CreateStringBind(buffer);
	sprintf(buffer, "mod_3 \"key %d\"", SDL_SCANCODE_RGUI);
	CreateStringBind(buffer);
	sprintf(buffer, "mod_3 \"key %d\"", SDL_SCANCODE_LGUI);
	CreateStringBind(buffer);
	for (const auto &handler_event : handlergroup) {
		handler_event->MakeDefaultBind(buffer);
		CreateStringBind(buffer);
	}

	/* joystick1, buttons 1-6 */
	sprintf(buffer,"jbutton_0_0 \"stick_0 button 0\" ");CreateStringBind(buffer);
	sprintf(buffer,"jbutton_0_1 \"stick_0 button 1\" ");CreateStringBind(buffer);
	sprintf(buffer,"jbutton_0_2 \"stick_0 button 2\" ");CreateStringBind(buffer);
	sprintf(buffer,"jbutton_0_3 \"stick_0 button 3\" ");CreateStringBind(buffer);
	sprintf(buffer,"jbutton_0_4 \"stick_0 button 4\" ");CreateStringBind(buffer);
	sprintf(buffer,"jbutton_0_5 \"stick_0 button 5\" ");CreateStringBind(buffer);
	/* joystick2, buttons 1-2 */
	sprintf(buffer,"jbutton_1_0 \"stick_1 button 0\" ");CreateStringBind(buffer);
	sprintf(buffer,"jbutton_1_1 \"stick_1 button 1\" ");CreateStringBind(buffer);

	/* joystick1, axes 1-4 */
	sprintf(buffer,"jaxis_0_0- \"stick_0 axis 0 0\" ");CreateStringBind(buffer);
	sprintf(buffer,"jaxis_0_0+ \"stick_0 axis 0 1\" ");CreateStringBind(buffer);
	sprintf(buffer,"jaxis_0_1- \"stick_0 axis 1 0\" ");CreateStringBind(buffer);
	sprintf(buffer,"jaxis_0_1+ \"stick_0 axis 1 1\" ");CreateStringBind(buffer);
	sprintf(buffer,"jaxis_0_2- \"stick_0 axis 2 0\" ");CreateStringBind(buffer);
	sprintf(buffer,"jaxis_0_2+ \"stick_0 axis 2 1\" ");CreateStringBind(buffer);
	sprintf(buffer,"jaxis_0_3- \"stick_0 axis 3 0\" ");CreateStringBind(buffer);
	sprintf(buffer,"jaxis_0_3+ \"stick_0 axis 3 1\" ");CreateStringBind(buffer);
	/* joystick2, axes 1-2 */
	sprintf(buffer,"jaxis_1_0- \"stick_1 axis 0 0\" ");CreateStringBind(buffer);
	sprintf(buffer,"jaxis_1_0+ \"stick_1 axis 0 1\" ");CreateStringBind(buffer);
	sprintf(buffer,"jaxis_1_1- \"stick_1 axis 1 0\" ");CreateStringBind(buffer);
	sprintf(buffer,"jaxis_1_1+ \"stick_1 axis 1 1\" ");CreateStringBind(buffer);

	/* joystick1, hat */
	sprintf(buffer,"jhat_0_0_0 \"stick_0 hat 0 1\" ");CreateStringBind(buffer);
	sprintf(buffer,"jhat_0_0_1 \"stick_0 hat 0 2\" ");CreateStringBind(buffer);
	sprintf(buffer,"jhat_0_0_2 \"stick_0 hat 0 4\" ");CreateStringBind(buffer);
	sprintf(buffer,"jhat_0_0_3 \"stick_0 hat 0 8\" ");CreateStringBind(buffer);
	LOG_MSG("MAPPER: Loaded default key bindings");
}

void MAPPER_AddHandler(MAPPER_Handler *handler,
                       SDL_Scancode key,
                       uint32_t mods,
                       const char *event_name,
                       const char *button_name)
{
	//Check if it already exists=> if so return.
	for (const auto &handler_event : handlergroup) {
		if (handler_event->button_name == button_name)
			return;
	}

	char tempname[17];
	safe_strcpy(tempname, "hand_");
	safe_strcat(tempname, event_name);
	new CHandlerEvent(tempname, handler, key, mods, button_name);
	return ;
}

static void MAPPER_SaveBinds() {
	const char *filename = mapper.filename.c_str();
	FILE * savefile=fopen(filename,"wt+");
	if (!savefile) {
		LOG_MSG("MAPPER: Can't open %s for saving the key bindings", filename);
		return;
	}
	char buf[128];
	for (const auto& event : events) {
		fprintf(savefile,"%s ",event->GetName());
		for (CBindList_it bind_it = event->bindlist.begin(); bind_it != event->bindlist.end(); ++bind_it) {
			CBind * bind=*(bind_it);
			bind->ConfigName(buf);
			bind->AddFlags(buf);
			fprintf(savefile,"\"%s\" ",buf);
		}
		fprintf(savefile,"\n");
	}
	fclose(savefile);
	change_action_text("Mapper file saved.", color_white);
	LOG_MSG("MAPPER: Wrote key bindings to %s", filename);
}

static bool load_binds_from_file(const std::string_view mapperfile_path,
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

		mapper.filename = mapper_path.string();
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

void MAPPER_CheckEvent(SDL_Event *event)
{
	for (auto &group : bindgroups)
		if (group->CheckEvent(event))
			return;
}

void BIND_MappingEvents() {
	SDL_Event event;
	static bool isButtonPressed = false;
	static CButton *lastHoveredButton = nullptr;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_MOUSEBUTTONDOWN:
			isButtonPressed = true;
			/* Further check where are we pointing at right now */
			[[fallthrough]];
		case SDL_MOUSEMOTION:
			if (!isButtonPressed)
				break;
			/* Maybe we have been pointing at a specific button for
			 * a little while  */
			if (lastHoveredButton) {
				/* Check if there's any change */
				if (lastHoveredButton->OnTop(event.button.x,event.button.y))
					break;
				/* Not pointing at given button anymore */
				if (lastHoveredButton == last_clicked)
					lastHoveredButton->Click();
				else
					lastHoveredButton->BindColor();
				mapper.redraw = true;
				lastHoveredButton = nullptr;
			}
			/* Check which button are we currently pointing at */
			for (const auto& button : buttons) {
				if (dynamic_cast<CClickableTextButton*>(button.get()) &&
				    button->OnTop(event.button.x, event.button.y)) {
					button->SetColor(color_red);
					mapper.redraw     = true;
					lastHoveredButton = button.get();
					break;
				}
			}
			break;
		case SDL_MOUSEBUTTONUP:
			isButtonPressed = false;
			if (lastHoveredButton) {
				/* For most buttons the actual new color is going to be green; But not for a few others. */
				lastHoveredButton->BindColor();
				mapper.redraw = true;
				lastHoveredButton = nullptr;
			}
			/* Check the press */
			for (const auto& button : buttons) {
				if (dynamic_cast<CClickableTextButton*>(button.get()) &&
				    button->OnTop(event.button.x, event.button.y)) {
					button->Click();
					break;
				}
			}
			SetActiveBind(mapper.abind); // force redraw key binding
			                             // description
			break;
		case SDL_WINDOWEVENT:
			/* The resize event MAY arrive e.g. when the mapper is
			 * toggled, at least on X11. Furthermore, the restore
			 * event should be handled on Android.
			 */
			if ((event.window.event == SDL_WINDOWEVENT_RESIZED) ||
			    (event.window.event == SDL_WINDOWEVENT_RESTORED)) {
				GFX_UpdateDisplayDimensions(event.window.data1,
				                            event.window.data2);
				SDL_RenderSetLogicalSize(mapper.renderer, 640, 480);
				mapper.redraw = true;
			}
			if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
				mapper.redraw = true;
			}
			break;
		case SDL_QUIT:
			isButtonPressed = false;
			lastHoveredButton = nullptr;
			mapper.exit=true;
			break;
		default:
			if (mapper.addbind) for (CBindGroup_it it = bindgroups.begin(); it != bindgroups.end(); ++it) {
				CBind * newbind=(*it)->CreateEventBind(&event);
				if (!newbind) continue;
				mapper.aevent->AddBind(newbind);
				SetActiveEvent(mapper.aevent);
				mapper.addbind=false;
				break;
			}
		}
	}
}

//  Initializes SDL's joystick subsystem an setups up initial joystick settings.

// If the user wants auto-configuration, then this sets joytype based on queried
// results. If no joysticks are valid then joytype is set to JOY_NONE_FOUND.
// This also resets mapper.sticks.num_groups to 0 and mapper.sticks.num to the
// number of found SDL joysticks.

// 7-21-2023: No longer resetting mapper.sticks.num_groups due to
// https://github.com/dosbox-staging/dosbox-staging/issues/2687
static void QueryJoysticks()
{
	// Reset our joystick status
	mapper.sticks.num = 0;

	JOYSTICK_ParseConfiguredType();

	// The user doesn't want to use joysticks at all (not even for mapping)
	if (joytype == JOY_DISABLED) {
		LOG_INFO("MAPPER: Joystick subsystem disabled");
		return;
	}

	if (SDL_WasInit(SDL_INIT_JOYSTICK) != SDL_INIT_JOYSTICK)
		SDL_InitSubSystem(SDL_INIT_JOYSTICK);

	const bool wants_auto_config = joytype & (JOY_AUTO | JOY_ONLY_FOR_MAPPING);

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
	mapper.sticks.num = static_cast<unsigned int>(num_joysticks);
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

static void CreateBindGroups() {
	bindgroups.clear();
	CKeyBindGroup* key_bind_group = new CKeyBindGroup(SDL_NUM_SCANCODES);
	keybindgroups.push_back(key_bind_group);

	assert(joytype != JOY_UNSET);

	if (joytype == JOY_DISABLED)
		return;

	if (joytype != JOY_NONE_FOUND) {
#if defined (REDUCE_JOYSTICK_POLLING)
		// direct access to the SDL joystick, thus removed from the event handling
		if (mapper.sticks.num)
			SDL_JoystickEventState(SDL_DISABLE);
#else
		// enable joystick event handling
		if (mapper.sticks.num)
			SDL_JoystickEventState(SDL_ENABLE);
		else
			return;
#endif
		// Free up our previously assigned joystick slot before assinging below
		if (mapper.sticks.stick[mapper.sticks.num_groups]) {
			delete mapper.sticks.stick[mapper.sticks.num_groups];
			mapper.sticks.stick[mapper.sticks.num_groups] = nullptr;
		}

		uint8_t joyno = 0;
		switch (joytype) {
		case JOY_DISABLED:
		case JOY_NONE_FOUND: break;
		case JOY_4AXIS:
			mapper.sticks.stick[mapper.sticks.num_groups++] =
			        new C4AxisBindGroup(joyno, joyno);
			stickbindgroups.push_back(
			        new CStickBindGroup(joyno + 1U, joyno + 1U, true));
			break;
		case JOY_4AXIS_2:
			mapper.sticks.stick[mapper.sticks.num_groups++] =
			        new C4AxisBindGroup(joyno + 1U, joyno);
			stickbindgroups.push_back(
			        new CStickBindGroup(joyno, joyno + 1U, true));
			break;
		case JOY_FCS:
			mapper.sticks.stick[mapper.sticks.num_groups++] =
			        new CFCSBindGroup(joyno, joyno);
			stickbindgroups.push_back(
			        new CStickBindGroup(joyno + 1U, joyno + 1U, true));
			break;
		case JOY_CH:
			mapper.sticks.stick[mapper.sticks.num_groups++] =
			        new CCHBindGroup(joyno, joyno);
			stickbindgroups.push_back(
			        new CStickBindGroup(joyno + 1U, joyno + 1U, true));
			break;
		case JOY_AUTO:
		case JOY_ONLY_FOR_MAPPING:
		case JOY_2AXIS:
		default:
			mapper.sticks.stick[mapper.sticks.num_groups++] =
			        new CStickBindGroup(joyno, joyno);
			if ((joyno + 1U) < mapper.sticks.num) {
				delete mapper.sticks.stick[mapper.sticks.num_groups];
				mapper.sticks.stick[mapper.sticks.num_groups++] =
				        new CStickBindGroup(joyno + 1U, joyno + 1U);
			} else {
				stickbindgroups.push_back(
				        new CStickBindGroup(joyno + 1U, joyno + 1U, true));
			}
			break;
		}
	}
}

bool MAPPER_IsUsingJoysticks() {
	return (mapper.sticks.num > 0);
}

#if defined (REDUCE_JOYSTICK_POLLING)
void MAPPER_UpdateJoysticks() {
	for (Bitu i=0; i<mapper.sticks.num_groups; i++) {
		assert(mapper.sticks.stick[i]);
		mapper.sticks.stick[i]->UpdateJoystick();
	}
}
#endif

void MAPPER_LosingFocus() {
	for (const auto& event : events) {
		if (event.get() != caps_lock_event && event.get() != num_lock_event) {
			event->DeActivateAll();
		}
	}
}

void MAPPER_RunEvent(uint32_t /*val*/)
{
	KEYBOARD_ClrBuffer();           // Clear buffer
	GFX_LosingFocus();		//Release any keys pressed (buffer gets filled again).
	MAPPER_DisplayUI();
}

void MAPPER_Run(bool pressed) {
	if (pressed)
		return;
	PIC_AddEvent(MAPPER_RunEvent,0);	//In case mapper deletes the key object that ran it
}

SDL_Surface* SDL_SetVideoMode_Wrap(int width,int height,int bpp,uint32_t flags);

void MAPPER_DisplayUI() {
	MOUSE_NotifyTakeOver(true);

	// The mapper is about to take-over SDL's surface and rendering
	// functions, so disengage the main ones. When the mapper closes, SDL
	// main will recreate its rendering pipeline.
	GFX_DisengageRendering();

	// Be sure that there is no update in progress
	GFX_EndUpdate( nullptr );
	mapper.window = GFX_GetWindow();
	if (mapper.window == nullptr) {
		E_Exit("MAPPER: Could not initialize video mode: %s",
		       SDL_GetError());
	}
	mapper.renderer = SDL_GetRenderer(mapper.window);
#if C_OPENGL
	SDL_GLContext context = nullptr;
	if (!mapper.renderer) {
		context = SDL_GL_GetCurrentContext();
		if (!context) {
			E_Exit("MAPPER: Failed to retrieve current OpenGL context: %s",
			       SDL_GetError());
		}

		const auto renderer_drivers_count = SDL_GetNumRenderDrivers();
		if (renderer_drivers_count <= 0) {
			E_Exit("MAPPER: Failed to retrieve available SDL renderer drivers: %s",
			       SDL_GetError());
		}
		int renderer_driver_index = -1;
		for (int i = 0; i < renderer_drivers_count; ++i) {
			SDL_RendererInfo renderer_info = {};
			if (SDL_GetRenderDriverInfo(i, &renderer_info) < 0) {
				E_Exit("MAPPER: Failed to retrieve SDL renderer driver info: %s",
				       SDL_GetError());
			}
			assert(renderer_info.name);
			if (strcmp(renderer_info.name, "opengl") == 0) {
				renderer_driver_index = i;
				break;
			}
		}
		if (renderer_driver_index == -1) {
			E_Exit("MAPPER: OpenGL support in SDL renderer is unavailable but required for OpenGL output");
		}
		constexpr uint32_t renderer_flags = 0;
		mapper.renderer = SDL_CreateRenderer(mapper.window,
		                                     renderer_driver_index,
		                                     renderer_flags);
	}
#endif
	if (mapper.renderer == nullptr) {
		E_Exit("MAPPER: Could not retrieve window renderer: %s",
		       SDL_GetError());
	}

	if (SDL_RenderSetLogicalSize(mapper.renderer, 640, 480) < 0) {
		LOG_WARNING("MAPPER: Failed to set renderer logical size: %s",
		            SDL_GetError());
	}

	// Create font atlas surface
	SDL_Surface* atlas_surface = SDL_CreateRGBSurfaceFrom(
	        int10_font_14, 8, 256 * 14, 1, 1, 0, 0, 0, 0);
	if (atlas_surface == nullptr) {
		E_Exit("MAPPER: Failed to create atlas surface: %s", SDL_GetError());
	}

	// Invert default surface palette
	const SDL_Color atlas_colors[2] = {{0x00, 0x00, 0x00, 0x00},
	                                   {0xff, 0xff, 0xff, 0xff}};
	if (SDL_SetPaletteColors(atlas_surface->format->palette, atlas_colors, 0, 2) <
	    0) {
		LOG_WARNING("MAPPER: Failed to set colors in font atlas: %s",
		            SDL_GetError());
	}

	// Convert surface to texture for accelerated SDL renderer
	mapper.font_atlas = SDL_CreateTextureFromSurface(mapper.renderer,
	                                                 atlas_surface);
	SDL_FreeSurface(atlas_surface);
	atlas_surface = nullptr;
	if (mapper.font_atlas == nullptr) {
		E_Exit("MAPPER: Failed to create font texture atlas: %s",
		       SDL_GetError());
	}

	if (last_clicked) {
		last_clicked->BindColor();
		last_clicked=nullptr;
	}
	/* Go in the event loop */
	mapper.exit = false;
	mapper.redraw=true;
	SetActiveEvent(nullptr);
#if defined (REDUCE_JOYSTICK_POLLING)
	SDL_JoystickEventState(SDL_ENABLE);
#endif
	while (!mapper.exit) {
		if (mapper.redraw) {
			mapper.redraw = false;
			DrawButtons();
		}
		BIND_MappingEvents();
		Delay(1);
	}
	/* ONE SHOULD NOT FORGET TO DO THIS!
	Unless a memory leak is desired... */
	SDL_DestroyTexture(mapper.font_atlas);
	SDL_RenderSetLogicalSize(mapper.renderer, 0, 0);
	SDL_SetRenderDrawColor(mapper.renderer,
	                       color_black.red,
	                       color_black.green,
	                       color_black.blue,
	                       SDL_ALPHA_OPAQUE);
#if C_OPENGL
	if (context) {
#if SDL_VERSION_ATLEAST(2, 0, 10)
		if (SDL_RenderFlush(mapper.renderer) < 0) {
			LOG_WARNING("MAPPER: Failed to flush pending renderer commands: %s",
			            SDL_GetError());
		}
#endif
		SDL_DestroyRenderer(mapper.renderer);
		if (SDL_GL_MakeCurrent(mapper.window, context) < 0) {
			LOG_ERR("MAPPER: Failed to restore OpenGL context: %s",
			        SDL_GetError());
		}
	}
#endif
#if defined (REDUCE_JOYSTICK_POLLING)
	SDL_JoystickEventState(SDL_DISABLE);
#endif
	GFX_ResetScreen();
	MOUSE_NotifyTakeOver(false);
}

static void MAPPER_Destroy(Section *sec) {
	(void) sec; // unused but present for API compliance

	// Stop any ongoing typing as soon as possible (because it access events)
	mapper.typist.Stop();

	// Release all the accumulated allocations by the mapper
	events.clear();

	for (auto & ptr : all_binds)
		delete ptr;
	all_binds.clear();

	buttons.clear();

	for (auto & ptr : keybindgroups)
		delete ptr;
	keybindgroups.clear();

	for (auto & ptr : stickbindgroups)
		delete ptr;
	stickbindgroups.clear();

	// Free any allocated sticks
	for (int i = 0; i < MAXSTICKS; ++i) {
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

void MAPPER_BindKeys(Section *sec)
{
	// Release any keys pressed, or else they'll get stuck
	GFX_LosingFocus();

	// Get the mapper file set by the user
	const auto section = static_cast<const Section_prop *>(sec);
	const auto mapperfile_value = section->Get_string("mapperfile");
	const auto property = section->Get_path("mapperfile");
	assert(property);
	mapper.filename = property->realpath.string();

	QueryJoysticks();

	// Create the graphical layout for all registered key-binds
	if (buttons.empty())
		CreateLayout();

	if (bindgroups.empty())
		CreateBindGroups();

	// Create binds from file or fallback to internals
	if (!load_binds_from_file(mapper.filename, mapperfile_value))
		CreateDefaultBinds();

	for (const auto& button : buttons) {
		button->BindColor();
	}

	if (SDL_GetModState()&KMOD_CAPS)
		MAPPER_TriggerEvent(caps_lock_event, false);

	if (SDL_GetModState()&KMOD_NUM)
		MAPPER_TriggerEvent(num_lock_event, false);

	GFX_RegenerateWindow(sec);
}

std::vector<std::string> MAPPER_GetEventNames(const std::string &prefix) {
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

void MAPPER_AutoType(std::vector<std::string> &sequence,
                     const uint32_t wait_ms,
                     const uint32_t pace_ms) {
	mapper.typist.Start(&events, sequence, wait_ms, pace_ms);
}

void MAPPER_AutoTypeStopImmediately()
{
	mapper.typist.StopImmediately();
}

void MAPPER_StartUp(Section* sec)
{
	assert(sec);
	Section_prop* section = static_cast<Section_prop*>(sec);

	// Runs after this function ends and for subsequent `config -set "sdl
	// mapperfile=file.map"` commands
	constexpr auto changeable_at_runtime = true;
	section->AddInitFunction(&MAPPER_BindKeys, changeable_at_runtime);

	// Runs one-time on shutdown
	section->AddDestroyFunction(&MAPPER_Destroy);
	MAPPER_AddHandler(&MAPPER_Run, SDL_SCANCODE_F1, PRIMARY_MOD, "mapper", "Mapper");
}
