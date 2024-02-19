#include "event.h"


void CEvent::AddBind(CBind * bind) {
	bindlist.push_front(bind);
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