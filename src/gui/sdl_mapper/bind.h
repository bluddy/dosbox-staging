#ifndef DOSBOX_MAPPER_BIND
#define DOSBOX_MAPPER_BIND

#include <list>
#include <vector>
#include <string>

typedef std::list<CBind *> CBindList;
typedef std::list<CBind *>::iterator CBindList_it;
typedef std::vector<CBindGroup *>::iterator CBindGroup_it;

class CBind {
public:
	CBind(CBindList *binds);

	virtual ~CBind()
	{
		if (list)
			list->remove(this);
	}

	CBind(const CBind&) = delete; // prevent copy
	CBind& operator=(const CBind&) = delete; // prevent assignment

	void AddFlags(char * buf);
	void SetFlags(char *buf);

	void ActivateBind(Bits _value,bool ev_trigger,bool skip_action=false);			/* use value-boundary for on/off events */
	void DeActivateBind(bool ev_trigger);
	virtual void ConfigName(char * buf) = 0;

	virtual std::string GetBindName() const = 0;

	Bitu mods = 0;
	Bitu flags = 0;
	CEvent *event = nullptr;
	CBindList *list = nullptr;
	bool active = false;
	bool holding = false;
};

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

class CKeyBind;
class CKeyBindGroup;

class CKeyBind final : public CBind {
public:
	CKeyBind(CBindList *_list, SDL_Scancode _key)
		: CBind(_list),
		  key(_key)
	{}

	std::string GetBindName() const override;

	void ConfigName(char *buf) override
	{
		sprintf(buf, "key %d", key);
	}

public:
	SDL_Scancode key;
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

class CJAxisBind;
class CJButtonBind;
class CJHatBind;

class CJAxisBind final : public CBind {
public:
	CJAxisBind(CBindList *_list, CBindGroup *_group, int _axis, bool _positive)
		: CBind(_list),
		  group(_group),
		  axis(_axis),
		  positive(_positive)
	{}

	CJAxisBind(const CJAxisBind&) = delete; // prevent copy
	CJAxisBind& operator=(const CJAxisBind&) = delete; // prevent assignment

	void ConfigName(char *buf) override
	{
		sprintf(buf, "%s axis %d %d",
		        group->ConfigStart(),
		        axis,
		        positive ? 1 : 0);
	}

	std::string GetBindName() const override
	{
		char buf[30];
		safe_sprintf(buf, "%s Axis %d%s", group->BindStart(), axis,
		             positive ? "+" : "-");
		return buf;
	}

protected:
	CBindGroup *group;
	int axis;
	bool positive;
};

class CJButtonBind final : public CBind {
public:
	CJButtonBind(CBindList *_list, CBindGroup *_group, int _button)
		: CBind(_list),
		  group(_group),
		  button(_button)
	{}

	CJButtonBind(const CJButtonBind&) = delete; // prevent copy
	CJButtonBind& operator=(const CJButtonBind&) = delete; // prevent assignment

	void ConfigName(char *buf) override
	{
		sprintf(buf, "%s button %d", group->ConfigStart(), button);
	}

	std::string GetBindName() const override
	{
		char buf[30];
		safe_sprintf(buf, "%s Button %d", group->BindStart(), button);
		return buf;
	}

protected:
	CBindGroup *group;
	int button;
};

class CJHatBind final : public CBind {
public:
	CJHatBind(CBindList *_list, CBindGroup *_group, uint8_t _hat, uint8_t _dir);

	CJHatBind(const CJHatBind&) = delete; // prevent copy
	CJHatBind& operator=(const CJHatBind&) = delete; // prevent assignment

	void ConfigName(char *buf) override;
	std::string GetBindName() const override;

protected:
	CBindGroup *group;
	uint8_t hat;
	uint8_t dir;
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