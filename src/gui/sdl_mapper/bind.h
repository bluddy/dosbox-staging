#ifndef DOSBOX_MAPPER_BIND
#define DOSBOX_MAPPER_BIND

#include <list>
#include <vector>

typedef std::list<CBind *> CBindList;
typedef std::list<CBind *>::iterator CBindList_it;
typedef std::vector<CBindGroup *>::iterator CBindGroup_it;

class CBind {
public:
	CBind(CBindList *binds)
		: list(binds)
	{
		list->push_back(this);
		event=nullptr;
		all_binds.push_back(this);
	}

	virtual ~CBind()
	{
		if (list)
			list->remove(this);
	}

	CBind(const CBind&) = delete; // prevent copy
	CBind& operator=(const CBind&) = delete; // prevent assignment

	void AddFlags(char * buf) {
		if (mods & BMOD_Mod1) strcat(buf," mod1");
		if (mods & BMOD_Mod2) strcat(buf," mod2");
		if (mods & BMOD_Mod3) strcat(buf," mod3");
		if (flags & BFLG_Hold) strcat(buf," hold");
	}

	void SetFlags(char *buf)
	{
		char *word;
		while (*(word = strip_word(buf))) {
			if (!strcasecmp(word, "mod1"))
				mods |= BMOD_Mod1;
			if (!strcasecmp(word, "mod2"))
				mods |= BMOD_Mod2;
			if (!strcasecmp(word, "mod3"))
				mods |= BMOD_Mod3;
			if (!strcasecmp(word, "hold"))
				flags |= BFLG_Hold;
		}
	}

	void ActivateBind(Bits _value,bool ev_trigger,bool skip_action=false) {
		if (event->IsTrigger()) {
			/* use value-boundary for on/off events */
			if (_value>25000) {
				event->SetValue(_value);
				if (active) return;
				event->ActivateEvent(ev_trigger,skip_action);
				active=true;
			} else {
				if (active) {
					event->DeActivateEvent(ev_trigger);
					active=false;
				}
			}
		} else {
			/* store value for possible later use in the activated event */
			event->SetValue(_value);
			event->ActivateEvent(ev_trigger,false);
		}
	}
	void DeActivateBind(bool ev_trigger) {
		if (event->IsTrigger()) {
			if (!active) return;
			active=false;
			if (flags & BFLG_Hold) {
				if (!holding) {
					holdlist.push_back(this);
					holding=true;
					return;
				} else {
					holdlist.remove(this);
					holding=false;
				}
			}
			event->DeActivateEvent(ev_trigger);
		} else {
			/* store value for possible later use in the activated event */
			event->SetValue(0);
			event->DeActivateEvent(ev_trigger);
		}
	}
	virtual void ConfigName(char * buf)=0;

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

protected:

};

class CKeyBind;
class CKeyBindGroup;

class CKeyBind final : public CBind {
public:
	CKeyBind(CBindList *_list, SDL_Scancode _key)
		: CBind(_list),
		  key(_key)
	{}

	std::string GetBindName() const override
	{
		// Always map Return to Enter
		if (key == SDL_SCANCODE_RETURN) {
			return "Enter";
		}

		const std::string sdl_scancode_name = SDL_GetScancodeName(key);
		if (!sdl_scancode_name.empty()) {
			return sdl_scancode_name;
		}

		// SDL Doesn't have a name for this key, so use our own
		assert(sdl_scancode_name.empty());

		// Key between Left Shift and Z is "oem102"
		if (key == SDL_SCANCODE_NONUSBACKSLASH) {
			return "oem102"; // called 'OEM_102" at kbdlayout.info
		}
		// Key to the left of Right Shift on ABNT layouts
		if (key == SDL_SCANCODE_INTERNATIONAL1) {
			return "abnt1"; // called "ABNT_C1" at kbdlayout.info
		}

		LOG_DEBUG("MAPPER: Please report unnamed SDL scancode %d (%xh)",
		          key,
		          key);

		return sdl_scancode_name;
	}

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

	CBind *CreateConfigBind(char *&buf) override
	{
		if (strncasecmp(buf, configname, strlen(configname)))
			return nullptr;
		strip_word(buf);
		long code = atol(strip_word(buf));
		assert(code > 0);
		return CreateKeyBind((SDL_Scancode)code);
	}

