#ifndef VIRT_JOYSTICK_H
#define VIRT_JOYSTICK_H

constexpr int MAX_VJOY_BUTTONS{8};
constexpr int MAX_VJOY_HAT{16};
constexpr int MAX_VJOY_AXIS{8};

struct VirtJoystick
{
	int16_t axis_pos[MAX_VJOY_AXIS] = {0};
	bool hat_pressed[MAX_VJOY_HAT] = {false};
	bool button_pressed[MAX_VJOY_BUTTONS] = {false};
};

#endif