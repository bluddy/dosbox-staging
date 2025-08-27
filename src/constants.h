// SPDX-FileCopyrightText:  2019-2025 The DOSBox Staging Team
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DOSBOX_CONSTANTS_H
#define DOSBOX_CONSTANTS_H

constexpr auto DefaultMt32RomsDir   = "mt32-roms";
constexpr auto DefaultSoundfontsDir = "soundfonts";
constexpr auto GlShadersDir         = "glshaders";
constexpr auto DiskNoiseDir         = "disknoises";
constexpr auto PluginsDir           = "plugins";

constexpr auto MicrosInMillisecond = 1000;
constexpr auto BytesPerKilobyte    = 1024;

enum class DiskSpeed { Maximum, Fast, Medium, Slow };

#endif /* DOSBOX_CONSTANTS_H */
