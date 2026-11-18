Verified 8K single-key firmware snapshot, taken 2026-10-22 just before the 4x4 matrix was integrated.
- build\keypad.hex/.bin/.elf = the validated 8K image (BIN SHA-256 872DCEF3CD7B29430E2C811D7F40221BA4AB534D050287752D4289B616044BA3).
  Flash it directly with CubeProgrammer if you want the one-key firmware back without rebuilding.
- firmware\Core\Src\{key_switch,usb_keyboard,main}.c and firmware\Core\Inc\keyboard.h = the sources that produced it.
- tests\host and tools\test-host.ps1 = the single-key host tests.
- build\ also holds the old CMake cache (it points at the pre-move folder path and is not reusable).
Hardware status of this 8K image: compiled and logic-tested only; 8K enumeration was not yet bench-confirmed at snapshot time.
To roll back: copy these sources back over firmware\Core, delete key_matrix.c/matrix_hw.c/key_matrix.h/matrix_config.h, restore tests\host and tools\test-host.ps1, rebuild.
