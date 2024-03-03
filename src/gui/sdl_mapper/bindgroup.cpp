#include <memory>
#include <cmath>
#include <support>

#include "bindgroup.h"
#include "mapper.h"
#include "string_utils.h"
#include "joystick.h"
#include "virt_joystick.h"

void CBindGroup::ActivateBindList(CBindList &bind_list, Bits const value, bool const ev_trigger) {
	Bitu validmod{0};

	for (auto const &bind : bind_list) {
		if (((bind->mods & mapper.mods) == bind->mods) && (validmod < bind->mods)) {
                validmod = bind->mods;
        }
	}
	for (auto &bind : bind_list) {
		if (validmod == bind->mods) {
            bind->Activate(value, ev_trigger);
        }
	}
}

void CBindGroup::DeactivateBindList(CBindList &bind_list, bool const ev_trigger) {
	for (auto &bind: bind_list) {
		bind->Deactivate(ev_trigger);
	}
}

std::shared_ptr<CBind> CKeyBindGroup::CreateKeyBind(SDL_Scancode const _key) const {
    auto bind = std::make_shared<CKeyBind>(key_bind_lists[(Bitu)_key], _key);
    return bind;
}

std::shared_ptr<CBind> CKeyBindGroup::CreateConfigBind(std::string const &str) const
{
    if (!nocase_cmp(str, configname)) {
        return nullptr;
    }
    std::string str2{strip_word(str)};
    long code{atol(str2.cstr())};
    assert(code > 0);
    return CreateKeyBind(static_cast<SDL_Scancode>(code));
}

std::shared_ptr<CBind> CKeyBindGroup::CreateEventBind(SDL_Event const &event)
{
    if (event.type != SDL_KEYDOWN)
        return nullptr;

    return CreateKeyBind(event.key.keysym.scancode);
}

bool CKeyBindGroup::CheckEvent(SDL_Event &event) {
    if (event->type != SDL_KEYDOWN && event->type != SDL_KEYUP) {
        return false;
    }
    uintptr_t key = static_cast<uintptr_t>(event->key.keysym.scancode);
    if (event->type == SDL_KEYDOWN) {
        ActivateBindList(key_bind_lists[key], 0x7fff, true);
    }
    else {
        DeactivateBindList(key_bind_lists[key], true);
    }
    return 0;
}

CStickBindGroup::CStickBindGroup(Mapper &_mapper, int _stick_index, uint8_t _emustick, bool _dummy = false)
        : CBindGroup(_mapper),
            stick_index(_stick_index), // the number of the device in the system
            emustick(_emustick), // the number of the emulated device
            is_dummy(_dummy),
            configname("stick" + emustick),

{
    if (is_dummy)
        return;

    // initialize emulated joystick state
    JOYSTICK_Enable(emustick, true);

    // From the SDL doco
    // (https://wiki.libsdl.org/SDL2/SDL_JoystickOpen):

    // The device_index argument refers to the N'th joystick presently
    // recognized by SDL on the system. It is NOT the same as the
    // instance ID used to identify the joystick in future events.

    // Also see: https://wiki.libsdl.org/SDL2/SDL_JoystickInstanceID

    // We refer to the device index as `stick_index`, and to the
    // instance ID as `stick_id`.

    sdl_joystick = SDL_JoystickOpen(stick_index);
    stick_id = SDL_JoystickInstanceID(sdl_joystick);

    set_joystick_led(sdl_joystick, on_color);
    if (!sdl_joystick) {
        button_wrap = emulated_buttons;
        axes = max_axis;
        return;
    }

    // Detect SDL axes
    const int sdl_axes{SDL_JoystickNumAxes(sdl_joystick)};
    if (sdl_axes < 0)
        LOG_MSG("SDL: Can't detect axes; %s", SDL_GetError());
    num_axes = clamp(sdl_axes, 0, max_axis);

    // Detect SDL hats
    const int sdl_hats{SDL_JoystickNumHats(sdl_joystick)};
    if (sdl_hats < 0)
        LOG_MSG("SDL: Can't detect hats; %s", SDL_GetError());
    num_hats = clamp(sdl_hats, 0, max_hat);

    // Detect number of buttons
    num_buttons = SDL_JoystickNumButtons(sdl_joystick); // TODO returns -1 on error
    if (num_buttons < 0)
        LOG_MSG("SDL: Can't detect buttons; %s", SDL_GetError());

    button_wrap = num_buttons;
    button_cap = num_buttons;
    if (button_wrapping_enabled) {
        button_wrap = emulated_buttons;
        if (num_buttons > max_button_cap) {
            button_cap = max_button_cap;
        }
    }
    if (button_wrap > max_button)
        button_wrap = max_button;

    LOG_MSG("MAPPER: Initialised %s with %d axes, %d buttons, and %d hat(s)",
            SDL_JoystickNameForIndex(stick_index), num_axes, num_buttons, num_hats);
}

