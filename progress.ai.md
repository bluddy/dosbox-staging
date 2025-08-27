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

### Targeted `dosbox.h` Include Replacements
- Replaced `#include "dosbox.h"` with specific includes in the following files:
    - `src/main.cpp`: Replaced with `#include "dosbox_app.h"`.
    - `src/shell/shell_history.cpp`: Replaced with `#include "config/config.h"` and `#include "misc/logging.h"`.
    - `src/shell/autoexec.cpp`: Replaced with `#include "misc/logging.h"`, `#include "misc/messages.h"`, and `#include "dos/dos_inc.h"`.
    - `src/misc/unicode.cpp`: Replaced with `#include "misc/logging.h"` and `#include "dosbox_app.h"`.
    - `src/misc/messages.cpp`: Replaced with `#include "misc/logging.h"`.
    - `src/misc/host_locale_macos.cpp`: Removed `#include "dosbox.h"` entirely as it was not needed.
    - `src/misc/ethernet_slirp.cpp`: Replaced with `#include "misc/logging.h"` and `#include "identity.h"`.
    - `src/ints/xms.cpp`: Replaced with `#include "misc/logging.h"`, `#include "hardware/memory.h"`, `#include "hardware/port.h"`, `#include "utils/math_utils.h"`, `#include "cpu/cpu.h"`, `#include "cpu/callback.h"`, `#include "dos/dos_inc.h"`, `#include "ints/bios.h"`, `#include "config/setup.h"`, and `#include "misc/compiler.h"`.
    - `src/ints/int10.cpp`: Replaced with `#include "misc/logging.h"`, `#include "machine_type.h"`, and `#include "constants.h"`.

### Current Status
- `dosbox.h` has been significantly reduced in size and now primarily serves as a central point for other headers. The goal is to eventually remove `dosbox.h` entirely if possible, or keep it as a minimal, high-level include.
- The project successfully builds after each refactoring step, ensuring no regressions were introduced.

### Next Steps
- Continue replacing `#include "dosbox.h"` in remaining files with more specific headers.
- Investigate and resolve any new build errors that arise from these changes.