	CBind *CreateEventBind(SDL_Event *event) override
	{
		if (event->type != SDL_KEYDOWN)
			return nullptr;
		return CreateKeyBind(event->key.keysym.scancode);
	}

	bool CheckEvent(SDL_Event * event) override {
		if (event->type!=SDL_KEYDOWN && event->type!=SDL_KEYUP) return false;
		uintptr_t key = static_cast<uintptr_t>(event->key.keysym.scancode);
		if (event->type==SDL_KEYDOWN) ActivateBindList(&lists[key],0x7fff,true);
		else DeactivateBindList(&lists[key],true);
		return 0;
	}
	CBind * CreateKeyBind(SDL_Scancode _key) {
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
	CJHatBind(CBindList *_list, CBindGroup *_group, uint8_t _hat, uint8_t _dir)
		: CBind(_list),
		  group(_group),
		  hat(_hat),
		  dir(_dir)
	{
		// TODO this code allows to bind only a single hat position, but
		// perhaps we should allow 8-way positioning?
		if (dir & SDL_HAT_UP)
			dir = SDL_HAT_UP;
		else if (dir & SDL_HAT_RIGHT)
			dir=SDL_HAT_RIGHT;
		else if (dir & SDL_HAT_DOWN)
			dir=SDL_HAT_DOWN;
		else if (dir & SDL_HAT_LEFT)
			dir=SDL_HAT_LEFT;
		else
			E_Exit("MAPPER:JOYSTICK:Invalid hat position");
	}

	CJHatBind(const CJHatBind&) = delete; // prevent copy
	CJHatBind& operator=(const CJHatBind&) = delete; // prevent assignment

	void ConfigName(char *buf) override
	{
		sprintf(buf,"%s hat %" PRIu8 " %" PRIu8,
		        group->ConfigStart(), hat, dir);
	}

	std::string GetBindName() const override
	{
		char buf[30];
		safe_sprintf(buf, "%s Hat %" PRIu8 " %s", group->BindStart(), hat,
		             ((dir == SDL_HAT_UP)      ? "up"
		              : (dir == SDL_HAT_RIGHT) ? "right"
		              : (dir == SDL_HAT_DOWN)  ? "down"
		                                       : "left"));
		return buf;
	}

protected:
	CBindGroup *group;
	uint8_t hat;
	uint8_t dir;
};

class CStickBindGroup : public CBindGroup {
public:
	CStickBindGroup(int _stick_index, uint8_t _emustick, bool _dummy = false)
	        : CBindGroup(),
	          stick_index(_stick_index), // the number of the device in the system
	          emustick(_emustick), // the number of the emulated device
	          is_dummy(_dummy)
	{
		sprintf(configname, "stick_%u", static_cast<unsigned>(emustick));
		if (is_dummy)
			return;

		// initialize binding lists and position data
		pos_axis_lists = new CBindList[MAXAXIS];
		neg_axis_lists = new CBindList[MAXAXIS];
		button_lists = new CBindList[MAXBUTTON];
		hat_lists = new CBindList[4];

		// initialize emulated joystick state
		emulated_axes=2;
		emulated_buttons=2;
		emulated_hats=0;
		JOYSTICK_Enable(emustick,true);

		// From the SDL doco
		// (https://wiki.libsdl.org/SDL2/SDL_JoystickOpen):

		// The device_index argument refers to the N'th joystick presently
		// recognized by SDL on the system. It is NOT the same as the
		// instance ID used to identify the joystick in future events.

		// Also see: https://wiki.libsdl.org/SDL2/SDL_JoystickInstanceID

		// We refer to the device index as `stick_index`, and to the
		// instance ID as `stick_id`.

		sdl_joystick = SDL_JoystickOpen(stick_index);
		stick_id = SDL_JoystickInstanceID(sdl_joystick);

		set_joystick_led(sdl_joystick, on_color);
		if (sdl_joystick==nullptr) {
			button_wrap=emulated_buttons;
			axes=MAXAXIS;
			return;
		}

		const int sdl_axes = SDL_JoystickNumAxes(sdl_joystick);
		if (sdl_axes < 0)
			LOG_MSG("SDL: Can't detect axes; %s", SDL_GetError());
		axes = clamp(sdl_axes, 0, MAXAXIS);

		const int sdl_hats = SDL_JoystickNumHats(sdl_joystick);
		if (sdl_hats < 0)
			LOG_MSG("SDL: Can't detect hats; %s", SDL_GetError());
		hats = clamp(sdl_hats, 0, MAXHAT);

		buttons = SDL_JoystickNumButtons(sdl_joystick); // TODO returns -1 on error
		button_wrap = buttons;
		button_cap = buttons;
		if (button_wrapping_enabled) {
			button_wrap=emulated_buttons;
			if (buttons>MAXBUTTON_CAP) button_cap = MAXBUTTON_CAP;
		}
		if (button_wrap > MAXBUTTON)
			button_wrap = MAXBUTTON;

		LOG_MSG("MAPPER: Initialised %s with %d axes, %d buttons, and %d hat(s)",
		        SDL_JoystickNameForIndex(stick_index), axes, buttons, hats);
	}