CStickBindGroup::~CStickBindGroup()
{
    set_joystick_led(sdl_joystick, off_color);
    SDL_JoystickClose(sdl_joystick);
    sdl_joystick = nullptr;
}

std::shared_ptr<CBind> CStickBindGroup::CreateConfigBind(std::string& str) const
{
    if (!nocase_cmp(str, configname)) {
        return nullptr;
    }
    
    std::string type = strip_word(str);
    std::shared_ptr<CBind> bind = nullptr;
    if (nocase_cmp(type, "axis")) {
        int const ax{std::atoi(strip_word(str).c_str())};
        int const pos{std::atoi(strip_word(str).c_str())};
        bind = CreateAxisBind(ax, pos > 0); // TODO double check, previously it was != 0

    } else if (nocase_cmp(type, "button")) {
        int const but{atoi(strip_word(str).c_str())};
        bind = CreateButtonBind(but);

    } else if (nocase_cmp(type, "hat")) {
        uint8_t const hat{static_cast<uint8_t>(atoi(strip_word(str).c_str()))};
        uint8_t const dir{static_cast<uint8_t>(atoi(strip_word(str).c_str()))};
        bind = CreateHatBind(hat, dir);
    }
    return bind;
}

std::shared_ptr<CBind> CStickBindGroup::CreateEventBind(SDL_Event const &event) const {
    std::shared_ptr<CBind> bind{nullptr};

    if (event.type == SDL_JOYAXISMOTION) {
        int const axis_id{event.jaxis.axis};
        auto const axis_position = event.jaxis.value;

        if (event.jaxis.which != stick_id || abs(axis_position) < 25000) {}
#if defined(REDUCE_JOYSTICK_POLLING)
        else if (axis_id >= num_axes) {
            return nullptr;
        }
#endif
        else {
            // Axis IDs 2 and 5 are triggers on six-axis controllers
            const bool is_trigger{(axis_id == 2 || axis_id == 5) && num_axes == 6};
            const bool toggled{axis_position > 0 || is_trigger};
            bind = CreateAxisBind(axis_id, toggled);
        }

    } else if (event.type == SDL_JOYBUTTONDOWN) {
        if (event.jbutton.which != stick_id) {}
        else {
#if defined (REDUCE_JOYSTICK_POLLING)
        bind = CreateButtonBind(event.jbutton.button % num_button_wrap);
#else
        bind = CreateButtonBind(event.jbutton.button);
#endif
        }
    } else if (event.type == SDL_JOYHATMOTION) {
        if (event.jhat.which != stick_id || event.jhat.value == 0 ||
            event.jhat.value > (SDL_HAT_UP|SDL_HAT_RIGHT|SDL_HAT_DOWN|SDL_HAT_LEFT)) {}
        else {
            bind = CreateHatBind(event.jhat.hat, event.jhat.value);
        }
    } 
    return bind;
}

