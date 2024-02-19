#include "bind.h"

void CBindGroup::ActivateBindList(CBindList * list,Bits value,bool ev_trigger) {
	assert(list);
	Bitu validmod=0;
	CBindList_it it;
	for (it = list->begin(); it != list->end(); ++it) {
		if (((*it)->mods & mapper.mods) == (*it)->mods) {
			if (validmod<(*it)->mods) validmod=(*it)->mods;
		}
	}
	for (it = list->begin(); it != list->end(); ++it) {
		if (validmod==(*it)->mods) (*it)->ActivateBind(value,ev_trigger);
	}
}

void CBindGroup::DeactivateBindList(CBindList * list,bool ev_trigger) {
	assert(list);
	CBindList_it it;
	for (it = list->begin(); it != list->end(); ++it) {
		(*it)->DeActivateBind(ev_trigger);
	}
}