	~CStickBindGroup() override
	{
		set_joystick_led(sdl_joystick, off_color);
		SDL_JoystickClose(sdl_joystick);
		sdl_joystick = nullptr;

		delete[] pos_axis_lists;
		pos_axis_lists = nullptr;

		delete[] neg_axis_lists;
		neg_axis_lists = nullptr;

		delete[] button_lists;
		button_lists = nullptr;

		delete[] hat_lists;
		hat_lists = nullptr;
	}

	CStickBindGroup(const CStickBindGroup&) = delete; // prevent copy
	CStickBindGroup& operator=(const CStickBindGroup&) = delete; // prevent assignment

	CBind * CreateConfigBind(char *& buf) override
	{
		if (strncasecmp(configname,buf,strlen(configname))) return nullptr;
		strip_word(buf);
		char *type = strip_word(buf);
		CBind *bind = nullptr;
		if (!strcasecmp(type,"axis")) {
			int ax = atoi(strip_word(buf));
			int pos = atoi(strip_word(buf));
			bind = CreateAxisBind(ax, pos > 0); // TODO double check, previously it was != 0
		} else if (!strcasecmp(type, "button")) {
			int but = atoi(strip_word(buf));
			bind = CreateButtonBind(but);
		} else if (!strcasecmp(type, "hat")) {
			uint8_t hat = static_cast<uint8_t>(atoi(strip_word(buf)));
			uint8_t dir = static_cast<uint8_t>(atoi(strip_word(buf)));
			bind = CreateHatBind(hat, dir);
		}
		return bind;
	}

	CBind * CreateEventBind(SDL_Event * event) override {
		if (event->type==SDL_JOYAXISMOTION) {
			const int axis_id = event->jaxis.axis;
			const auto axis_position = event->jaxis.value;

			if (event->jaxis.which != stick_id)
				return nullptr;
#if defined(REDUCE_JOYSTICK_POLLING)
			if (axis_id >= axes)
				return nullptr;
#endif
			if (abs(axis_position) < 25000)
				return nullptr;

			// Axis IDs 2 and 5 are triggers on six-axis controllers
			const bool is_trigger = (axis_id == 2 || axis_id == 5) && axes == 6;
			const bool toggled = axis_position > 0 || is_trigger;
			return CreateAxisBind(axis_id, toggled);

		} else if (event->type == SDL_JOYBUTTONDOWN) {
			if (event->jbutton.which != stick_id)
				return nullptr;
#if defined (REDUCE_JOYSTICK_POLLING)
			return CreateButtonBind(event->jbutton.button%button_wrap);
#else
			return CreateButtonBind(event->jbutton.button);
#endif
		} else if (event->type==SDL_JOYHATMOTION) {
			if (event->jhat.which != stick_id) return nullptr;
			if (event->jhat.value == 0) return nullptr;
			if (event->jhat.value>(SDL_HAT_UP|SDL_HAT_RIGHT|SDL_HAT_DOWN|SDL_HAT_LEFT)) return nullptr;
			return CreateHatBind(event->jhat.hat, event->jhat.value);
		} else return nullptr;
	}

