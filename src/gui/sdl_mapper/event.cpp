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
	for (CBindList_it bit = bindlist.begin() ; bit != bindlist.end(); ++bit) {
		(*bit)->DeActivateBind(true);
	}
}