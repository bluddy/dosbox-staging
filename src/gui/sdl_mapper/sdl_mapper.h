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

static struct CMapper {
	SDL_Window *window = nullptr;
	SDL_Renderer* renderer  = nullptr;
	SDL_Texture* font_atlas = nullptr;
	bool exit = false;
	CEvent *aevent = nullptr;  // Active Event
	CBind *abind = nullptr;    // Active Bind
	CBindList_it abindit = {}; // Location of active bind in list
	bool redraw = false;
	bool addbind = false;
	Bitu mods = 0;
	struct {
		CStickBindGroup *stick[MAXSTICKS] = {nullptr};
		unsigned int num = 0;
		unsigned int num_groups = 0;
	} sticks = {};
	Typer typist = {};
	std::string filename = "";
} mapper;


#endif