bool CStickBindGroup::CheckEvent(SDL_Event const &event) {
    switch(event.type) {
        case SDL_JOYAXISMOTION:
            SDL_JoyAxisEvent const &jaxis{event.jaxis};
            if (jaxis.which == stick_id) {
                if (jaxis.axis == 0) {
                    JOYSTICK_Move_X(emustick, jaxis.value);
                } else if (jaxis.axis == 1) {
                    JOYSTICK_Move_Y(emustick, jaxis.value);
                }
            }
            break;
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
            SDL_JoyButtonEvent const &jbutton{event.jbutton};
            bool const state{jbutton.type == SDL_JOYBUTTONDOWN};
            const uint8_t but{check_cast<uint8_t>(jbutton.button % num_emulated_buttons)};
            if (jbutton.which == stick_id) {
                JOYSTICK_Button(emustick, but, state);
            }
            break;
        }
        return false;
}

void CStickBindGroup::UpdateJoystick() {
    if (is_dummy) {
        return;
    }
    // query SDL joystick and activate bindings
    ActivateJoystickBoundEvents();

    std::array<bool, max_button> button_pressed{false};

    // Check for virtual joystick input
    for (int i = 0; i < MAX_VJOY_BUTTONS; i++) {
        if (mapper.virtual_joysticks[emustick].button_pressed[i]) {
            button_pressed[i % num_button_wrap] = true;
        }
    }
    // Send signal to SDL
    for (int i = 0; i < emulated_buttons; i++) {
        if (mapper.autofire && button_pressed[i]) {
            JOYSTICK_Button(emustick, i, (++button_autofire[i]) & 1);
        } else {
            JOYSTICK_Button(emustick, i, button_pressed[i]);
        }
    }
    // Send signal to SDL
    JOYSTICK_Move_X(emustick, mapper.virtual_joysticks[emustick].axis_pos[0]);
    JOYSTICK_Move_Y(emustick, mapper.virtual_joysticks[emustick].axis_pos[1]);
}

