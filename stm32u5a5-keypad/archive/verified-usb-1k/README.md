# Known working USB baseline

Saved before the 8K timing update. Tyler confirmed MX2 types A, holding repeats, and releasing stops it. USBView confirmed High Speed and bInterval=4 (1K). Other formal acceptance tests were not recorded.

To restore the previously built firmware, flash this folder's build/keypad.hex using CubeProgrammer USB DFU, then reset with BOOT released. Source snapshots identify the timing changes; they are not a second active project.