	bool CheckEvent(SDL_Event * event) override {
		SDL_JoyAxisEvent * jaxis = nullptr;
		SDL_JoyButtonEvent *jbutton = nullptr;

		switch(event->type) {
			case SDL_JOYAXISMOTION:
				jaxis = &event->jaxis;
				if(jaxis->which == stick_id) {
					if(jaxis->axis == 0)
						JOYSTICK_Move_X(emustick, jaxis->value);
					else if (jaxis->axis == 1)
						JOYSTICK_Move_Y(emustick, jaxis->value);
				}
				break;
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				jbutton = &event->jbutton;
				bool state;
				state = jbutton->type == SDL_JOYBUTTONDOWN;
				const auto but = check_cast<uint8_t>(jbutton->button % emulated_buttons);
				if (jbutton->which == stick_id) {
					JOYSTICK_Button(emustick, but, state);
				}
				break;
			}
			return false;
	}

	virtual void UpdateJoystick() {
		if (is_dummy) return;
		/* query SDL joystick and activate bindings */
		ActivateJoystickBoundEvents();

		bool button_pressed[MAXBUTTON];
		std::fill_n(button_pressed, MAXBUTTON, false);
		for (int i = 0; i < MAX_VJOY_BUTTONS; i++) {
			if (virtual_joysticks[emustick].button_pressed[i])
				button_pressed[i % button_wrap]=true;
		}
		for (uint8_t i = 0; i < emulated_buttons; i++) {
			if (autofire && (button_pressed[i]))
				JOYSTICK_Button(emustick,i,(++button_autofire[i])&1);
			else
				JOYSTICK_Button(emustick,i,button_pressed[i]);
		}

		JOYSTICK_Move_X(emustick, virtual_joysticks[emustick].axis_pos[0]);
		JOYSTICK_Move_Y(emustick, virtual_joysticks[emustick].axis_pos[1]);
	}

	void ActivateJoystickBoundEvents() {
		if (GCC_UNLIKELY(sdl_joystick==nullptr)) return;

		bool button_pressed[MAXBUTTON];
		std::fill_n(button_pressed, MAXBUTTON, false);
		/* read button states */
		for (int i = 0; i < button_cap; i++) {
			if (SDL_JoystickGetButton(sdl_joystick, i))
				button_pressed[i % button_wrap]=true;
		}
		for (int i = 0; i < button_wrap; i++) {
			/* activate binding if button state has changed */
			if (button_pressed[i]!=old_button_state[i]) {
				if (button_pressed[i]) ActivateBindList(&button_lists[i],32767,true);
				else DeactivateBindList(&button_lists[i],true);
				old_button_state[i]=button_pressed[i];
			}
		}
		for (int i = 0; i < axes; i++) {
			Sint16 caxis_pos = SDL_JoystickGetAxis(sdl_joystick, i);
			/* activate bindings for joystick position */
			if (caxis_pos>1) {
				if (old_neg_axis_state[i]) {
					DeactivateBindList(&neg_axis_lists[i],false);
					old_neg_axis_state[i] = false;
				}
				ActivateBindList(&pos_axis_lists[i],caxis_pos,false);
				old_pos_axis_state[i] = true;
			} else if (caxis_pos<-1) {
				if (old_pos_axis_state[i]) {
					DeactivateBindList(&pos_axis_lists[i],false);
					old_pos_axis_state[i] = false;
				}
				if (caxis_pos!=-32768) caxis_pos=(Sint16)abs(caxis_pos);
				else caxis_pos=32767;
				ActivateBindList(&neg_axis_lists[i],caxis_pos,false);
				old_neg_axis_state[i] = true;
			} else {
				/* center */
				if (old_pos_axis_state[i]) {
					DeactivateBindList(&pos_axis_lists[i],false);
					old_pos_axis_state[i] = false;
				}
				if (old_neg_axis_state[i]) {
					DeactivateBindList(&neg_axis_lists[i],false);
					old_neg_axis_state[i] = false;
				}
			}
		}
		for (int i = 0; i < hats; i++) {
			assert(i < MAXHAT);
			const uint8_t chat_state = SDL_JoystickGetHat(sdl_joystick, i);

			/* activate binding if hat state has changed */
			if ((chat_state & SDL_HAT_UP) != (old_hat_state[i] & SDL_HAT_UP)) {
				if (chat_state & SDL_HAT_UP) ActivateBindList(&hat_lists[(i<<2)+0],32767,true);
				else DeactivateBindList(&hat_lists[(i<<2)+0],true);
			}
			if ((chat_state & SDL_HAT_RIGHT) != (old_hat_state[i] & SDL_HAT_RIGHT)) {
				if (chat_state & SDL_HAT_RIGHT) ActivateBindList(&hat_lists[(i<<2)+1],32767,true);
				else DeactivateBindList(&hat_lists[(i<<2)+1],true);
			}
			if ((chat_state & SDL_HAT_DOWN) != (old_hat_state[i] & SDL_HAT_DOWN)) {
				if (chat_state & SDL_HAT_DOWN) ActivateBindList(&hat_lists[(i<<2)+2],32767,true);
				else DeactivateBindList(&hat_lists[(i<<2)+2],true);
			}
			if ((chat_state & SDL_HAT_LEFT) != (old_hat_state[i] & SDL_HAT_LEFT)) {
				if (chat_state & SDL_HAT_LEFT) ActivateBindList(&hat_lists[(i<<2)+3],32767,true);
				else DeactivateBindList(&hat_lists[(i<<2)+3],true);
			}
			old_hat_state[i] = chat_state;
		}
	}

private:
	CBind * CreateAxisBind(int axis, bool positive)
	{
		if (axis < 0 || axis >= axes)
			return nullptr;
		if (positive)
			return new CJAxisBind(&pos_axis_lists[axis],
			                      this, axis, positive);
		else
			return new CJAxisBind(&neg_axis_lists[axis],
			                      this, axis, positive);
	}

