#include "ui.h"

void MapperUI::DrawButtons() const
{
	SDL_SetRenderDrawColor(mapper.renderer,
	                       color_black.red,
	                       color_black.green,
	                       color_black.blue,
	                       SDL_ALPHA_OPAQUE);
	SDL_RenderClear(mapper.renderer);
	for (const auto& button : buttons) {
		button->Draw();
	}
	SDL_RenderPresent(mapper.renderer);
}


void MapperUI::CreateLayout() {

	int32_t i;
	/* Create the buttons for the Keyboard */
	constexpr int32_t button_width  = 28;
	constexpr int32_t button_height = 20;
	constexpr int32_t margin        = 5;
	constexpr auto pos_x = [](int32_t x) { return x * button_width + margin; };
	constexpr auto pos_y = [](int32_t y) { return 10 + y * button_height; };
	AddKeyButtonEvent(pos_x(0), pos_y(0), button_width, button_height, "ESC", "esc", KBD_esc);
	for (i = 0; i < 12; i++) {
		AddKeyButtonEvent(pos_x(2 + i), pos_y(0), button_width, button_height, combo_f[i].title, combo_f[i].entry, combo_f[i].key);
	}
	for (i = 0; i < 14; i++) {
		AddKeyButtonEvent(pos_x(i), pos_y(1), button_width, button_height, combo_1[i].title, combo_1[i].entry, combo_1[i].key);
	}

	AddKeyButtonEvent(pos_x(0), pos_y(2), button_width * 2, button_height, "TAB", "tab", KBD_tab);
	for (i = 0; i < 12; i++) {
		AddKeyButtonEvent(pos_x(2 + i), pos_y(2), button_width, button_height, combo_2[i].title, combo_2[i].entry, combo_2[i].key);
	}

	AddKeyButtonEvent(pos_x(14), pos_y(2), button_width * 2, button_height * 2, "ENTER", "enter", KBD_enter);

	caps_lock_event = AddKeyButtonEvent(pos_x(0), pos_y(3), button_width * 2, button_height, "CLCK", "capslock", KBD_capslock);
	for (i = 0; i < 12; i++) {
		AddKeyButtonEvent(pos_x(2 + i), pos_y(3), button_width, button_height, combo_3[i].title, combo_3[i].entry, combo_3[i].key);
	}

	AddKeyButtonEvent(pos_x(0), pos_y(4), button_width * 2, button_height, "SHIFT", "lshift", KBD_leftshift);
	for (i = 0; i < 12; i++) {
		AddKeyButtonEvent(pos_x(2 + i),
		                  pos_y(4),
		                  button_width,
		                  button_height,
		                  combo_4[i].title,
		                  combo_4[i].entry,
		                  combo_4[i].key);
	}
	AddKeyButtonEvent(pos_x(14), pos_y(4), button_width * 3, button_height, "SHIFT", "rshift", KBD_rightshift);

	/* Bottom Row */
	AddKeyButtonEvent(pos_x(0), pos_y(5), button_width * 2, button_height, MMOD1_NAME, "lctrl", KBD_leftctrl);

#if !defined(MACOSX)
	AddKeyButtonEvent(pos_x(2), pos_y(5), button_width * 2, button_height, MMOD3_NAME, "lgui", KBD_leftgui);
	AddKeyButtonEvent(pos_x(4), pos_y(5), button_width * 2, button_height, MMOD2_NAME, "lalt", KBD_leftalt);
#else
	AddKeyButtonEvent(pos_x(2), pos_y(5), button_width * 2, button_height, MMOD2_NAME, "lalt", KBD_leftalt);
	AddKeyButtonEvent(pos_x(4), pos_y(5), button_width * 2, button_height, MMOD3_NAME, "lgui", KBD_leftgui);
#endif

	AddKeyButtonEvent(pos_x(6), pos_y(5), button_width * 4, button_height, "SPACE", "space", KBD_space);

#if !defined(MACOSX)
	AddKeyButtonEvent(pos_x(10), pos_y(5), button_width * 2, button_height, MMOD2_NAME, "ralt", KBD_rightalt);
	AddKeyButtonEvent(pos_x(12), pos_y(5), button_width * 2, button_height, MMOD3_NAME, "rgui", KBD_rightgui);
#else
	AddKeyButtonEvent(pos_x(10), pos_y(5), button_width * 2, button_height, MMOD3_NAME, "rgui", KBD_rightgui);
	AddKeyButtonEvent(pos_x(12), pos_y(5), button_width * 2, button_height, MMOD2_NAME, "ralt", KBD_rightalt);
#endif

	AddKeyButtonEvent(pos_x(14), pos_y(5), button_width * 2, button_height, MMOD1_NAME, "rctrl", KBD_rightctrl);

	/* Arrow Keys */
#define XO 17
#define YO 0

	AddKeyButtonEvent(pos_x(XO + 0), pos_y(YO), button_width, button_height, "PRT", "printscreen", KBD_printscreen);
	AddKeyButtonEvent(pos_x(XO + 1), pos_y(YO), button_width, button_height, "SCL", "scrolllock", KBD_scrolllock);
	AddKeyButtonEvent(pos_x(XO + 2), pos_y(YO), button_width, button_height, "PAU", "pause", KBD_pause);
	AddKeyButtonEvent(pos_x(XO + 0), pos_y(YO + 1), button_width, button_height, "INS", "insert", KBD_insert);
	AddKeyButtonEvent(pos_x(XO + 1), pos_y(YO + 1), button_width, button_height, "HOM", "home", KBD_home);
	AddKeyButtonEvent(pos_x(XO + 2), pos_y(YO + 1), button_width, button_height, "PUP", "pageup", KBD_pageup);
	AddKeyButtonEvent(pos_x(XO + 0), pos_y(YO + 2), button_width, button_height, "DEL", "delete", KBD_delete);
	AddKeyButtonEvent(pos_x(XO + 1), pos_y(YO + 2), button_width, button_height, "END", "end", KBD_end);
	AddKeyButtonEvent(pos_x(XO + 2), pos_y(YO + 2), button_width, button_height, "PDN", "pagedown", KBD_pagedown);
	AddKeyButtonEvent(pos_x(XO + 1), pos_y(YO + 4), button_width, button_height, "\x18", "up", KBD_up);
	AddKeyButtonEvent(pos_x(XO + 0), pos_y(YO + 5), button_width, button_height, "\x1B", "left", KBD_left);
	AddKeyButtonEvent(pos_x(XO + 1), pos_y(YO + 5), button_width, button_height, "\x19", "down", KBD_down);
	AddKeyButtonEvent(pos_x(XO + 2), pos_y(YO + 5), button_width, button_height, "\x1A", "right", KBD_right);
#undef XO
#undef YO
#define XO 0
#define YO 7
	/* Numeric KeyPad */
	num_lock_event = AddKeyButtonEvent(pos_x(XO), pos_y(YO), button_width, button_height, "NUM", "numlock", KBD_numlock);
	AddKeyButtonEvent(pos_x(XO + 1), pos_y(YO), button_width, button_height, "/", "kp_divide", KBD_kpdivide);
	AddKeyButtonEvent(pos_x(XO + 2), pos_y(YO), button_width, button_height, "*", "kp_multiply", KBD_kpmultiply);
	AddKeyButtonEvent(pos_x(XO + 3), pos_y(YO), button_width, button_height, "-", "kp_minus", KBD_kpminus);
	AddKeyButtonEvent(pos_x(XO + 0), pos_y(YO + 1), button_width, button_height, "7", "kp_7", KBD_kp7);
	AddKeyButtonEvent(pos_x(XO + 1), pos_y(YO + 1), button_width, button_height, "8", "kp_8", KBD_kp8);
	AddKeyButtonEvent(pos_x(XO + 2), pos_y(YO + 1), button_width, button_height, "9", "kp_9", KBD_kp9);
	AddKeyButtonEvent(pos_x(XO + 3), pos_y(YO + 1), button_width, button_height * 2, "+", "kp_plus", KBD_kpplus);
	AddKeyButtonEvent(pos_x(XO), pos_y(YO + 2), button_width, button_height, "4", "kp_4", KBD_kp4);
	AddKeyButtonEvent(pos_x(XO + 1), pos_y(YO + 2), button_width, button_height, "5", "kp_5", KBD_kp5);
	AddKeyButtonEvent(pos_x(XO + 2), pos_y(YO + 2), button_width, button_height, "6", "kp_6", KBD_kp6);
	AddKeyButtonEvent(pos_x(XO + 0), pos_y(YO + 3), button_width, button_height, "1", "kp_1", KBD_kp1);
	AddKeyButtonEvent(pos_x(XO + 1), pos_y(YO + 3), button_width, button_height, "2", "kp_2", KBD_kp2);
	AddKeyButtonEvent(pos_x(XO + 2), pos_y(YO + 3), button_width, button_height, "3", "kp_3", KBD_kp3);
	AddKeyButtonEvent(pos_x(XO + 3), pos_y(YO + 3), button_width, button_height * 2, "ENT", "kp_enter", KBD_kpenter);
	AddKeyButtonEvent(pos_x(XO), pos_y(YO + 4), button_width * 2, button_height, "0", "kp_0", KBD_kp0);
	AddKeyButtonEvent(pos_x(XO + 2), pos_y(YO + 4), button_width, button_height, ".", "kp_period", KBD_kpperiod);

#undef XO
#undef YO

#define XO 5
#define YO 8
	/* Mouse Buttons */
	new CTextButton(pos_x(XO + 0), pos_y(YO - 1), 3 * button_width, 20, "Mouse");

	AddMouseButtonEvent(pos_x(XO + 0),
	                    pos_y(YO),
	                    button_width,
	                    button_height,
	                    "L",
	                    "mouse_left",
	                    MouseButtonId::Left);

	AddMouseButtonEvent(pos_x(XO + 1),
	                    pos_y(YO),
	                    button_width,
	                    button_height,
	                    "M",
	                    "mouse_middle",
	                    MouseButtonId::Middle);

	AddMouseButtonEvent(pos_x(XO + 2),
	                    pos_y(YO),
	                    button_width,
	                    button_height,
	                    "R",
	                    "mouse_right",
	                    MouseButtonId::Right);
#undef XO
#undef YO

#define XO 10
#define YO 7
	/* Joystick Buttons/Texts */
	/* Buttons 1+2 of 1st Joystick */
	AddJButtonButton(pos_x(XO), pos_y(YO), button_width, button_height, "1", 0, 0);
	AddJButtonButton(pos_x(XO + 2), pos_y(YO), button_width, button_height, "2", 0, 1);
	/* Axes 1+2 (X+Y) of 1st Joystick */
	CJAxisEvent* cjaxis = AddJAxisButton(pos_x(XO + 1), pos_y(YO), button_width, button_height, "Y-", 0, 1, false, nullptr);
	AddJAxisButton(pos_x(XO + 1), pos_y(YO + 1), button_width, button_height, "Y+", 0, 1, true, cjaxis);
	cjaxis = AddJAxisButton(pos_x(XO), pos_y(YO + 1), button_width, button_height, "X-", 0, 0, false, nullptr);
	AddJAxisButton(pos_x(XO + 2), pos_y(YO + 1), button_width, button_height, "X+", 0, 0, true, cjaxis);

	CJAxisEvent * tmp_ptr;

	assert(joytype != JOY_UNSET);
	if (joytype == JOY_2AXIS) {
		/* Buttons 1+2 of 2nd Joystick */
		AddJButtonButton(pos_x(XO + 4), pos_y(YO), button_width, button_height, "1", 1, 0);
		AddJButtonButton(pos_x(XO + 4 + 2), pos_y(YO), button_width, button_height, "2", 1, 1);
		/* Buttons 3+4 of 1st Joystick, not accessible */
		AddJButtonButton_hidden(0,2);
		AddJButtonButton_hidden(0,3);

		/* Axes 1+2 (X+Y) of 2nd Joystick */
		cjaxis  = AddJAxisButton(pos_x(XO + 4), pos_y(YO + 1), button_width, button_height, "X-", 1, 0, false, nullptr);
		tmp_ptr = AddJAxisButton(pos_x(XO + 4 + 2), pos_y(YO + 1), button_width, button_height, "X+", 1, 0, true, cjaxis);
		(void)tmp_ptr;
		cjaxis  = AddJAxisButton(pos_x(XO + 4 + 1), pos_y(YO + 0), button_width, button_height, "Y-", 1, 1, false, nullptr);
		tmp_ptr = AddJAxisButton(pos_x(XO + 4 + 1), pos_y(YO + 1), button_width, button_height, "Y+", 1, 1, true, cjaxis);
		(void)tmp_ptr;
		/* Axes 3+4 (X+Y) of 1st Joystick, not accessible */
		cjaxis  = AddJAxisButton_hidden(0, 2, false, nullptr);
		tmp_ptr = AddJAxisButton_hidden(0, 2, true, cjaxis);
		(void)tmp_ptr;
		cjaxis  = AddJAxisButton_hidden(0, 3, false, nullptr);
		tmp_ptr = AddJAxisButton_hidden(0, 3, true, cjaxis);
		(void)tmp_ptr;
	} else {
		/* Buttons 3+4 of 1st Joystick */
		AddJButtonButton(pos_x(XO + 4), pos_y(YO), button_width, button_height, "3", 0, 2);
		AddJButtonButton(pos_x(XO + 4 + 2), pos_y(YO), button_width, button_height, "4", 0, 3);
		/* Buttons 1+2 of 2nd Joystick, not accessible */
		AddJButtonButton_hidden(1, 0);
		AddJButtonButton_hidden(1, 1);

		/* Axes 3+4 (X+Y) of 1st Joystick */
		cjaxis  = AddJAxisButton(pos_x(XO + 4), pos_y(YO + 1), button_width, button_height, "X-", 0, 2, false, nullptr);
		tmp_ptr = AddJAxisButton(pos_x(XO + 4 + 2), pos_y(YO + 1), button_width, button_height, "X+", 0, 2, true, cjaxis);
		(void)tmp_ptr;
		cjaxis  = AddJAxisButton(pos_x(XO + 4 + 1), pos_y(YO + 0), button_width, button_height, "Y-", 0, 3, false, nullptr);
		tmp_ptr = AddJAxisButton(pos_x(XO + 4 + 1), pos_y(YO + 1), button_width, button_height, "Y+", 0, 3, true, cjaxis);
		(void)tmp_ptr;
		/* Axes 1+2 (X+Y) of 2nd Joystick , not accessible*/
		cjaxis  = AddJAxisButton_hidden(1, 0, false, nullptr);
		tmp_ptr = AddJAxisButton_hidden(1, 0, true, cjaxis);
		(void)tmp_ptr;
		cjaxis  = AddJAxisButton_hidden(1, 1, false, nullptr);
		tmp_ptr = AddJAxisButton_hidden(1, 1, true, cjaxis);
		(void)tmp_ptr;
	}

	if (joytype == JOY_CH) {
		/* Buttons 5+6 of 1st Joystick */
		AddJButtonButton(pos_x(XO + 8), pos_y(YO), button_width, button_height, "5", 0, 4);
		AddJButtonButton(pos_x(XO + 8 + 2), pos_y(YO), button_width, button_height, "6", 0, 5);
	} else {
		/* Buttons 5+6 of 1st Joystick, not accessible */
		AddJButtonButton_hidden(0, 4);
		AddJButtonButton_hidden(0, 5);
	}

	/* Hat directions up, left, down, right */
	AddJHatButton(pos_x(XO + 8 + 1), pos_y(YO), button_width, button_height, "UP", 0, 0, 0);
	AddJHatButton(pos_x(XO + 8 + 0), pos_y(YO + 1), button_width, button_height, "LFT", 0, 0, 3);
	AddJHatButton(pos_x(XO + 8 + 1), pos_y(YO + 1), button_width, button_height, "DWN", 0, 0, 2);
	AddJHatButton(pos_x(XO + 8 + 2), pos_y(YO + 1), button_width, button_height, "RGT", 0, 0, 1);

	/* Labels for the joystick */
	CTextButton * btn;
	if (joytype == JOY_2AXIS) {
		new CTextButton(pos_x(XO + 0), pos_y(YO - 1), 3 * button_width, 20, "Joystick 1");
		new CTextButton(pos_x(XO + 4), pos_y(YO - 1), 3 * button_width, 20, "Joystick 2");
		btn = new CTextButton(pos_x(XO + 8), pos_y(YO - 1), 3 * button_width, 20, "Disabled");
		btn->SetColor(color_grey);
	} else if(joytype == JOY_4AXIS || joytype == JOY_4AXIS_2) {
		new CTextButton(pos_x(XO + 0), pos_y(YO - 1), 3 * button_width, 20, "Axis 1/2");
		new CTextButton(pos_x(XO + 4), pos_y(YO - 1), 3 * button_width, 20, "Axis 3/4");
		btn = new CTextButton(pos_x(XO + 8), pos_y(YO - 1), 3 * button_width, 20, "Disabled");
		btn->SetColor(color_grey);
	} else if(joytype == JOY_CH) {
		new CTextButton(pos_x(XO + 0), pos_y(YO - 1), 3 * button_width, 20, "Axis 1/2");
		new CTextButton(pos_x(XO + 4), pos_y(YO - 1), 3 * button_width, 20, "Axis 3/4");
		new CTextButton(pos_x(XO + 8), pos_y(YO - 1), 3 * button_width, 20, "Hat/D-pad");
	} else if ( joytype == JOY_FCS) {
		new CTextButton(pos_x(XO + 0), pos_y(YO - 1), 3 * button_width, 20, "Axis 1/2");
		new CTextButton(pos_x(XO + 4), pos_y(YO - 1), 3 * button_width, 20, "Axis 3");
		new CTextButton(pos_x(XO + 8), pos_y(YO - 1), 3 * button_width, 20, "Hat/D-pad");
	} else if (joytype == JOY_DISABLED) {
		btn = new CTextButton(pos_x(XO + 0), pos_y(YO - 1), 3 * button_width, 20, "Disabled");
		btn->SetColor(color_grey);
		btn = new CTextButton(pos_x(XO + 4), pos_y(YO - 1), 3 * button_width, 20, "Disabled");
		btn->SetColor(color_grey);
		btn = new CTextButton(pos_x(XO + 8), pos_y(YO - 1), 3 * button_width, 20, "Disabled");
		btn->SetColor(color_grey);
	}

	/* The modifier buttons */
	AddModButton(pos_x(0), pos_y(14), 50, 20, "Mod1", 1);
	AddModButton(pos_x(2), pos_y(14), 50, 20, "Mod2", 2);
	AddModButton(pos_x(4), pos_y(14), 50, 20, "Mod3", 3);

	/* Create Handler buttons */
	int32_t xpos = 0;
	int32_t ypos = 10;
	constexpr auto bw = button_width + 5;
	for (const auto &handler_event : handlergroup) {
		new CEventButton(200 + xpos * 3 * bw, pos_y(ypos), bw * 3, button_height,
		                 handler_event->button_name.c_str(), handler_event);
		xpos++;
		if (xpos>3) {
			xpos = 0;
			ypos++;
		}
	}
	/* Create some text buttons */
//	new CTextButton(pos_x(6), 0, 124, 20, "Keyboard Layout");
//	new CTextButton(pos_x(17), 0, 124, 20, "Joystick Layout");

	bind_but.action = new CCaptionButton(0, 335, 0, 0);

	bind_but.event_title=new CCaptionButton(0,350,0,0);
	bind_but.bind_title=new CCaptionButton(0,365,0,0);

	/* Create binding support buttons */

	bind_but.mod1 = new CCheckButton(20, 410, 110, 20, "Mod1", BC_Mod1);
	bind_but.mod2 = new CCheckButton(20, 432, 110, 20, "Mod2", BC_Mod2);
	bind_but.mod3 = new CCheckButton(20, 454, 110, 20, "Mod3", BC_Mod3);
	bind_but.hold = new CCheckButton(150, 410, 60, 20, "Hold", BC_Hold);

	bind_but.add = new CBindButton(250, 380, 100, 20, "Add bind", BB_Add);
	bind_but.del = new CBindButton(250, 400, 100, 20, "Remove bind", BB_Del);
	bind_but.next = new CBindButton(250, 420, 100, 20, "Next bind", BB_Next);

	bind_but.save=new CBindButton(400,450,50,20,"Save",BB_Save);
	bind_but.exit=new CBindButton(450,450,50,20,"Exit",BB_Exit);

	bind_but.bind_title->Change("Bind Title");
}

