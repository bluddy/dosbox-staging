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
9.  **Decoupled `src/fpu/fpu.h` from `dosbox.h`**:
    - Replaced `#include "dosbox.h"` with `#include "dosbox_config.h"`, `#include "misc/types.h"`, and `#include "misc/logging.h"` in `src/fpu/fpu.h`.
    - Removed `#include "dosbox.h"` from `src/fpu/fpu.cpp`.
10. **Fixed linker errors related to FPU functions**: Added `libfpu_dep` to `src/cpu/meson.build`.
11. **Fixed `libdos_dep` unknown variable error**: Reordered the subdirectories in `meson.build` (moving `src/fpu` and `src/dos` before `src/cpu`).
12. **Fixed duplicated `subdir('src/dos')` in `meson.build`**.

### Current Status
- `dosbox.h` has been significantly reduced in size and now primarily serves as a central point for other headers. The goal is to eventually remove `dosbox.h` entirely if possible, or keep it as a minimal, high-level include.
- The project successfully builds after each refactoring step, ensuring no regressions were introduced.

### Next Steps
- Continue replacing `#include "dosbox.h"` in remaining files with more specific headers.
- Investigate and resolve any new build errors that arise from these changes.