	CBind * CreateButtonBind(int button)
	{
		if (button < 0 || button >= button_wrap)
			return nullptr;
		return new CJButtonBind(&button_lists[button],
		                        this,
		                        button);
	}

	CBind *CreateHatBind(uint8_t hat, uint8_t value)
	{
		if (is_dummy)
			return nullptr;
		assert(hat_lists);

		uint8_t hat_dir;
		if (value & SDL_HAT_UP)
			hat_dir = 0;
		else if (value & SDL_HAT_RIGHT)
			hat_dir = 1;
		else if (value & SDL_HAT_DOWN)
			hat_dir = 2;
		else if (value & SDL_HAT_LEFT)
			hat_dir = 3;
		else
			return nullptr;
		return new CJHatBind(&hat_lists[(hat << 2) + hat_dir],
		                     this, hat, value);
	}

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
	C4AxisBindGroup(uint8_t _stick, uint8_t _emustick) : CStickBindGroup(_stick, _emustick)
	{
		emulated_axes = 4;
		emulated_buttons = 4;
		if (button_wrapping_enabled)
			button_wrap = emulated_buttons;
		JOYSTICK_Enable(1, true);
	}

	bool CheckEvent(SDL_Event * event) override {
		SDL_JoyAxisEvent * jaxis = nullptr;
		SDL_JoyButtonEvent *jbutton = nullptr;

		switch(event->type) {
			case SDL_JOYAXISMOTION:
				jaxis = &event->jaxis;
				if(jaxis->which == stick_id && jaxis->axis < 4) {
					if(jaxis->axis & 1)
						JOYSTICK_Move_Y(jaxis->axis >> 1 & 1, jaxis->value);
					else
						JOYSTICK_Move_X(jaxis->axis >> 1 & 1, jaxis->value);
		        }
		        break;
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				jbutton = &event->jbutton;
				bool state;
				state = jbutton->type == SDL_JOYBUTTONDOWN;
				const auto but = check_cast<uint8_t>(jbutton->button % emulated_buttons);
				if (jbutton->which == stick_id) {
					JOYSTICK_Button((but >> 1), (but & 1), state);
				}
				break;
		}
		return false;
	}

