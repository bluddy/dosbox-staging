#ifndef DOSBOX_SDL_MAPPER_EVENT
#define DOSBOX_SDL_MAPPER_EVENT

#include <string>
#include <memory>
#include <list>

#include "types.h"

class CBind;
typedef std::list<std::shared_ptr<CBind>> CBindList;

class CEvent {
public:
	CEvent(std::string _entry) : entry(_entry) {}

	virtual ~CEvent() = default;

    void Trigger(bool const deactivation_state);
	void AddBind(std::shared_ptr<CBind> bind);
	void ClearBinds();
	virtual void SetActive(bool yesno) = 0;
	virtual void ActivateEvent(bool ev_trigger, bool skip_action) = 0;
	virtual void DeActivateEvent(bool ev_trigger) = 0;
	void DeActivateAll();
	void SetValue(Bits value) {
		current_value = value;
	}
	Bits GetValue() const {
		return current_value;
	}
	std::string GetName() const {
		return entry;
	}
	virtual bool IsTrigger() = 0;

protected:
	CBindList bind_list;
	Bitu activity = 0;
	std::string entry;
	Bits current_value = 0;
};

/* Class for events which can be ON/OFF only: key presses, joystick buttons, joystick hat */
class CTriggeredEvent : public CEvent {
public:
	CTriggeredEvent(std::string const _entry) : CEvent(_entry) {}
	bool IsTrigger() const {
		return true;
	}
	void ActivateEvent(bool ev_trigger, bool skip_action) override;
	void DeActivateEvent(bool /*ev_trigger*/) override;
};

/* class for events which have a non-boolean state: joystick axis movement */
class CContinuousEvent : public CEvent {
public:
	CContinuousEvent(const char* const _entry) : CEvent(_entry) {}
	bool IsTrigger() override {
		return false;
	}
	void ActivateEvent(bool ev_trigger,bool skip_action) override {
		if (ev_trigger) {
			activity++;
			if (!skip_action) SetActive(true);
		} else {
			/* test if no trigger-activity is present, this cares especially
			   about activity of the opposite-direction joystick axis for example */
			if (!GetActivityCount()) SetActive(true);
		}
	}
	void DeActivateEvent(const bool ev_trigger) override
	{
		if (ev_trigger || GetActivityCount() == 0) {
			// Zero-out this event's pending activity if triggered
			// or we have no opposite-direction events
			activity = 0;
			SetActive(false);
		}
	}

	virtual Bitu GetActivityCount() {
		return activity;
	}
	virtual void RepostActivity() {}
};

class CKeyEvent final : public CTriggeredEvent {
public:
	CKeyEvent(const char* const entry, KBD_KEYS k)
	        : CTriggeredEvent(entry),
	          key(k)
	{}

	void SetActive(bool yesno) override {
		KEYBOARD_AddKey(key, yesno);
	}

	KBD_KEYS key;
};

class CMouseButtonEvent final : public CTriggeredEvent {
public:
	CMouseButtonEvent() = delete;

	CMouseButtonEvent(const char* const entry, const MouseButtonId id)
	        : CTriggeredEvent(entry),
	          button_id(id)
	{}

	void SetActive(const bool pressed) override
	{
		MOUSE_EventButton(button_id, pressed);
	}

private:
	const MouseButtonId button_id = MouseButtonId::None;
};

class CJAxisEvent final : public CContinuousEvent {
public:
	CJAxisEvent(const char* const entry, Bitu s, Bitu a, bool p,
	            CJAxisEvent* op_axis)
	        : CContinuousEvent(entry),
	          stick(s),
	          axis(a),
	          positive(p),
	          opposite_axis(op_axis)
	{
		if (opposite_axis)
			opposite_axis->SetOppositeAxis(this);
	}

	CJAxisEvent(const CJAxisEvent&) = delete; // prevent copy
	CJAxisEvent& operator=(const CJAxisEvent&) = delete; // prevent assignment

	void SetActive(bool /*moved*/) override {
		virtual_joysticks[stick].axis_pos[axis]=(int16_t)(GetValue()*(positive?1:-1));
	}
	Bitu GetActivityCount() override {
		return activity|opposite_axis->activity;
	}
	void RepostActivity() override {
		/* caring for joystick movement into the opposite direction */
		opposite_axis->SetActive(true);
	}
protected:
	void SetOppositeAxis(CJAxisEvent * _opposite_axis) {
		opposite_axis=_opposite_axis;
	}
	Bitu stick,axis;
	bool positive;
	CJAxisEvent * opposite_axis;
};

class CJButtonEvent final : public CTriggeredEvent {
public:
	CJButtonEvent(const char* const entry, Bitu s, Bitu btn)
	        : CTriggeredEvent(entry),
	          stick(s),
	          button(btn)
	{}

	void SetActive(bool pressed) override
	{
		virtual_joysticks[stick].button_pressed[button]=pressed;
	}

protected:
	Bitu stick,button;
};

class CJHatEvent final : public CTriggeredEvent {
public:
	CJHatEvent(const char* const entry, Bitu s, Bitu h, Bitu d)
	        : CTriggeredEvent(entry),
	          stick(s),
	          hat(h),
	          dir(d)
	{}

	void SetActive(bool pressed) override
	{
		virtual_joysticks[stick].hat_pressed[(hat<<2)+dir]=pressed;
	}

protected:
	Bitu stick,hat,dir;
};

class CModEvent final : public CTriggeredEvent {
public:
	CModEvent(const char* const _entry, int _wmod)
	        : CTriggeredEvent(_entry),
	          wmod(_wmod)
	{}

	void SetActive(bool yesno) override
	{
		if (yesno)
			mapper.mods |= (static_cast<Bitu>(1) << (wmod-1));
		else
			mapper.mods &= ~(static_cast<Bitu>(1) << (wmod-1));
	}

protected:
	int wmod;
};

class CHandlerEvent final : public CTriggeredEvent {
public:
	CHandlerEvent(const char *entry,
	              MAPPER_Handler *handle,
	              SDL_Scancode k,
	              uint32_t mod,
	              const char *bname)
	        : CTriggeredEvent(entry),
	          defkey(k),
	          defmod(mod),
	          handler(handle),
	          button_name(bname)
	{
		handlergroup.push_back(this);
	}

	~CHandlerEvent() override = default;
	CHandlerEvent(const CHandlerEvent&) = delete; // prevent copy
	CHandlerEvent& operator=(const CHandlerEvent&) = delete; // prevent assignment

	void SetActive(bool yesno) override { (*handler)(yesno); }
	std::string MakeDefaultBind() const;

protected:
	SDL_Scancode defkey;
	uint32_t defmod;
	MAPPER_Handler * handler;
public:
	std::string button_name;
};

#endif