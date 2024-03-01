#ifndef DOSBOX_SDL_MAPPER_EVENT
#define DOSBOX_SDL_MAPPER_EVENT

#include <string>
#include <memory>
#include <list>
#include <SDL.h>

#include "types.h"
#include "keyboard.h"
#include "mouse.h"
#include "sdl_mapper.h"

class CBind;

// Events are the things that dosbox sends to the program

class CEvent {
public:
	CEvent(Mapper &_mapper, std::string const &_name) : mapper(_mapper), name(_name) {}

	virtual ~CEvent() = default;

    void Trigger(bool const deactivation_state);
	void AddBind(std::shared_ptr<CBind> bind);
	void ClearBinds();
	virtual void SetActive(bool yesno) = 0;
	virtual void Activate(bool ev_trigger, bool skip_action) = 0;
	virtual void Deactivate(bool ev_trigger) = 0;
	void DeactivateAll();
	void SetValue(Bits value) {
		current_value = value;
	}
	Bits GetValue() const {
		return current_value;
	}
	std::string const &GetName() const {
		return name;
	}
	virtual bool IsTrigger() const = 0;

protected:
	// Bindings of this event
	std::list<std::shared_ptr<CBind>> bind_list;
	Bitu activity = 0;  // Number of times triggered?
	std::string name;   // Name of event
	Bits current_value = 0;
	Mapper &mapper;     // Reference to the sdl mapper
};

// Class for events which can be ON/OFF only: key presses, joystick buttons, joystick hat
class CTriggeredEvent : public CEvent {
public:
	CTriggeredEvent(Mapper &_mapper, std::string const &_name) : CEvent(_mapper, _name) {}
	bool IsTrigger() const override {
		return true;
	}
	void Activate(bool ev_trigger, bool skip_action) override;
	void Deactivate(bool /*ev_trigger*/) override;
};

// Class for events which have a non-boolean state: joystick axis movement
class CContinuousEvent : public CEvent {
public:
	CContinuousEvent(CMapper &_mapper, std::string const &_name) :
	 CEvent(_mapper, _name) {}

	bool IsTrigger() const override {
		return false;
	}

	void Activate(bool const ev_trigger, bool const skip_action) override;

	void Deactivate(bool const ev_trigger) override;

	virtual Bitu GetActivityCount() const { return activity; }
	virtual void RepostActivity() {}
};

class CKeyEvent final : public CTriggeredEvent {
public:
	CKeyEvent(Mapper &_mapper, std::string const &name, KBD_KEYS const k) :
		CTriggeredEvent(_mapper, name), key(k)
	{}

	void SetActive(bool yesno) override;

protected:
	KBD_KEYS const key;
};

class CMouseButtonEvent final : public CTriggeredEvent {
public:
	CMouseButtonEvent() = delete;

	CMouseButtonEvent(Mapper &_mapper, std::string const &name, MouseButtonId const id)
		: CTriggeredEvent(_mapper, name), button_id(id)
	{}

	void SetActive(const bool pressed) override;
	
protected:
	MouseButtonId const button_id{MouseButtonId::None};
};

class CJAxisEvent final : public CContinuousEvent {
public:
	CJAxisEvent(Mapper &_mapper, std::string const &name, Bitu const s,
				Bitu const a, bool const p,
				CJAxisEvent* op_axis);

	CJAxisEvent(const CJAxisEvent&) = delete; // prevent copy
	CJAxisEvent& operator=(const CJAxisEvent&) = delete; // prevent assignment

	void SetActive(bool /*moved*/) override;
	Bitu GetActivityCount() const override;
	void RepostActivity() override;
protected:
	void SetOppositeAxis(CJAxisEvent * _opposite_axis) {
		// Circularity requires raw pointer here
		opposite_axis =_opposite_axis;
	}
	Bitu stick;
	Bitu axis;
	bool positive;
	CJAxisEvent* opposite_axis;  // Need raw pointer here
};

class CJButtonEvent final : public CTriggeredEvent {
public:
	CJButtonEvent(Mapper &_mapper, std::string const &_name, Bitu s, Bitu btn)
	        : CTriggeredEvent(_mapper, _name),
	          stick(s),
	          button(btn) {}
	void SetActive(bool pressed) override;

protected:
	Bitu stick;
	Bitu button;
};

class CJHatEvent final : public CTriggeredEvent {
public:
	CJHatEvent(Mapper &_mapper, std::string const &name, Bitu const s, Bitu const h, Bitu const d)
	        : CTriggeredEvent(_mapper, name),
	          stick(s), hat(h), dir(d) {}

	void SetActive(bool pressed) override;

protected:
	Bitu stick;
	Bitu hat;
	Bitu dir;
};

class CModEvent final : public CTriggeredEvent {
public:
	CModEvent(Mapper &_mapper, std::string const &_name, int const _wmod)
	        : CTriggeredEvent(_mapper, _name),
	          wmod(_wmod)
	{}

	void SetActive(bool yesno) override;

protected:
	int wmod;
};

class CHandlerEvent final : public CTriggeredEvent {
	// Custom handler associated with key
public:
	CHandlerEvent(Mapper &_mapper,
		          std::string const &name,
	              MAPPER_Handler *handle,
	              SDL_Scancode const k,
	              uint32_t const mod,
	              std::string const &_button_name)
	        : CTriggeredEvent(_mapper, name),
	          defkey(k),
	          defmod(mod),
	          handler(handle),
	          button_name(_button_name)
	{}

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