	void UpdateJoystick() override {
		/* query SDL joystick and activate bindings */
		ActivateJoystickBoundEvents();

		bool button_pressed[MAXBUTTON];
		std::fill_n(button_pressed, MAXBUTTON, false);
		for (int i = 0; i < MAX_VJOY_BUTTONS; i++) {
			if (virtual_joysticks[0].button_pressed[i])
				button_pressed[i % button_wrap]=true;
		}
		for (uint8_t i = 0; i < emulated_buttons; ++i) {
			if (autofire && (button_pressed[i]))
				JOYSTICK_Button(i>>1,i&1,(++button_autofire[i])&1);
			else
				JOYSTICK_Button(i>>1,i&1,button_pressed[i]);
		}

		JOYSTICK_Move_X(0, virtual_joysticks[0].axis_pos[0]);
		JOYSTICK_Move_Y(0, virtual_joysticks[0].axis_pos[1]);
		JOYSTICK_Move_X(1, virtual_joysticks[0].axis_pos[2]);
		JOYSTICK_Move_Y(1, virtual_joysticks[0].axis_pos[3]);
	}
};

class CFCSBindGroup final : public  CStickBindGroup {
public:
	CFCSBindGroup(uint8_t _stick, uint8_t _emustick) : CStickBindGroup(_stick, _emustick)
	{
		emulated_axes=4;
		emulated_buttons=4;
		emulated_hats=1;
		if (button_wrapping_enabled) button_wrap=emulated_buttons;
		JOYSTICK_Enable(1,true);
		JOYSTICK_Move_Y(1, INT16_MAX);
	}

	bool CheckEvent(SDL_Event * event) override {
		SDL_JoyAxisEvent * jaxis = nullptr;
		SDL_JoyButtonEvent * jbutton = nullptr;
		SDL_JoyHatEvent *jhat = nullptr;

		switch(event->type) {
			case SDL_JOYAXISMOTION:
				jaxis = &event->jaxis;
				if(jaxis->which == stick_id) {
					if(jaxis->axis == 0)
						JOYSTICK_Move_X(0, jaxis->value);
					else if (jaxis->axis == 1)
						JOYSTICK_Move_Y(0, jaxis->value);
					else if (jaxis->axis == 2)
						JOYSTICK_Move_X(1, jaxis->value);
				}
				break;
			case SDL_JOYHATMOTION:
				jhat = &event->jhat;
				if (jhat->which == stick_id)
					DecodeHatPosition(jhat->value);
				break;
			case SDL_JOYBUTTONDOWN:
			case SDL_JOYBUTTONUP:
				jbutton = &event->jbutton;
			bool state;
			state=jbutton->type==SDL_JOYBUTTONDOWN;
				const auto but = check_cast<uint8_t>(jbutton->button % emulated_buttons);
				if (jbutton->which == stick_id) {
					JOYSTICK_Button((but >> 1), (but & 1), state);
				}
				break;
		}
		return false;
	}

	void UpdateJoystick() override {
		/* query SDL joystick and activate bindings */
		ActivateJoystickBoundEvents();

		bool button_pressed[MAXBUTTON];
		for (uint8_t i = 0; i < MAXBUTTON; i++)
			button_pressed[i] = false;
		for (uint8_t i = 0; i < MAX_VJOY_BUTTONS; i++) {
			if (virtual_joysticks[0].button_pressed[i])
				button_pressed[i % button_wrap]=true;
		}
		for (uint8_t i = 0; i < emulated_buttons; i++) {
			if (autofire && (button_pressed[i]))
				JOYSTICK_Button(i>>1,i&1,(++button_autofire[i])&1);
			else
				JOYSTICK_Button(i>>1,i&1,button_pressed[i]);
		}

		JOYSTICK_Move_X(0, virtual_joysticks[0].axis_pos[0]);
		JOYSTICK_Move_Y(0, virtual_joysticks[0].axis_pos[1]);
		JOYSTICK_Move_X(1, virtual_joysticks[0].axis_pos[2]);

		Uint8 hat_pos=0;
		if (virtual_joysticks[0].hat_pressed[0]) hat_pos|=SDL_HAT_UP;
		else if (virtual_joysticks[0].hat_pressed[2]) hat_pos|=SDL_HAT_DOWN;
		if (virtual_joysticks[0].hat_pressed[3]) hat_pos|=SDL_HAT_LEFT;
		else if (virtual_joysticks[0].hat_pressed[1]) hat_pos|=SDL_HAT_RIGHT;

		if (hat_pos!=old_hat_position) {
			DecodeHatPosition(hat_pos);
			old_hat_position=hat_pos;
		}
	}

private:
	uint8_t old_hat_position = 0;

