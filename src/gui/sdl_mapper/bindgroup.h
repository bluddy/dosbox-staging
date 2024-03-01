#ifndef SDL_MAPPER_CBINDGROUP_H
#define SDL_MAPPER_CBINDGROUP_H

#include <list>
#include <memory>

#include "bind.h"

typedef std::list<std::shared_ptr<CBind>> CBindList;

class Mapper;

// These are actual physical devices
class CBindGroup {
public:
	CBindGroup(Mapper &_mapper) : mapper(_mapper)
	{
		// bindgroups.push_back(this); // Must always add to bindgroups
	}
	virtual ~CBindGroup() = default;
	void ActivateBindList(CBindList& list, Bits const value, bool const ev_trigger);
	void DeactivateBindList(CBindList& list, bool const ev_trigger);
	// Create a bind based on a configuration string
	virtual std::shared_ptr<CBind> CreateConfigBind(std::string const &buf) const = 0;
	// Create a bind based on an SDL event
	virtual std::shared_ptr<CBind> CreateEventBind(SDL_Event const &event) const = 0;

	virtual bool CheckEvent(SDL_Event const &event) const = 0;
	virtual std::string const &ConfigStart() = 0;
	virtual std::string const BindStart() = 0;
protected:
	Mapper &mapper; // Reference to sdl_mapper
};

class CKeyBindGroup final : public CBindGroup {
public:
	CKeyBindGroup(Bitu _num_keys)
		: CBindGroup(),
		  key_bind_lists(_num_keys)
	{}

	CKeyBindGroup(const CKeyBindGroup&) = delete; // prevent copy
	CKeyBindGroup& operator=(const CKeyBindGroup&) = delete; // prevent assignment

	std::shared_ptr<CBind> CreateConfigBind(std::string const &str) override;
	std::shared_ptr<CBind> CreateEventBind(SDL_Event const &event) override;
	// Create a bind based on an SDL scancode
	std::shared_ptr<CBind> CreateKeyBind(SDL_Scancode const _key) const;

	bool CheckEvent(SDL_Event const &event) override;


private:
	std::string const &ConfigStart() override {
		return configname;
	}
	std::string const BindStart() override {
		return "Key";
	}
protected:
	std::string const configname{"key"};
	// vectors of Lists of bindings
	std::vector<std::list<std::shared_ptr<CBind>>> key_bind_lists;
};

class CStickBindGroup : public CBindGroup {
public:
	CStickBindGroup(int _stick_index, uint8_t _emustick, bool _dummy = false);

	~CStickBindGroup() override;

	CStickBindGroup(const CStickBindGroup&) = delete; // prevent copy
	CStickBindGroup& operator=(const CStickBindGroup&) = delete; // prevent assignment

	CBind * CreateConfigBind(char *& buf) override;
	CBind * CreateEventBind(SDL_Event * event) override;
	bool CheckEvent(SDL_Event * event) override;
	virtual void UpdateJoystick();

	void ActivateJoystickBoundEvents();

private:
	CBind * CreateAxisBind(int axis, bool positive);
	CBind * CreateButtonBind(int button);
	CBind *CreateHatBind(uint8_t hat, uint8_t value);
	const char * ConfigStart() override
	{
		return configname;
	}

	const char * BindStart() override
	{
		if (sdl_joystick)
			return SDL_JoystickNameForIndex(stick_index);
		else
			return "[missing joystick]";
	}

protected:
	CBindList *pos_axis_lists = nullptr;
	CBindList *neg_axis_lists = nullptr;
	CBindList *button_lists = nullptr;
	CBindList *hat_lists = nullptr;
	int axes = 0;
	int emulated_axes = 0;
	int buttons = 0;
	int button_cap = 0;
	int button_wrap = 0;
	uint8_t emulated_buttons = 0;
	int hats = 0;
	int emulated_hats = 0;
    // Instance ID of the joystick as it appears in SDL events
	int stick_id{-1};
	// Index of the joystick in the system
	int stick_index{-1};
	uint8_t emustick;
	SDL_Joystick *sdl_joystick = nullptr;
	char configname[10];
	unsigned button_autofire[MAXBUTTON] = {};
	bool old_button_state[MAXBUTTON] = {};
	bool old_pos_axis_state[MAXAXIS] = {};
	bool old_neg_axis_state[MAXAXIS] = {};
	uint8_t old_hat_state[MAXHAT] = {};
	bool is_dummy;
};

class C4AxisBindGroup final : public  CStickBindGroup {
public:
	C4AxisBindGroup(uint8_t _stick, uint8_t _emustick) : CStickBindGroup(_stick, _emustick);

	bool CheckEvent(SDL_Event * event) override;
	void UpdateJoystick() override;
};

class CFCSBindGroup final : public  CStickBindGroup {
public:
	CFCSBindGroup(uint8_t _stick, uint8_t _emustick) : CStickBindGroup(_stick, _emustick);

	bool CheckEvent(SDL_Event * event) override;

	void UpdateJoystick() override;

private:
	uint8_t old_hat_position = 0;
	void DecodeHatPosition(Uint8 hat_pos);
};

class CCHBindGroup final : public CStickBindGroup {
public:
	CCHBindGroup(uint8_t _stick, uint8_t _emustick);

	bool CheckEvent(SDL_Event * event) override;

	void UpdateJoystick() override;

protected:
	uint16_t button_state = 0;
};

#endif