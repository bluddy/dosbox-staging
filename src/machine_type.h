// SPDX-FileCopyrightText:  2019-2025 The DOSBox Staging Team
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DOSBOX_MACHINE_TYPE_H
#define DOSBOX_MACHINE_TYPE_H

enum class MachineType {
	// Value not set yet
	None,

	// PC XT with Hercules
	Hercules,

	// PC XT with CGA and monochrome monitor
	CgaMono,
	// PC XT with CGA and color monitor
	CgaColor,
	// IBM PCjr
	Pcjr,
	// Tandy 1000
	Tandy,

	// PC AT with EGA
	Ega,
	// PC AT with VGA or SVGA
	Vga
};

enum class SvgaType {
	// Non-SVGA card
	None,
	// S3 Graphics series (emulated card is mostly Trio64 compatible)
	S3,
	// Tseng Labs ET3000
	TsengEt3k,
	// Tseng Labs ET4000
	TsengEt4k,
	// Paradise Systems PVGA1A
	Paradise
};

extern MachineType machine;
extern SvgaType    svga_type;

inline bool is_machine_svga() {
	return svga_type != SvgaType::None;
}

inline bool is_machine_vga_or_better() {
	return machine == MachineType::Vga;
}

inline bool is_machine_ega() {
	return machine == MachineType::Ega;	
}

inline bool is_machine_ega_or_better() {
	return is_machine_ega() || is_machine_vga_or_better();
}

inline bool is_machine_tandy() {
	return machine == MachineType::Tandy;
}

inline bool is_machine_pcjr() {
	return machine == MachineType::Pcjr;
}

inline bool is_machine_pcjr_or_tandy() {
	return is_machine_pcjr() || is_machine_tandy();
}

inline bool is_machine_cga_mono() {
	return machine == MachineType::CgaMono;
}

inline bool is_machine_cga_color() {
	return machine == MachineType::CgaColor;
}

inline bool is_machine_cga()
{
	return is_machine_cga_mono() || is_machine_cga_color();
}

inline bool is_machine_cga_or_better() {
	return is_machine_cga() ||
               is_machine_pcjr_or_tandy() ||
               is_machine_ega_or_better();
}

inline bool is_machine_hercules() {
	return machine == MachineType::Hercules;
}

#endif /* DOSBOX_MACHINE_TYPE_H */
