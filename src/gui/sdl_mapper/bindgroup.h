#ifndef SDL_MAPPER_CBINDGROUP_H
#define SDL_MAPPER_CBINDGROUP_H

#include <list>
#include <memory>
#include <array>

#include "bind.h"
#include "sdl_mapper.h"

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
	// Create a bind based on an SDL event that is fired by the user
	virtual std::shared_ptr<CBind> CreateEventBind(SDL_Event const &event) const = 0;

	virtual bool CheckEvent(SDL_Event const &event) const = 0;
	virtual std::string const &ConfigStart() const = 0;
	virtual std::string const BindStart() const = 0;
protected:
	Mapper &mapper; // Reference to sdl_mapper
};

class CKeyBindGroup final : public CBindGroup {
public:
	CKeyBindGroup(Mapper &_mapper, Bitu _num_keys)
		: CBindGroup(_mapper),
		  key_bind_lists(_num_keys)
	{}

	CKeyBindGroup(const CKeyBindGroup&) = delete; // prevent copy
	CKeyBindGroup& operator=(const CKeyBindGroup&) = delete; // prevent assignment

	std::shared_ptr<CBind> CreateConfigBind(std::string const &str) const override;
	std::shared_ptr<CBind> CreateEventBind(SDL_Event const &event) const override;
	// Create a bind based on an SDL scancode
	std::shared_ptr<CBind> CreateKeyBind(SDL_Scancode const _key) const;

	bool CheckEvent(SDL_Event const &event) const override;


private:
	std::string const &ConfigStart() const override {
		return configname;
	}
	std::string const BindStart() const override {
		return "Key";
	}
protected:
	std::string const configname{"key"};
	// vectors of Lists of bindings
	std::vector<std::list<std::shared_ptr<CBind>>> key_bind_lists;
};

class CStickBindGroup : public CBindGroup {
public:
	CStickBindGroup(Mapper &_mapper, int const _stick_index, uint8_t const _emustick, bool const _dummy = false);

	~CStickBindGroup() override;

	CStickBindGroup(const CStickBindGroup&) = delete; // prevent copy
	CStickBindGroup& operator=(const CStickBindGroup&) = delete; // prevent assignment

	std::shared_ptr<CBind> CreateConfigBind(std::string const &buf) const override;
	std::shared_ptr<CBind> CreateEventBind(SDL_Event const &event) const override;
	bool CheckEvent(SDL_Event const &event) const override;
	virtual void UpdateJoystick();

	void ActivateJoystickBoundEvents();

private:
	std::shared_ptr<CBind> CreateAxisBind(int axis, bool positive) const;
	std::shared_ptr<CBind> CreateButtonBind(int button) const;
	std::shared_ptr<CBind> CreateHatBind(uint8_t hat, uint8_t value) const;
	std::string const &ConfigStart()  const override
	{
		return configname;
	}

	std::string const BindStart()  const override
	{
		if (sdl_joystick)
			return SDL_JoystickNameForIndex(stick_index);
		else
			return "[missing joystick]";
	}

protected:
	std::array<CBindList, max_axis> pos_axis_lists;
	std::array<CBindList, max_axis> neg_axis_lists;
	std::array<CBindList, max_button> button_lists;
	std::array<CBindList, 4> hat_lists;

	int emulated_axes{2};
	int emulated_hats{0};
	uint8_t emulated_buttons{0};

	int num_axes{0};
	int num_buttons{0};
	int num_hats{0};

	int button_cap{0};
	// The number to wrap our joystick by (buttons begin to overlap after this)
	int num_button_wrap{0};

    // Instance ID of the joystick as it appears in SDL events
	int stick_id{-1};
	// Index of the joystick in the system
	int stick_index{-1};
	uint8_t emustick;
	SDL_Joystick *sdl_joystick = nullptr;

	// Name of joystick
	std::string const configname;
	// Counter used for autofire: will increment and send signal on odd numbers
	std::array<unsigned int, max_button> button_autofire;
	std::array<bool, max_button> old_button_state;
	std::array<bool, max_button> old_pos_axis_state;
	std::array<bool, max_axis> old_neg_axis_state;
	std::array<uint8_t, max_hat> old_hat_state;
	// Is this a real device
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