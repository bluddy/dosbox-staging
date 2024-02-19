#include "bind.h"

CBind::CBind(CBindList *binds)
    : list(binds)
{
    list->push_back(this);
    event=nullptr;
    all_binds.push_back(this);
}
	

void CBind::AddFlags(char * buf) {
    if (mods & BMOD_Mod1) strcat(buf," mod1");
    if (mods & BMOD_Mod2) strcat(buf," mod2");
    if (mods & BMOD_Mod3) strcat(buf," mod3");
    if (flags & BFLG_Hold) strcat(buf," hold");
}

void CBind::SetFlags(char *buf)
{
    char *word;
    while (*(word = strip_word(buf))) {
        if (!strcasecmp(word, "mod1"))
            mods |= BMOD_Mod1;
        if (!strcasecmp(word, "mod2"))
            mods |= BMOD_Mod2;
        if (!strcasecmp(word, "mod3"))
            mods |= BMOD_Mod3;
        if (!strcasecmp(word, "hold"))
            flags |= BFLG_Hold;
    }
}

void CBind::ActivateBind(Bits _value,bool ev_trigger,bool skip_action=false) {
    if (event->IsTrigger()) {
        /* use value-boundary for on/off events */
        if (_value>25000) {
            event->SetValue(_value);
            if (active) return;
            event->ActivateEvent(ev_trigger,skip_action);
            active=true;
        } else {
            if (active) {
                event->DeActivateEvent(ev_trigger);
                active=false;
            }
        }
    } else {
        /* store value for possible later use in the activated event */
        event->SetValue(_value);
        event->ActivateEvent(ev_trigger,false);
    }
}

void CBind::DeActivateBind(bool ev_trigger) {
    if (event->IsTrigger()) {
        if (!active) return;
        active=false;
        if (flags & BFLG_Hold) {
            if (!holding) {
                holdlist.push_back(this);
                holding=true;
                return;
            } else {
                holdlist.remove(this);
                holding=false;
            }
        }
        event->DeActivateEvent(ev_trigger);
    } else {
        /* store value for possible later use in the activated event */
        event->SetValue(0);
        event->DeActivateEvent(ev_trigger);
    }
}

std::string CKeyBind::GetBindName() const
{
    // Always map Return to Enter
    if (key == SDL_SCANCODE_RETURN) {
        return "Enter";
    }

    const std::string sdl_scancode_name = SDL_GetScancodeName(key);
    if (!sdl_scancode_name.empty()) {
        return sdl_scancode_name;
    }

    // SDL Doesn't have a name for this key, so use our own
    assert(sdl_scancode_name.empty());

    // Key between Left Shift and Z is "oem102"
    if (key == SDL_SCANCODE_NONUSBACKSLASH) {
        return "oem102"; // called 'OEM_102" at kbdlayout.info
    }
    // Key to the left of Right Shift on ABNT layouts
    if (key == SDL_SCANCODE_INTERNATIONAL1) {
        return "abnt1"; // called "ABNT_C1" at kbdlayout.info
    }

    LOG_DEBUG("MAPPER: Please report unnamed SDL scancode %d (%xh)",
                key,
                key);

    return sdl_scancode_name;
}

CJHatBind::CJHatBind(CBindList *_list, CBindGroup *_group, uint8_t _hat, uint8_t _dir)
    : CBind(_list),
        group(_group),
        hat(_hat),
        dir(_dir)
{
    // TODO this code allows to bind only a single hat position, but
    // perhaps we should allow 8-way positioning?
    if (dir & SDL_HAT_UP)
        dir = SDL_HAT_UP;
    else if (dir & SDL_HAT_RIGHT)
        dir=SDL_HAT_RIGHT;
    else if (dir & SDL_HAT_DOWN)
        dir=SDL_HAT_DOWN;
    else if (dir & SDL_HAT_LEFT)
        dir=SDL_HAT_LEFT;
    else
        E_Exit("MAPPER:JOYSTICK:Invalid hat position");
}

void CJHatBind::ConfigName(char *buf)
{
    sprintf(buf,"%s hat %" PRIu8 " %" PRIu8,
            group->ConfigStart(), hat, dir);
}

std::string CJHatBind::GetBindName() const
{
    char buf[30];
    safe_sprintf(buf, "%s Hat %" PRIu8 " %s", group->BindStart(), hat,
                    ((dir == SDL_HAT_UP)      ? "up"
                    : (dir == SDL_HAT_RIGHT) ? "right"
                    : (dir == SDL_HAT_DOWN)  ? "down"
                                            : "left"));
    return buf;
}