CKeyEvent* MapperUI::AddKeyButtonEvent(int32_t x, int32_t y, int32_t dx,
                                    int32_t dy, const char* const title,
                                    const char* const entry, KBD_KEYS key)
{
	char buf[64];
	safe_strcpy(buf, "key_");
	safe_strcat(buf, entry);
	CKeyEvent * event=new CKeyEvent(buf,key);
	new CEventButton(x,y,dx,dy,title,event);
	return event;
}

CMouseButtonEvent* MapperUI::AddMouseButtonEvent(const int32_t x, const int32_t y,
                                              const int32_t dx, const int32_t dy,
                                              const char* const title,
                                              const char* const entry,
                                              const MouseButtonId button_id)
{
	auto event = new CMouseButtonEvent(entry, button_id);
	new CEventButton(x, y, dx, dy, title, event);
	return event;
}

CJAxisEvent* MapperUI::AddJAxisButton(int32_t x, int32_t y, int32_t dx, int32_t dy,
                                   const char* const title, Bitu stick, Bitu axis,
                                   bool positive, CJAxisEvent* opposite_axis)
{
	char buf[64];
	sprintf(buf, "jaxis_%d_%d%s",
	        static_cast<int>(stick),
	        static_cast<int>(axis),
	        positive ? "+" : "-");
	CJAxisEvent	* event=new CJAxisEvent(buf,stick,axis,positive,opposite_axis);
	new CEventButton(x,y,dx,dy,title,event);
	return event;
}
CJAxisEvent * MapperUI::AddJAxisButton_hidden(Bitu stick,Bitu axis,bool positive,CJAxisEvent * opposite_axis) {
	char buf[64];
	sprintf(buf, "jaxis_%d_%d%s",
	        static_cast<int>(stick),
	        static_cast<int>(axis),
	        positive ? "+" : "-");
	return new CJAxisEvent(buf,stick,axis,positive,opposite_axis);
}

