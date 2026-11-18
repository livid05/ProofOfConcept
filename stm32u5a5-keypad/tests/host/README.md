# Host (PC) tests for the matrix firmware

Run `tools/test-host.ps1` (needs Visual Studio 2022 `cl.exe`). It compiles the REAL `key_matrix.c` and `matrix_hw.c` against fake time, fake key contacts, a fake USB endpoint and an electrical model of the 4 x 4 matrix. It never touches the board.

* `harness_logic.c` - every key, chords, press order, bounce, 10 ms release, six-key limit, busy USB, reconnect, overflow, clock wrap, 8000 scans/s.
* `harness_hw.c` - pin mapping (PC0-PC3 rows, PB4-PB7 columns), one row low at a time, settle time, all 65,536 key patterns.
* `harness_noneager.c` - conservative mode (eager press off, on a copied config).
* `fakes/` - stand-in HAL and keyboard headers.

A failing test returns the source line number of the failed check. The earlier single-key tests are in `archive/verified-8k-single-key/tests/host`.
