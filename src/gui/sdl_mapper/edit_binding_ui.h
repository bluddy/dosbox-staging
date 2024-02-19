#ifndef EDIT_BINDING_UI
#define EDIT_BINDING_UI

#include "ui_button.h"

class EditBindingUI {
	// Class holding the "edit binding" display

    EditBindingUI() = default;

	void ChangeActionText(const char* text, const Rgb888& col);

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