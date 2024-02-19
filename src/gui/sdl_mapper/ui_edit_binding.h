#ifndef EDIT_BINDING_UI
#define EDIT_BINDING_UI

#include <string>

#include "ui_button.h"

class CBind;
class CEvent;
class CBindList;

class EditBindingUI {
	// Class holding the "edit binding" display

    EditBindingUI() = default;

	void ChangeActionText(const char* text, const Rgb888& col);
	std::string HumanizeKeyName(const CBindList &binds, const std::string &fallback);
	void Update();
	void SetActiveBind(CBind *new_active_bind);
	void SetActiveEvent(CEvent * event);

private:
	CCaptionButton *event_title;
	CCaptionButton *bind_title;
	CCaptionButton *selected;
	CCaptionButton *action;
	CBindButton *save;
	CBindButton *exit;
	CBindButton *add;
	CBindButton *del;
	CBindButton *next;
	CCheckButton *mod1,*mod2,*mod3,*hold;
};


#endif