	void DecodeHatPosition(Uint8 hat_pos) {
		// Common joystick positions
		constexpr int16_t joy_centered = 0;
		constexpr int16_t joy_full_negative = INT16_MIN;
		constexpr int16_t joy_full_positive = INT16_MAX;
		constexpr int16_t joy_50pct_negative = static_cast<int16_t>(INT16_MIN / 2);
		constexpr int16_t joy_50pct_positive = static_cast<int16_t>(INT16_MAX / 2);

		switch (hat_pos) {
		case SDL_HAT_CENTERED: JOYSTICK_Move_Y(1, joy_full_positive); break;
		case SDL_HAT_UP: JOYSTICK_Move_Y(1, joy_full_negative); break;
		case SDL_HAT_RIGHT: JOYSTICK_Move_Y(1, joy_50pct_negative); break;
		case SDL_HAT_DOWN: JOYSTICK_Move_Y(1, joy_centered); break;
		case SDL_HAT_LEFT: JOYSTICK_Move_Y(1, joy_50pct_positive); break;
		case SDL_HAT_LEFTUP:
			if (JOYSTICK_GetMove_Y(1) < 0)
				JOYSTICK_Move_Y(1, joy_50pct_positive);
			else
				JOYSTICK_Move_Y(1, joy_full_negative);
			break;
		case SDL_HAT_RIGHTUP:
			if (JOYSTICK_GetMove_Y(1) < -0.7)
				JOYSTICK_Move_Y(1, joy_50pct_negative);
			else
				JOYSTICK_Move_Y(1, joy_full_negative);
			break;
		case SDL_HAT_RIGHTDOWN:
			if (JOYSTICK_GetMove_Y(1) < -0.2)
				JOYSTICK_Move_Y(1, joy_centered);
			else
				JOYSTICK_Move_Y(1, joy_50pct_negative);
			break;
		case SDL_HAT_LEFTDOWN:
			if (JOYSTICK_GetMove_Y(1) > 0.2)
				JOYSTICK_Move_Y(1, joy_centered);
			else
				JOYSTICK_Move_Y(1, joy_50pct_positive);
			break;
		}
	}
};

class CCHBindGroup final : public CStickBindGroup {
public:
	CCHBindGroup(uint8_t _stick, uint8_t _emustick) : CStickBindGroup(_stick, _emustick)
	{
		emulated_axes=4;
		emulated_buttons=6;
		emulated_hats=1;
		if (button_wrapping_enabled) button_wrap=emulated_buttons;
		JOYSTICK_Enable(1,true);
	}

