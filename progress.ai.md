## Progress on Untangling the .h Mess

### Basic Rules
- Build with meson
- Avoid touching libglad if you can - it's a mountain of trouble. Keep opengl and glad as is.

### Initial Analysis
- Identified `dosbox.h` as a central, problematic header due to its extensive inclusions.
- Found 81 C++ files directly including `dosbox.h`.

### Refactoring Steps
1.  **Extracted `MachineType` and `SvgaType` enums and related functions** into `src/machine_type.h`.
    - Replaced original definitions in `dosbox.h` with `#include "machine_type.h"`.
    - Verified successful build.
2.  **Extracted constants** (e.g., `DefaultMt32RomsDir`, `MicrosInMillisecond`) into `src/constants.h`.
    - Replaced original definitions in `dosbox.h` with `#include "constants.h"`.
    - Verified successful build.
3.  **Extracted identity-related macros** (e.g., `DOSBOX_PROJECT_NAME`, `DOSBOX_NAME`) into `src/identity.h`.
    - Replaced original definitions in `dosbox.h` with `#include "identity.h"`.
    - Verified successful build.
4.  **Extracted core application logic declarations** (e.g., `sdl_main`, `shutdown_requested`, `E_Exit`) into `src/dosbox_app.h`.
    - Replaced original declarations in `dosbox.h` with `#include "dosbox_app.h"`.
    - Updated `src/dosbox.cpp` to include `dosbox_app.h` instead of `dosbox.h`.
    - Verified successful build.
5.  **Fixed `iir.h` compilation error**: Added `libiir_dep` to `src/ints/meson.build`.
6.  **Fixed `uint16_t` error**: Added `#include <cstdint>` to `src/shell/command_line.h`.
7.  **Decoupled `src/misc/cross.h` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "misc/types.h"` in `src/misc/cross.h`.
    - Added `#include "dosbox_config.h"` to `src/misc/cross.cpp`.
8.  **Decoupled `src/gui/mapper.h` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "identity.h"` and a forward declaration for `Section` in `src/gui/mapper.h`.
    - Removed `#include "dosbox.h"` from `src/gui/mapper.cpp`.
9.  **Decoupled `src/fpu/fpu.h` and `src/fpu/fpu.cpp` from `dosbox.h`**:
    - In `src/fpu/fpu.h`, replaced conditional `#include "dosbox.h"` with `#include "dosbox_config.h"`, `#include "misc/types.h"`, and `#include "misc/logging.h"`.
    - In `src/fpu/fpu.cpp`, replaced `#include "dosbox.h"` with `#include "hardware/memory.h"`, `#include "misc/cross.h"`, `#include "misc/logging.h"`, `#include "misc/support.h"`, and `#include "config/config.h"`.