void MapperUI::AddJButtonButton(int32_t x, int32_t y, int32_t dx, int32_t dy,
                             const char* const title, Bitu stick, Bitu button)
{
	char buf[64];
	sprintf(buf, "jbutton_%d_%d",
	        static_cast<int>(stick),
	        static_cast<int>(button));
	CJButtonEvent * event=new CJButtonEvent(buf,stick,button);
	new CEventButton(x,y,dx,dy,title,event);
}

void MapperUI::AddJButtonButton_hidden(Bitu stick,Bitu button) {
	char buf[64];
	sprintf(buf, "jbutton_%d_%d",
	        static_cast<int>(stick),
	        static_cast<int>(button));
	new CJButtonEvent(buf,stick,button);
}

void MapperUI::AddJHatButton(int32_t x, int32_t y, int32_t dx, int32_t dy,
                          const char* const title, Bitu _stick, Bitu _hat, Bitu _dir)
{
	char buf[64];
	sprintf(buf, "jhat_%d_%d_%d",
	        static_cast<int>(_stick),
	        static_cast<int>(_hat),
	        static_cast<int>(_dir));
	CJHatEvent * event=new CJHatEvent(buf,_stick,_hat,_dir);
	new CEventButton(x,y,dx,dy,title,event);
}

void MapperUI::AddModButton(int32_t x, int32_t y, int32_t dx, int32_t dy,
                         const char* const title, int mod)
{
	char buf[64];
	sprintf(buf, "mod_%d", mod);
	CModEvent *event = new CModEvent(buf, mod);
	new CEventButton(x, y, dx, dy, title, event);
}