	bool CheckEvent(SDL_Event * event) override {
		SDL_JoyAxisEvent * jaxis = nullptr;
		SDL_JoyButtonEvent * jbutton = nullptr;
		SDL_JoyHatEvent * jhat = nullptr;
		Bitu but = 0;
		static const unsigned button_magic[6] = {
		        0x02, 0x04, 0x10, 0x100, 0x20, 0x200};
		static const unsigned hat_magic[2][5] = {
		        {0x8888, 0x8000, 0x800, 0x80, 0x08},
		        {0x5440, 0x4000, 0x400, 0x40, 0x1000}};
		switch(event->type) {
			case SDL_JOYAXISMOTION:
				jaxis = &event->jaxis;
				if(jaxis->which == stick_id && jaxis->axis < 4) {
					if(jaxis->axis & 1)
						JOYSTICK_Move_Y(jaxis->axis >> 1 & 1, jaxis->value);
					else
						JOYSTICK_Move_X(jaxis->axis >> 1 & 1, jaxis->value);
				}
				break;
			case SDL_JOYHATMOTION:
				jhat = &event->jhat;
				if (jhat->which == stick_id && jhat->hat < 2) {
					if (jhat->value == SDL_HAT_CENTERED)
						button_state &= ~hat_magic[jhat->hat][0];
					if (jhat->value & SDL_HAT_UP)
						button_state|=hat_magic[jhat->hat][1];
					if(jhat->value & SDL_HAT_RIGHT)
						button_state|=hat_magic[jhat->hat][2];
					if(jhat->value & SDL_HAT_DOWN)
						button_state|=hat_magic[jhat->hat][3];
					if(jhat->value & SDL_HAT_LEFT)
						button_state|=hat_magic[jhat->hat][4];
				}
				break;
			case SDL_JOYBUTTONDOWN:
				jbutton = &event->jbutton;
				but = jbutton->button % emulated_buttons;
				if (jbutton->which == stick_id)
					button_state|=button_magic[but];
				break;
			case SDL_JOYBUTTONUP:
				jbutton = &event->jbutton;
				but = jbutton->button % emulated_buttons;
				if (jbutton->which == stick_id)
					button_state&=~button_magic[but];
				break;
		}

		unsigned i;
		uint16_t j;
		j=button_state;
		for(i=0;i<16;i++) if (j & 1) break; else j>>=1;
		JOYSTICK_Button(0,0,i&1);
		JOYSTICK_Button(0,1,(i>>1)&1);
		JOYSTICK_Button(1,0,(i>>2)&1);
		JOYSTICK_Button(1,1,(i>>3)&1);
		return false;
	}

	void UpdateJoystick() override {
		static const unsigned button_priority[6] = {7, 11, 13, 14, 5, 6};
		static const unsigned hat_priority[2][4] = {{0, 1, 2, 3},
		                                            {8, 9, 10, 12}};

		/* query SDL joystick and activate bindings */
		ActivateJoystickBoundEvents();

		JOYSTICK_Move_X(0, virtual_joysticks[0].axis_pos[0]);
		JOYSTICK_Move_Y(0, virtual_joysticks[0].axis_pos[1]);
		JOYSTICK_Move_X(1, virtual_joysticks[0].axis_pos[2]);
		JOYSTICK_Move_Y(1, virtual_joysticks[0].axis_pos[3]);

		Bitu bt_state=15;

		for (int i = 0; i < (hats < 2 ? hats : 2); i++) {
			Uint8 hat_pos=0;
			if (virtual_joysticks[0].hat_pressed[(i<<2)+0]) hat_pos|=SDL_HAT_UP;
			else if (virtual_joysticks[0].hat_pressed[(i<<2)+2]) hat_pos|=SDL_HAT_DOWN;
			if (virtual_joysticks[0].hat_pressed[(i<<2)+3]) hat_pos|=SDL_HAT_LEFT;
			else if (virtual_joysticks[0].hat_pressed[(i<<2)+1]) hat_pos|=SDL_HAT_RIGHT;

			if (hat_pos & SDL_HAT_UP)
				if (bt_state>hat_priority[i][0]) bt_state=hat_priority[i][0];
			if (hat_pos & SDL_HAT_DOWN)
				if (bt_state>hat_priority[i][1]) bt_state=hat_priority[i][1];
			if (hat_pos & SDL_HAT_RIGHT)
				if (bt_state>hat_priority[i][2]) bt_state=hat_priority[i][2];
			if (hat_pos & SDL_HAT_LEFT)
				if (bt_state>hat_priority[i][3]) bt_state=hat_priority[i][3];
		}

		bool button_pressed[MAXBUTTON];
		std::fill_n(button_pressed, MAXBUTTON, false);
		for (int i = 0; i < MAX_VJOY_BUTTONS; i++) {
			if (virtual_joysticks[0].button_pressed[i])
				button_pressed[i % button_wrap] = true;
		}
		for (int i = 0; i < 6; i++) {
			if ((button_pressed[i]) && (bt_state>button_priority[i]))
				bt_state=button_priority[i];
		}

		if (bt_state>15) bt_state=15;
		JOYSTICK_Button(0,0,(bt_state&8)==0);
		JOYSTICK_Button(0,1,(bt_state&4)==0);
		JOYSTICK_Button(1,0,(bt_state&2)==0);
		JOYSTICK_Button(1,1,(bt_state&1)==0);
	}

protected:
	uint16_t button_state = 0;
};

#endif