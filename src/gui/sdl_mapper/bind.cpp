#include <iostream>
#include <sstream>

#include "bind.h"
#include "mapper.h"

CBind::CBind() {}

constexpr uint32_t BMOD_Mod1{MMOD1};
constexpr uint32_t BMOD_Mod2{MMOD2};
constexpr uint32_t BMOD_Mod3{MMOD3};

constexpr uint32_t BFLG_Hold{0x0001};
constexpr uint32_t BFLG_Repeat{0x0004};

	
std::string CBind::GetFlagsStr() const {
    std::ostringstream oss;
    if (mods & BMOD_Mod1) {
        oss << " mod1";
    }
    if (mods & BMOD_Mod2) {
        oss << " mod2";
    }
    if (mods & BMOD_Mod3) {
        oss << " mod3";
    }
    if (flags & BFLG_Hold) {
        oss << " hold";
    }
    return oss.str();
}

void CBind::SetFlagsFromStr(std::string flag_s)
{
    std::string word;
    do {
        word = strip_word(flag_s);
        lowcase(word);
        if (word == "mod1") {
            mods |= BMOD_Mod1;
        }
        if (word == "mod2") {
            mods |= BMOD_Mod2;
        }
        if (word == "mod3") {
            mods |= BMOD_Mod3;
        }
        if (word == "hold") {
            flags |= BFLG_Hold;
        }
    } while (word != "");
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

std::string CJHatBind::GetConfigName() const
{
    std::ostringstream oss;
    oss << group->ConfigStart() << " hat " << hat << " " << dir;
    return oss.str();
}

std::string CJHatBind::GetBindName() const
{
    std::ostringstream oss;
    std::string dir_s{""};

    switch (dir) {
    case SDL_HAT_UP:
        dir_s = "up";
    case SDL_HAT_RIGHT:
        dir_s = "right";
    case SDL_HAT_LEFT:
        dir_s = "left";
    case SDL_HAT_DOWN:
        dir_s = "down";
    };
    oss << group->BindStart() << " Hat " << hat << " " << dir;
    return oss.str();
}
