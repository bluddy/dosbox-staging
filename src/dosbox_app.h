// SPDX-FileCopyrightText:  2019-2025 The DOSBox Staging Team
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef DOSBOX_APP_H
#define DOSBOX_APP_H

#include "misc/compiler.h"
#include "misc/types.h"

int sdl_main(int argc, char *argv[]);

// The shutdown_requested bool is a conditional break in the parse-loop and
// machine-loop. Set it to true to gracefully quit in expected circumstances.
extern bool shutdown_requested;

// The E_Exit function throws an exception to quit. Call it in unexpected
// circumstances.
[[noreturn]] void E_Exit(const char *message, ...)
        GCC_ATTRIBUTE(__format__(__printf__, 1, 2));

class Section;
class SectionProp;

typedef Bitu (LoopHandler)(void);

const char* DOSBOX_GetVersion() noexcept;
const char* DOSBOX_GetDetailedVersion() noexcept;

double DOSBOX_GetUptime();

void DOSBOX_RunMachine();
void DOSBOX_SetLoop(LoopHandler * handler);
void DOSBOX_SetNormalLoop();

void DOSBOX_InitAllModuleConfigsAndMessages(void);

void DOSBOX_SetMachineTypeFromConfig(SectionProp* section);

int64_t DOSBOX_GetTicksDone();
void DOSBOX_SetTicksDone(const int64_t ticks_done);
void DOSBOX_SetTicksScheduled(const int64_t ticks_scheduled);

#endif /* DOSBOX_APP_H */