void MapperUI::MappingEvents() {
	SDL_Event event;
	static bool isButtonPressed = false;
	static CButton *lastHoveredButton = nullptr;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_MOUSEBUTTONDOWN:
			isButtonPressed = true;
			/* Further check where are we pointing at right now */
			[[fallthrough]];
		case SDL_MOUSEMOTION:
			if (!isButtonPressed)
				break;
			/* Maybe we have been pointing at a specific button for
			 * a little while  */
			if (lastHoveredButton) {
				/* Check if there's any change */
				if (lastHoveredButton->OnTop(event.button.x,event.button.y))
					break;
				/* Not pointing at given button anymore */
				if (lastHoveredButton == last_clicked)
					lastHoveredButton->Click();
				else
					lastHoveredButton->BindColor();
				mapper.redraw = true;
				lastHoveredButton = nullptr;
			}
			/* Check which button are we currently pointing at */
			for (const auto& button : buttons) {
				if (dynamic_cast<CClickableTextButton*>(button.get()) &&
				    button->OnTop(event.button.x, event.button.y)) {
					button->SetColor(color_red);
					mapper.redraw     = true;
					lastHoveredButton = button.get();
					break;
				}
			}
			break;
		case SDL_MOUSEBUTTONUP:
			isButtonPressed = false;
			if (lastHoveredButton) {
				/* For most buttons the actual new color is going to be green; But not for a few others. */
				lastHoveredButton->BindColor();
				mapper.redraw = true;
				lastHoveredButton = nullptr;
			}
			/* Check the press */
			for (const auto& button : buttons) {
				if (dynamic_cast<CClickableTextButton*>(button.get()) &&
				    button->OnTop(event.button.x, event.button.y)) {
					button->Click();
					break;
				}
			}
			SetActiveBind(mapper.abind); // force redraw key binding
			                             // description
			break;
		case SDL_WINDOWEVENT:
			/* The resize event MAY arrive e.g. when the mapper is
			 * toggled, at least on X11. Furthermore, the restore
			 * event should be handled on Android.
			 */
			if ((event.window.event == SDL_WINDOWEVENT_RESIZED) ||
			    (event.window.event == SDL_WINDOWEVENT_RESTORED)) {
				GFX_UpdateDisplayDimensions(event.window.data1,
				                            event.window.data2);
				SDL_RenderSetLogicalSize(mapper.renderer, 640, 480);
				mapper.redraw = true;
			}
			if (event.window.event == SDL_WINDOWEVENT_EXPOSED) {
				mapper.redraw = true;
			}
			break;
		case SDL_QUIT:
			isButtonPressed = false;
			lastHoveredButton = nullptr;
			mapper.exit=true;
			break;
		default:
			if (mapper.addbind) for (CBindGroup_it it = bindgroups.begin(); it != bindgroups.end(); ++it) {
				CBind * newbind=(*it)->CreateEventBind(&event);
				if (!newbind) continue;
				mapper.aevent->AddBind(newbind);
				SetActiveEvent(mapper.aevent);
				mapper.addbind=false;
				break;
			}
		}
	}
}

