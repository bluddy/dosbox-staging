#ifndef VIRT_JOYSTICK_H
#define VIRT_JOYSTICK_H

constexpr int MAX_VJOY_BUTTONS{8};
constexpr int MAX_VJOY_HAT{16};
constexpr int MAX_VJOY_AXIS{8};

struct VirtJoystick
{
	std::array<int16_t, MAX_VJOY_AXIS> axis_pos{0};
	std::array<bool, MAX_VJOY_HAT> hat_pressed{false};
	std::array<bool, MAX_VJOY_BUTTONS> button_pressed{false};
};

#endif