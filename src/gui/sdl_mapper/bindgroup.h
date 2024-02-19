#ifndef SDL_MAPPER_CBINDGROUP_H
#define SDL_MAPPER_CBINDGROUP_H

class CBindGroup {
public:
	CBindGroup() {
		bindgroups.push_back(this);
	}
	virtual ~CBindGroup() = default;
	void ActivateBindList(CBindList * list,Bits value,bool ev_trigger);
	void DeactivateBindList(CBindList * list,bool ev_trigger);
	virtual CBind * CreateConfigBind(char *&buf)=0;
	virtual CBind * CreateEventBind(SDL_Event * event)=0;

	virtual bool CheckEvent(SDL_Event * event)=0;
	virtual const char * ConfigStart() = 0;
	virtual const char * BindStart() = 0;
};

class CKeyBindGroup final : public  CBindGroup {
public:
	CKeyBindGroup(Bitu _keys)
		: CBindGroup(),
		  lists(new CBindList[_keys]),
		  keys(_keys)
	{
		for (size_t i = 0; i < keys; i++)
			lists[i].clear();
	}

	~CKeyBindGroup() override
	{
		delete[] lists;
		lists = nullptr;
	}

	CKeyBindGroup(const CKeyBindGroup&) = delete; // prevent copy
	CKeyBindGroup& operator=(const CKeyBindGroup&) = delete; // prevent assignment

	CBind* CreateConfigBind(char *&buf) override;

	CBind* CreateEventBind(SDL_Event *event) override;

	bool CheckEvent(SDL_Event * event) override;

	CBind* CreateKeyBind(SDL_Scancode _key) {
		return new CKeyBind(&lists[(Bitu)_key],_key);
	}
private:
	const char * ConfigStart() override {
		return configname;
	}
	const char * BindStart() override {
		return "Key";
	}
protected:
	const char *configname = "key";
	CBindList *lists = nullptr;
	Bitu keys = 0;
};
static std::list<CKeyBindGroup *> keybindgroups;

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