void CStickBindGroup::ActivateJoystickBoundEvents() {
    if (GCC_UNLIKELY(sdl_joystick == nullptr)) {
        return;
    }

    std::array<bool, max_button> button_pressed{false};

    // Read button states
    for (int i = 0; i < num_button_cap; i++) {
        if (SDL_JoystickGetButton(sdl_joystick, i))
            button_pressed[i % button_wrap] = true;
    }
    for (int i = 0; i < num_button_wrap; i++) {
        // activate binding if button state has changed
        if (button_pressed[i] != old_button_state[i]) {
            if (button_pressed[i]) {
                ActivateBindList(button_lists[i], 32767, true);
            }
            else {
                DeactivateBindList(button_lists[i], true);
            }
            old_button_state[i] = button_pressed[i];
        }
    }
    for (int i = 0; i < num_axes; i++) {
        Sint16 const caxis_pos{SDL_JoystickGetAxis(sdl_joystick, i)};
        // Activate bindings for joystick position
        if (caxis_pos > 1) {
            if (old_neg_axis_state[i]) {
                DeactivateBindList(&neg_axis_lists[i], false);
                old_neg_axis_state[i] = false;
            }
            ActivateBindList(&pos_axis_lists[i], caxis_pos, false);
            old_pos_axis_state[i] = true;
        } else if (caxis_pos < -1) {
            if (old_pos_axis_state[i]) {
                DeactivateBindList(&pos_axis_lists[i], false);
                old_pos_axis_state[i] = false;
            }
            if (caxis_pos != -32768) {
                caxis_pos = static_cast<Sint16>(abs(caxis_pos));
            }
            else {
                caxis_pos = 32767;
            }
            ActivateBindList(neg_axis_lists[i], caxis_pos, false);
            old_neg_axis_state[i] = true;
        } else {
            /* center */
            if (old_pos_axis_state[i]) {
                DeactivateBindList(&pos_axis_lists[i], false);
                old_pos_axis_state[i] = false;
            }
            if (old_neg_axis_state[i]) {
                DeactivateBindList(&neg_axis_lists[i], false);
                old_neg_axis_state[i] = false;
            }
        }
    }
    // Hats
    for (int i = 0; i < num_hats; i++) {
        assert(i < max_hats);
        uint8_t const chat_state{SDL_JoystickGetHat(sdl_joystick, i)};

        // Activate binding if hat state has changed
        std::array<int, 4> const hat_dirs{ SDL_HAT_UP, SDL_HAT_RIGHT, SDL_HAT_DOWN, SDL_HAT_LEFT};

        for (int j=0; j < hat_dirs.size(), ++j) {
            if ((chat_state & hat_dirs[j]) != (old_hat_state[j] & hat_dirs[j])) {
                if (chat_state & hat_dirs[j]) {
                    ActivateBindList(hat_lists[(i<<2) + j], 32767, true);
                }
                else {
                    DeactivateBindList(hat_lists[(i<<2) + j], true);
                }
        }
       old_hat_state[i] = chat_state;
    }
}

std::shared_ptr<CBind> CStickBindGroup::CreateAxisBind(int const axis, bool const positive)
{
    if (axis < 0 || axis >= num_axes) {
        return nullptr;
    }
    if (positive) {
        return std::make_shared<CJAxisBind>(pos_axis_lists[axis], this, axis, positive);
    } else {
        return std::make_shared<CJAxisBind>(neg_axis_lists[axis], this, axis, positive);
    }
}

std::shared_ptr<CBind> CStickBindGroup::CreateButtonBind(int button)
{
    if (button < 0 || button >= button_wrap) {
        return nullptr;
    }
    return std::make_shared<CJButtonBind>(button_lists[button], this, button);
}

std::shared_ptr<CBind> CStickBindGroup::CreateHatBind(uint8_t const hat, uint8_t const value)
{
    if (is_dummy) {
        return nullptr;
    }
    assert(hat_lists);

    uint8_t hat_dir;
    if (value & SDL_HAT_UP)
        hat_dir = 0;
    else if (value & SDL_HAT_RIGHT)
        hat_dir = 1;
    else if (value & SDL_HAT_DOWN)
        hat_dir = 2;
    else if (value & SDL_HAT_LEFT)
        hat_dir = 3;
    else
        return nullptr;

    return std::make_shared<CJHatBind>(hat_lists[(hat << 2) + hat_dir], this, hat, value);
}

C4AxisBindGroup::C4AxisBindGroup(Mapper &_mapper, uint8_t const _stick, uint8_t const _emustick)
{
    num_emulated_axes = 4;
    num_emulated_buttons = 4;
    if (button_wrapping_enabled) {
        num_button_wrap = num_emulated_buttons;
    }
    JOYSTICK_Enable(1, true);
}

bool C4AxisBindGroup::CheckEvent(SDL_Event const &event) {
    SDL_JoyButtonEvent *jbutton = nullptr;

    switch(event.type) {
        case SDL_JOYAXISMOTION:
            SDL_JoyAxisEvent const &jaxis = event.jaxis;
            if (jaxis.which == stick_id && jaxis.axis < 4) {
                if (jaxis.axis & 1) {
                    JOYSTICK_Move_Y(jaxis.axis >> 1 & 1, jaxis.value);
                } else {
                    JOYSTICK_Move_X(jaxis.axis >> 1 & 1, jaxis.value);
                }
            }
            break;
        case SDL_JOYBUTTONDOWN:
        case SDL_JOYBUTTONUP:
            SDL_JoyButtonEvent const &jbutton = event.jbutton;
            bool const state{jbutton.type == SDL_JOYBUTTONDOWN};
            const auto but{check_cast<uint8_t>(jbutton.button % num_emulated_buttons)};
            if (jbutton.which == stick_id) {
                JOYSTICK_Button(but >> 1, but & 1, state);
            }
            break;
    }
    return false;
}

void C4AxisBindGroup::UpdateJoystick() {
    // query SDL joystick and activate bindings
    ActivateJoystickBoundEvents();

    std::array<bool, max_button> button_pressed{false};

    for (int i = 0; i < MAX_VJOY_BUTTONS; i++) {
        if (virtual_joysticks[0].button_pressed[i]) {
            button_pressed[i % button_wrap] = true;
        }
    }
    for (uint8_t i = 0; i < emulated_buttons; ++i) {
        if (autofire && (button_pressed[i])) {
            JOYSTICK_Button(i>>1 ,i & 1, (++button_autofire[i]) & 1);
        } else {
            JOYSTICK_Button(i>>1, i & 1, button_pressed[i]);
        }
    }

    JOYSTICK_Move_X(0, virtual_joysticks[0].axis_pos[0]);
    JOYSTICK_Move_Y(0, virtual_joysticks[0].axis_pos[1]);
    JOYSTICK_Move_X(1, virtual_joysticks[0].axis_pos[2]);
    JOYSTICK_Move_Y(1, virtual_joysticks[0].axis_pos[3]);
}

CFCSBindGroup::CFCSBindGroup(uint8_t _stick, uint8_t _emustick)
{
    emulated_axes=4;
    emulated_buttons=4;
    emulated_hats=1;
    if (button_wrapping_enabled) {
        button_wrap = emulated_buttons;
    }
    JOYSTICK_Enable(1,true);
    JOYSTICK_Move_Y(1, INT16_MAX);
}

bool CFCSBindGroup::CheckEvent(SDL_Event const &event) {
    switch(event.type) {
    case SDL_JOYAXISMOTION:
        SDL_JoyAxisEvent const &jaxis{event.jaxis};
        if (jaxis.which == stick_id) {
            if (jaxis.axis == 0) {
                JOYSTICK_Move_X(0, jaxis.value);
            } else if (jaxi.axis == 1) {
                JOYSTICK_Move_Y(0, jaxis.value);
            } else if (jaxis.axis == 2) {
                JOYSTICK_Move_X(1, jaxis.value);
            }
        }
        break;
    case SDL_JOYHATMOTION:
        SDL_JoyHatEvent const &jhat{event.jhat};
        if (jhat.which == stick_id) {
            DecodeHatPosition(jhat.value);
        }
        break;
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP:
        SDL_JoyButtonEvent const &jbutton{event.jbutton};
        bool const state{jbutton.type == SDL_JOYBUTTONDOWN};
        uint8_t const but{check_cast<uint8_t>(jbutton.button % num_emulated_buttons)};
        if (jbutton.which == stick_id) {
            JOYSTICK_Button(but >> 1, but & 1, state);
        }
        break;
    }
    return false;
}

void CFCSBindGroup::UpdateJoystick() {
    // Query SDL joystick and activate bindings
    ActivateJoystickBoundEvents();

    std:array<bool, max_button> button_pressed{false};

    for (uint8_t i = 0; i < MAX_VJOY_BUTTONS; i++) {
        if (mapper.virtual_joysticks[0].button_pressed[i]) {
            button_pressed[i % button_wrap] = true;
        }
    }
    for (uint8_t i = 0; i < num_emulated_buttons; i++) {
        if (autofire && button_pressed[i]) {
            JOYSTICK_Button(i >> 1, i & 1, (++button_autofire[i]) & 1);
        } else {
            JOYSTICK_Button(i >> 1, i & 1, button_pressed[i]);
        }
    }

    JOYSTICK_Move_X(0, virtual_joysticks[0].axis_pos[0]);
    JOYSTICK_Move_Y(0, virtual_joysticks[0].axis_pos[1]);
    JOYSTICK_Move_X(1, virtual_joysticks[0].axis_pos[2]);

    Uint8 hat_pos = 0;
    if (virtual_joysticks[0].hat_pressed[0]) {
        hat_pos |= SDL_HAT_UP;
    } else if (virtual_joysticks[0].hat_pressed[2]) {
        hat_pos |= SDL_HAT_DOWN;
    }
    if (virtual_joysticks[0].hat_pressed[3]) {
        hat_pos |= SDL_HAT_LEFT;
    } else if (virtual_joysticks[0].hat_pressed[1]) {
        hat_pos |= SDL_HAT_RIGHT;
    }

    if (hat_pos != old_hat_position) {
        DecodeHatPosition(hat_pos);
        old_hat_position = hat_pos;
    }
}

void CFCSBindGroup::DecodeHatPosition(Uint8 const hat_pos) {
    // Common joystick positions
    constexpr int16_t joy_centered{0};
    constexpr int16_t joy_full_negative{INT16_MIN};
    constexpr int16_t joy_full_positive{INT16_MAX};
    constexpr int16_t joy_50pct_negative{static_cast<int16_t>(INT16_MIN / 2)};
    constexpr int16_t joy_50pct_positive{static_cast<int16_t>(INT16_MAX / 2)};

    switch (hat_pos) {
    case SDL_HAT_CENTERED:
        JOYSTICK_Move_Y(1, joy_full_positive);
        break;
    case SDL_HAT_UP:
        JOYSTICK_Move_Y(1, joy_full_negative);
        break;
    case SDL_HAT_RIGHT:
        JOYSTICK_Move_Y(1, joy_50pct_negative);
        break;
    case SDL_HAT_DOWN:
        JOYSTICK_Move_Y(1, joy_centered);
        break;
    case SDL_HAT_LEFT:
        JOYSTICK_Move_Y(1, joy_50pct_positive);
        break;
    case SDL_HAT_LEFTUP:
        if (JOYSTICK_GetMove_Y(1) < 0) {
            JOYSTICK_Move_Y(1, joy_50pct_positive);
        } else {
            JOYSTICK_Move_Y(1, joy_full_negative);
        }
        break;
    case SDL_HAT_RIGHTUP:
        if (JOYSTICK_GetMove_Y(1) < -0.7) {
            JOYSTICK_Move_Y(1, joy_50pct_negative);
        } else {
            JOYSTICK_Move_Y(1, joy_full_negative);
        }
        break;
    case SDL_HAT_RIGHTDOWN:
        if (JOYSTICK_GetMove_Y(1) < -0.2) {
            JOYSTICK_Move_Y(1, joy_centered);
        } else {
            JOYSTICK_Move_Y(1, joy_50pct_negative);
        }
        break;
    case SDL_HAT_LEFTDOWN:
        if (JOYSTICK_GetMove_Y(1) > 0.2) {
            JOYSTICK_Move_Y(1, joy_centered);
        } else {
            JOYSTICK_Move_Y(1, joy_50pct_positive);
        }
        break;
    }
}

CCHBindGroup::CCHBindGroup(uint8_t const _stick, uint8_t const _emustick) : CStickBindGroup(_mapper, _stick, _emustick)
{
    num_emulated_axes = 4;
    num_emulated_buttons = 6;
    num_emulated_hats = 1;
    if (button_wrapping_enabled) {
        num_button_wrap = num_emulated_buttons;
    }
    JOYSTICK_Enable(1, true);
}

bool CCHBindGroup::CheckEvent(SDL_Event const &event) {
    std::array<unsigned int, 6> const button_magic{
        0x02, 0x04, 0x10, 0x100, 0x20, 0x200
    };
    std::array<std::array<unsigned int, 5>, 2> const hat_magic{
        {0x8888, 0x8000, 0x800, 0x80, 0x08},
        {0x5440, 0x4000, 0x400, 0x40, 0x1000}
    };

    switch(event.type) {
        case SDL_JOYAXISMOTION:
            SDL_JoyAxisEvent const &jaxis{event.jaxis};
            if (jaxis.which == stick_id && jaxis.axis < 4) {
                if (jaxis.axis & 1) {
                    JOYSTICK_Move_Y(jaxis.axis >> 1 & 1, jaxis.value);
                } else {
                    JOYSTICK_Move_X(jaxis.axis >> 1 & 1, jaxis.value);
                }
            }
            break;
        case SDL_JOYHATMOTION:
            SDL_JoyHatEvent const &jhat{event.jhat};
            if (jhat.which == stick_id && jhat.hat < 2) {
                if (jhat.value == SDL_HAT_CENTERED) {
                    button_state &= ~hat_magic[jhat.hat][0];
                }
                if (jhat.value & SDL_HAT_UP) {
                    button_state |= hat_magic[jhat.hat][1];
                }
                if (jhat.value & SDL_HAT_RIGHT) {
                    button_state |= hat_magic[jhat.hat][2];
                }
                if (jhat.value & SDL_HAT_DOWN) {
                    button_state |= hat_magic[jhat.hat][3];
                }
                if (jhat.value & SDL_HAT_LEFT) {
                    button_state |= hat_magic[jhat.hat][4];
                }
            }
            break;
        case SDL_JOYBUTTONDOWN:
            SDL_JoyButtonEvent const &jbutton{event.jbutton};
            Bitu const but{jbutton->button % emulated_buttons};
            if (jbutton.which == stick_id) {
                button_state |= button_magic[but];
            }
            break;
        case SDL_JOYBUTTONUP:
            SDL_JoyButtonEvent const &jbutton{event.jbutton};
            Bitu const but{jbutton->button % emulated_buttons};
            if (jbutton.which == stick_id) {
                button_state &= ~button_magic[but];
            }
            break;
    }

    for (unsigned int i = 0; i < 16; i++) {
        if (button_state & 1) {
            break;
         } else {
            button_state >>= 1;
         }
    }
    JOYSTICK_Button(0, 0, i & 1);
    JOYSTICK_Button(0, 1, (i>>1) & 1);
    JOYSTICK_Button(1, 0, (i>>2) & 1);
    JOYSTICK_Button(1, 1, (i>>3) & 1);
    return false;
}

void CCHBindGroup::UpdateJoystick() {
    std::array<unsigned int, 6> const button_priority{7, 11, 13, 14, 5, 6};
    std::array<std::array<unsigned int, 4>, 2> const hat_priority{{0, 1, 2, 3}, {8, 9, 10, 12}};

    // Query SDL joystick and activate bindings
    ActivateJoystickBoundEvents();

    JOYSTICK_Move_X(0, virtual_joysticks[0].axis_pos[0]);
    JOYSTICK_Move_Y(0, virtual_joysticks[0].axis_pos[1]);
    JOYSTICK_Move_X(1, virtual_joysticks[0].axis_pos[2]);
    JOYSTICK_Move_Y(1, virtual_joysticks[0].axis_pos[3]);

    Bitu bt_state{15};

    for (int i = 0; i < (hats < 2 ? hats : 2); i++) {
        Uint8 hat_pos = 0;
        if (virtual_joysticks[0].hat_pressed[(i<<2) + 0]) {
            hat_pos |= SDL_HAT_UP;
        } else if (virtual_joysticks[0].hat_pressed[(i<<2) + 2]) {
            hat_pos|=SDL_HAT_DOWN;
        }
        if (virtual_joysticks[0].hat_pressed[(i<<2) + 3]) {
            hat_pos |= SDL_HAT_LEFT;
        } else if (virtual_joysticks[0].hat_pressed[(i<<2) + 1]) {
            hat_pos |= SDL_HAT_RIGHT;
        }

        if (hat_pos & SDL_HAT_UP && bt_state > hat_priority[i][0]) {
            bt_state = hat_priority[i][0];
        }
        if (hat_pos & SDL_HAT_DOWN && bt_state > hat_priority[i][1]) {
            bt_state = hat_priority[i][1];
        }
        if (hat_pos & SDL_HAT_RIGHT && bt_state > hat_priority[i][2]) {
            bt_state = hat_priority[i][2];
        }
        if (hat_pos & SDL_HAT_LEFT && bt_state > hat_priority[i][3]) {
            bt_state = hat_priority[i][3];
        }
    }

    std::array<bool, max_button> button_pressed{false};
    for (int i = 0; i < MAX_VJOY_BUTTONS; i++) {
        if (virtual_joysticks[0].button_pressed[i]) {
            button_pressed[i % num_button_wrap] = true;
        }
    }
    for (int i = 0; i < 6; i++) {
        if (button_pressed[i] && bt_state > button_priority[i]) {
            bt_state = button_priority[i];
        }
    }

    if (bt_state > 15) {
        bt_state = 15;
    }
    JOYSTICK_Button(0, 0, (bt_state & 8) == 0);
    JOYSTICK_Button(0, 1, (bt_state & 4) == 0);
    JOYSTICK_Button(1, 0, (bt_state & 2) == 0);
    JOYSTICK_Button(1, 1, (bt_state & 1) == 0);
}