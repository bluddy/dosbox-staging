#include "event.h"


std::string MakeDefaultBind() const {
    if (defkey == SDL_SCANCODE_UNKNOWN)
        return;
    sprintf(buf, "%s \"key %d%s%s%s\"",
            entry, static_cast<int>(defkey),
            defmod & MMOD1 ? " mod1" : "",
            defmod & MMOD2 ? " mod2" : "",
            defmod & MMOD3 ? " mod3" : "");
}

void CEvent::AddBind(std::shared_ptr<CBind> bind) {
	bind_list.push_front(bind);
	bind->event=this;
}

void CEvent::ClearBinds() {
	for (CBind *bind : bindlist) {
		all_binds.remove(bind);
		delete bind;
		bind = nullptr;
	}
	bindlist.clear();
}

void CEvent::DeActivateAll() {
	for (auto &bind : bindlist) {
		bind->DeActivateBind(true);
	}
}

void CEvent::Trigger(bool const deactivation_state) {
	for (auto &bind : bindlist) {
		bind->ActivateBind(32767, true, false);
		bind->DeActivateBind(deactivation_state);
	}
}

void CTriggeredEvent::ActivateEvent(bool ev_trigger, bool skip_action) {
    if (current_value > 25000) {
        /* value exceeds boundary, trigger event if not active */
        if (!activity && !skip_action) Active(true);
        if (activity<32767) activity++;
    } else {
        if (activity>0) {
            /* untrigger event if it is fully inactive */
            DeActivateEvent(ev_trigger);
            activity=0;
        }
    }
}

void CTriggeredEvent::DeActivateEvent(bool /*ev_trigger*/) {
    activity--;
    if (!activity) {
        Active(false);
    }
}
