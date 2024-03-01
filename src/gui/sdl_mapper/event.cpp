#include <sstream>

#include "event.h"
#include "mapper.h"

void CEvent::AddBind(std::shared_ptr<CBind> bind) {
	bind_list.push_front(bind);
}

void CEvent::ClearBinds() {
	bind_list.clear();
}

void CEvent::DeactivateAll() {
	for (auto bind : bind_list) {
		bind->Deactivate(true);
	}
}

void CEvent::Trigger(bool const deactivation_state) {
    // Quick activation and deactivation
	for (auto bind : bind_list) {
		bind->Activate(32767, true, false);
		bind->Deactivate(deactivation_state);
	}
}

void CTriggeredEvent::Activate(bool const ev_trigger, bool const skip_action) {
    if (current_value > 25000) {
        // Value exceeds boundary, trigger event if not active
        if (!activity && !skip_action) {
            SetActive(true);
        }
        if (activity < 32767) {
            activity++;
        }
    } else if (activity > 0) {
        // Untrigger event if it is fully inactive
        Deactivate(ev_trigger);
        activity = 0;
    }
}

void CTriggeredEvent::Deactivate(bool /*ev_trigger*/) {
    activity--;
    if (!activity) {
        SetActive(false);
    }
}

void CContinuousEvent::Activate(bool const ev_trigger, bool const skip_action) {
    if (ev_trigger) {
        activity++;
        if (!skip_action) {
            SetActive(true);
        }
    } else {
        // Test if no trigger-activity is present, this cares especially
        // about activity of the opposite-direction joystick axis for example
        if (!GetActivityCount()) {
            SetActive(true);
        }
    }
}

void CContinuousEvent::Deactivate(bool const ev_trigger) {
    if (ev_trigger || GetActivityCount() == 0) {
        // Zero-out this event's pending activity if triggered
        // or we have no opposite-direction events
        activity = 0;
        SetActive(false);
    }
}

void CModEvent::SetActive(bool yesno)
{
    if (yesno) {
        mapper.mods |= (static_cast<Bitu>(1) << (wmod-1));
    }
    else {
        mapper.mods &= ~(static_cast<Bitu>(1) << (wmod-1));
    }
}

void CKeyEvent::SetActive(bool yesno) {
    KEYBOARD_AddKey(key, yesno);
}

void CMouseButtonEvent::SetActive(const bool pressed)
{
    MOUSE_EventButton(button_id, pressed);
}

CJAxisEvent::CJAxisEvent(Mapper &_mapper, std::string const &name,
            Bitu const s, Bitu const a, bool const p,
            CJAxisEvent* op_axis)
        : CContinuousEvent(_mapper, name),
            stick(s),
            axis(a),
            positive(p),
            opposite_axis(op_axis)
{
    if (opposite_axis) {
        opposite_axis->SetOppositeAxis(this);
    }
}

void CJAxisEvent::SetActive(bool /*moved*/) {
    mapper.virtual_joysticks[stick].axis_pos[axis]=(int16_t)(GetValue()*(positive?1:-1));
}

Bitu CJAxisEvent::GetActivityCount() const {
    return activity | opposite_axis->activity;
}

void CJAxisEvent::RepostActivity() {
    // Caring for joystick movement into the opposite direction
    opposite_axis->SetActive(true);
}

void CJButtonEvent::SetActive(bool pressed)
{
    mapper.virtual_joysticks[stick].button_pressed[button] = pressed;
}

void CJHatEvent::SetActive(bool pressed)
{
    mapper.virtual_joysticks[stick].hat_pressed[(hat<<2)+dir] = pressed;
}

void CModEvent::SetActive(bool yesno)
{
    if (yesno) {
        mapper.mods |= (static_cast<Bitu>(1) << (wmod-1));
    } else {
        mapper.mods &= ~(static_cast<Bitu>(1) << (wmod-1));
    }
}

std::string CHandlerEvent::MakeDefaultBind() const {
    if (defkey == SDL_SCANCODE_UNKNOWN)
        return;

    std::ostringstream oss;
    oss << entry << "\"key " << defkey << 
        (defmod & MMOD1 ? " mod1" : "") <<
        (defmod & MMOD2 ? " mod2" : "") <<
        (defmod & MMOD3 ? " mod3" : "") <<
        "\"";
}
