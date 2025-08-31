// SPDX-FileCopyrightText: 2020-2025 The DOSBox Staging Team
// SPDX-FileCopyrightText: 2002-2021 The DOSBox Team
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DOSBOX_SECTION_H
#define DOSBOX_SECTION_H

#include <deque>
#include <string>
#include <cstdio>

class Section;

typedef void (*SectionFunction)(Section*);

class Section {
private:
	// Wrapper class around startup and shutdown functions. the variable
	// changeable_at_runtime indicates it can be called on configuration
	// changes
	struct Function_wrapper {
		SectionFunction function;
		bool changeable_at_runtime;

		Function_wrapper(const SectionFunction fn, bool ch)
		        : function(fn),
		          changeable_at_runtime(ch)
		{}
	};

	std::deque<Function_wrapper> init_functions   = {};
	std::deque<Function_wrapper> destroyfunctions = {};
	std::string sectionname                       = {};
	bool active                                   = true;

public:
	Section() = default;
	Section(const std::string& name, const bool active = true)
	        : sectionname(name),
	          active(active)
	{}

	// Construct and assign by std::move
	Section(Section&& other)            = default;
	Section& operator=(Section&& other) = default;

	// Children must call executedestroy!
	virtual ~Section() = default;

	void AddInitFunction(SectionFunction func, bool changeable_at_runtime = false);

	void AddDestroyFunction(SectionFunction func,
	                        bool changeable_at_runtime = false);

	void ExecuteInit(bool initall = true);
	void ExecuteDestroy(bool destroyall = true);

	bool IsActive() const
	{
		return active;
	}

	const char* GetName() const
	{
		return sectionname.c_str();
	}

	virtual std::string GetPropertyValue(const std::string& property) const = 0;

	virtual bool HandleInputline(const std::string& line) = 0;

	virtual void PrintData(FILE* outfile) const = 0;
};

#endif