void MapperUI::Display()
{
	MOUSE_NotifyTakeOver(true);

	// The mapper is about to take-over SDL's surface and rendering
	// functions, so disengage the main ones. When the mapper closes, SDL
	// main will recreate its rendering pipeline.
	GFX_DisengageRendering();

	// Be sure that there is no update in progress
	GFX_EndUpdate( nullptr );
	mapper.window = GFX_GetWindow();
	if (mapper.window == nullptr) {
		E_Exit("MAPPER: Could not initialize video mode: %s",
		       SDL_GetError());
	}
	mapper.renderer = SDL_GetRenderer(mapper.window);
#if C_OPENGL
	SDL_GLContext context = nullptr;
	if (!mapper.renderer) {
		context = SDL_GL_GetCurrentContext();
		if (!context) {
			E_Exit("MAPPER: Failed to retrieve current OpenGL context: %s",
			       SDL_GetError());
		}

		const auto renderer_drivers_count = SDL_GetNumRenderDrivers();
		if (renderer_drivers_count <= 0) {
			E_Exit("MAPPER: Failed to retrieve available SDL renderer drivers: %s",
			       SDL_GetError());
		}
		int renderer_driver_index = -1;
		for (int i = 0; i < renderer_drivers_count; ++i) {
			SDL_RendererInfo renderer_info = {};
			if (SDL_GetRenderDriverInfo(i, &renderer_info) < 0) {
				E_Exit("MAPPER: Failed to retrieve SDL renderer driver info: %s",
				       SDL_GetError());
			}
			assert(renderer_info.name);
			if (strcmp(renderer_info.name, "opengl") == 0) {
				renderer_driver_index = i;
				break;
			}
		}
		if (renderer_driver_index == -1) {
			E_Exit("MAPPER: OpenGL support in SDL renderer is unavailable but required for OpenGL output");
		}
		constexpr uint32_t renderer_flags = 0;
		mapper.renderer = SDL_CreateRenderer(mapper.window,
		                                     renderer_driver_index,
		                                     renderer_flags);
	}
#endif
	if (mapper.renderer == nullptr) {
		E_Exit("MAPPER: Could not retrieve window renderer: %s",
		       SDL_GetError());
	}

	if (SDL_RenderSetLogicalSize(mapper.renderer, 640, 480) < 0) {
		LOG_WARNING("MAPPER: Failed to set renderer logical size: %s",
		            SDL_GetError());
	}

	// Create font atlas surface
	SDL_Surface* atlas_surface = SDL_CreateRGBSurfaceFrom(
	        int10_font_14, 8, 256 * 14, 1, 1, 0, 0, 0, 0);
	if (atlas_surface == nullptr) {
		E_Exit("MAPPER: Failed to create atlas surface: %s", SDL_GetError());
	}

	// Invert default surface palette
	const SDL_Color atlas_colors[2] = {{0x00, 0x00, 0x00, 0x00},
	                                   {0xff, 0xff, 0xff, 0xff}};
	if (SDL_SetPaletteColors(atlas_surface->format->palette, atlas_colors, 0, 2) <
	    0) {
		LOG_WARNING("MAPPER: Failed to set colors in font atlas: %s",
		            SDL_GetError());
	}

	// Convert surface to texture for accelerated SDL renderer
	mapper.font_atlas = SDL_CreateTextureFromSurface(mapper.renderer,
	                                                 atlas_surface);
	SDL_FreeSurface(atlas_surface);
	atlas_surface = nullptr;
	if (mapper.font_atlas == nullptr) {
		E_Exit("MAPPER: Failed to create font texture atlas: %s",
		       SDL_GetError());
	}

	if (last_clicked) {
		last_clicked->BindColor();
		last_clicked=nullptr;
	}
	/* Go in the event loop */
	mapper.exit = false;
	mapper.redraw=true;
	SetActiveEvent(nullptr);
#if defined (REDUCE_JOYSTICK_POLLING)
	SDL_JoystickEventState(SDL_ENABLE);
#endif
	while (!mapper.exit) {
		if (mapper.redraw) {
			mapper.redraw = false;
			DrawButtons();
		}
		BIND_MappingEvents();
		Delay(1);
	}
	/* ONE SHOULD NOT FORGET TO DO THIS!
	Unless a memory leak is desired... */
	SDL_DestroyTexture(mapper.font_atlas);
	SDL_RenderSetLogicalSize(mapper.renderer, 0, 0);
	SDL_SetRenderDrawColor(mapper.renderer,
	                       color_black.red,
	                       color_black.green,
	                       color_black.blue,
	                       SDL_ALPHA_OPAQUE);
#if C_OPENGL
	if (context) {
#if SDL_VERSION_ATLEAST(2, 0, 10)
		if (SDL_RenderFlush(mapper.renderer) < 0) {
			LOG_WARNING("MAPPER: Failed to flush pending renderer commands: %s",
			            SDL_GetError());
		}
#endif
		SDL_DestroyRenderer(mapper.renderer);
		if (SDL_GL_MakeCurrent(mapper.window, context) < 0) {
			LOG_ERR("MAPPER: Failed to restore OpenGL context: %s",
			        SDL_GetError());
		}
	}
#endif
#if defined (REDUCE_JOYSTICK_POLLING)
	SDL_JoystickEventState(SDL_DISABLE);
#endif
	GFX_ResetScreen();
	MOUSE_NotifyTakeOver(false);
}