10. **Fixed linker errors related to FPU functions**: Added `libfpu_dep` to `src/cpu/meson.build`.
11. **Fixed `libdos_dep` unknown variable error**: Reordered the subdirectories in `meson.build` (moving `src/fpu` and `src/dos` before `src/cpu`).
12. **Fixed duplicated `subdir('src/dos')` in `meson.build`**.
13. **Added `libdos_dep` to `src/cpu/meson.build`**.
14. **Decoupled `src/config/config.h` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "shell/command_line.h"`, `#include "config/setup.h"`, `#include "misc/types.h"`, `#include "misc/logging.h"`, and `#include "identity.h"` in `src/config/config.h`.
15. **Decoupled `src/gui/sdlmain.cpp` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "dosbox_config.h"`, `#include "misc/compiler.h"`, `#include "misc/messages.h"`, `#include "misc/types.h"`, `#include "identity.h"`, `#include "dosbox_app.h"`, and `#include "misc/logging.h"` in `src/gui/sdlmain.cpp`.
16. **Decoupled `src/gui/render.cpp` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with specific includes in `src/gui/render.cpp`.
17. **Decoupled `src/gui/render_scalers.cpp` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with specific includes in `src/gui/render_scalers.cpp`.
18. **Decoupled `src/hardware/cmos.cpp` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with specific includes in `src/hardware/cmos.cpp`.
19. **Decoupled `src/hardware/dma.h` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with specific includes in `src/hardware/dma.h`.
20. **Decoupled `src/hardware/hardware.h` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with specific includes in `src/hardware/hardware.h`.
21. **Decoupled `src/hardware/ide.h` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with specific includes in `src/hardware/ide.h`.
22. **Decoupled `src/hardware/iohandler_containers.cpp` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "misc/logging.h"` and `#include "misc/types.h"` in `src/hardware/iohandler_containers.cpp`.
23. **Decoupled `tests/stubs.cpp` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "dosbox_config.h"` in `tests/stubs.cpp`.
24. **Decoupled `src/debugger/debugger.h` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "dosbox_config.h"` and `#include "misc/types.h"` in `src/debugger/debugger.h`.
25. **Decoupled `src/debugger/debugger.cpp` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "dosbox_config.h"`, `#include "misc/types.h"`, `#include "cpu/flags.h"`, `#include "cpu/decoder.h"`, `#include "dosbox_app.h"`, and `#include "misc/support.h"` in `src/debugger/debugger.cpp`.
26. **Decoupled `src/debugger/debugger_gui.cpp` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "dosbox_config.h"` and `#include "misc/types.h"` in `src/debugger/debugger_gui.cpp`.
27. **Decoupled `src/debugger/debugger_disasm.cpp` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "dosbox_config.h"` and `#include "misc/types.h"` in `src/debugger/debugger_disasm.cpp`.
28. **Decoupled `src/debugger/debugger_win32.cpp` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "dosbox_config.h"` in `src/debugger/debugger_win32.cpp`.
29. **Decoupled `src/cpu/cpu.h` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "dosbox_config.h"`, `#include "misc/types.h"`, and `#include "misc/compiler.h"` in `src/cpu/cpu.h`.
30. **Decoupled `src/cpu/callback.cpp` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "misc/types.h"` and `#include "misc/logging.h"` in `src/cpu/callback.cpp`.
31. **Decoupled `src/cpu/core_dyn_x86.cpp` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "dosbox_config.h"`, `#include "misc/types.h"`, `#include "cpu/flags.h"`, and `#include "dosbox_app.h"` in `src/cpu/core_dyn_x86.cpp`.
32. **Decoupled `src/cpu/core_dynrec.cpp` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "dosbox_config.h"`, `#include "misc/types.h"`, `#include "cpu/flags.h"`, and `#include "dosbox_app.h"` in `src/cpu/core_dynrec.cpp`.
33. **Decoupled `src/cpu/core_full.cpp` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "dosbox_config.h"`, `#include "misc/types.h"`, `#include "cpu/flags.h"`, `#include "misc/logging.h"`, and `#include "dosbox_app.h"` in `src/cpu/core_full.cpp`.
34. **Decoupled `src/cpu/core_prefetch.cpp` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "dosbox_config.h"`, `#include "misc/types.h"`, `#include "cpu/flags.h"`, `#include "misc/logging.h"`, and `#include "dosbox_app.h"` in `src/cpu/core_prefetch.cpp`.
35. **Decoupled `src/cpu/mmx.cpp` from `dosbox.h`**:
    - Removed `#include "dosbox.h"` in `src/cpu/mmx.cpp`.
36. **Decoupled `src/cpu/flags.cpp` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "misc/types.h"` and `#include "misc/logging.h"` in `src/cpu/flags.cpp`.
37. **Decoupled `src/cpu/paging.h` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "dosbox_config.h"` and `#include "misc/types.h"` in `src/cpu/paging.h`.

### Current Status
- `dosbox.h` has been significantly reduced in size and now primarily serves as a central point for other headers. The goal is to eventually remove `dosbox.h` entirely if possible, or keep it as a minimal, high-level include.
- The project successfully builds after each refactoring step, ensuring no regressions were introduced.

### Next Steps
- Continue replacing `#include "dosbox.h"` in remaining files with more specific headers.
- Investigate and resolve any new build errors that arise from these changes.
