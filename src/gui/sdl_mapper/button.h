#ifndef SDL_MAPPER_BUTTON_H
#define SDL_MAPPER_BUTTON_H

class CButton {
public:
	CButton(int32_t p_x, int32_t p_y, int32_t p_dx, int32_t p_dy)
	        : rect{p_x, p_y, p_dx, p_dy},
	          color(color_white),
	          enabled(true)
	{
		buttons.emplace_back(this);
	}

	virtual ~CButton() = default;

	virtual void Draw() {
		if (!enabled)
			return;
		SDL_SetRenderDrawColor(mapper.renderer,
		                       color.red,
		                       color.green,
		                       color.blue,
		                       SDL_ALPHA_OPAQUE);
		SDL_RenderDrawRect(mapper.renderer, &rect);
	}

	virtual bool OnTop(int32_t _x, int32_t _y)
	{
		return (enabled && (_x >= rect.x) && (_x < rect.x + rect.w) &&
		        (_y >= rect.y) && (_y < rect.y + rect.h));
	}

	virtual void BindColor() {}
	virtual void Click() {}

	void Enable(bool yes)
	{
		enabled = yes;
		mapper.redraw = true;
	}

	void SetColor(const Rgb888& _col)
	{
		color = _col;
	}

protected:
	SDL_Rect rect;
	Rgb888 color;
	bool enabled;
};

// Inform PVS-Studio that new CTextButtons won't be leaked when they go out of scope
//+V773:SUPPRESS, class:CTextButton
class CTextButton : public CButton {
public:
	CTextButton(int32_t x, int32_t y, int32_t dx, int32_t dy, const char* txt)
	        : CButton(x, y, dx, dy),
	          text(txt)
	{}

	CTextButton(const CTextButton&) = delete; // prevent copy
	CTextButton& operator=(const CTextButton&) = delete; // prevent assignment

	void Draw() override
	{
		if (!enabled)
			return;
		CButton::Draw();
		DrawText(rect.x + 2, rect.y + 2, text.c_str(), color);
	}

	void SetText(const std::string &txt) { text = txt; }

protected:
	std::string text;
};

class CClickableTextButton : public CTextButton {
public:
	CClickableTextButton(int32_t _x, int32_t _y, int32_t _dx, int32_t _dy,
	                     const char* _text)
	        : CTextButton(_x, _y, _dx, _dy, _text)
	{}

	void BindColor() override
	{
		this->SetColor(color_white);
	}
};

class CEventButton;
static CEventButton * last_clicked = nullptr;

class CEventButton final : public CClickableTextButton {
public:
	CEventButton(int32_t x, int32_t y, int32_t dx, int32_t dy,
	             const char* text, CEvent* ev)
	        : CClickableTextButton(x, y, dx, dy, text),
	          event(ev)
	{}

	CEventButton(const CEventButton&) = delete; // prevent copy
	CEventButton& operator=(const CEventButton&) = delete; // prevent assignment

	void BindColor() override {
		this->SetColor(event->bindlist.begin() == event->bindlist.end()
		                       ? color_grey
		                       : color_white);
	}
	void Click() override {
		if (last_clicked) last_clicked->BindColor();
		this->SetColor(color_green);
		SetActiveEvent(event);
		last_clicked=this;
	}
protected:
	CEvent * event = nullptr;
};

class CCaptionButton final : public CButton {
public:
	CCaptionButton(int32_t _x, int32_t _y, int32_t _dx, int32_t _dy)
	        : CButton(_x, _y, _dx, _dy)
	{
		caption[0]=0;
	}
	void Change(const char * format,...) GCC_ATTRIBUTE(__format__(__printf__,2,3));

	void Draw() override {
		if (!enabled) return;
		DrawText(rect.x + 2, rect.y + 2, caption, color);
	}
protected:
	char caption[128] = {};
};

void CCaptionButton::Change(const char * format,...) {
	va_list msg;
	va_start(msg,format);
	vsprintf(caption,format,msg);
	va_end(msg);
	mapper.redraw=true;
}

static void change_action_text(const char* text, const Rgb888& col);

static void MAPPER_SaveBinds();

class CBindButton final : public CClickableTextButton {
public:
	CBindButton(int32_t _x, int32_t _y, int32_t _dx, int32_t _dy,
	            const char* _text, BB_Types _type)
	        : CClickableTextButton(_x, _y, _dx, _dy, _text),
	          type(_type)
	{}

	void Click() override {
		switch (type) {
		case BB_Add:
			mapper.addbind=true;
			SetActiveBind(nullptr);
			change_action_text("Press a key/joystick button or move the joystick.",
			                   color_red);
			break;
		case BB_Del:
			if (mapper.abindit != mapper.aevent->bindlist.end()) {
				auto *active_bind = *mapper.abindit;
				all_binds.remove(active_bind);
				delete active_bind;
				mapper.abindit=mapper.aevent->bindlist.erase(mapper.abindit);
				if (mapper.abindit == mapper.aevent->bindlist.end())
					mapper.abindit=mapper.aevent->bindlist.begin();
			}
			if (mapper.abindit!=mapper.aevent->bindlist.end()) SetActiveBind(*(mapper.abindit));
			else SetActiveBind(nullptr);
			break;
		case BB_Next:
			if (mapper.abindit != mapper.aevent->bindlist.end())
				++mapper.abindit;
			if (mapper.abindit == mapper.aevent->bindlist.end())
				mapper.abindit = mapper.aevent->bindlist.begin();
			SetActiveBind(*(mapper.abindit));
			break;
		case BB_Save:
			MAPPER_SaveBinds();
			break;
		case BB_Exit:
			mapper.exit=true;
			break;
		}
	}
protected:
	BB_Types type;
};

class CCheckButton final : public CClickableTextButton {
public:
	CCheckButton(int32_t x, int32_t y, int32_t dx, int32_t dy,
	             const char* text, BC_Types t)
	        : CClickableTextButton(x, y, dx, dy, text),
	          type(t)
	{}

	void Draw() override {
		if (!enabled) return;
		bool checked=false;
		switch (type) {
		case BC_Mod1:
			checked=(mapper.abind->mods&BMOD_Mod1)>0;
			break;
		case BC_Mod2:
			checked=(mapper.abind->mods&BMOD_Mod2)>0;
			break;
		case BC_Mod3:
			checked=(mapper.abind->mods&BMOD_Mod3)>0;
			break;
		case BC_Hold:
			checked=(mapper.abind->flags&BFLG_Hold)>0;
			break;
		}
		if (checked) {
			const SDL_Rect checkbox_rect = {rect.x + rect.w - rect.h + 2,
			                                rect.y + 2,
			                                rect.h - 4,
			                                rect.h - 4};
			SDL_SetRenderDrawColor(mapper.renderer,
			                       color.red,
			                       color.green,
			                       color.blue,
			                       SDL_ALPHA_OPAQUE);
			SDL_RenderFillRect(mapper.renderer, &checkbox_rect);
		}
		CClickableTextButton::Draw();
	}
	void Click() override {
		switch (type) {
		case BC_Mod1:
			mapper.abind->mods^=BMOD_Mod1;
			break;
		case BC_Mod2:
			mapper.abind->mods^=BMOD_Mod2;
			break;
		case BC_Mod3:
			mapper.abind->mods^=BMOD_Mod3;
			break;
		case BC_Hold:
			mapper.abind->flags^=BFLG_Hold;
			break;
		}
		mapper.redraw=true;
	}
protected:
	BC_Types type;
};

#endif