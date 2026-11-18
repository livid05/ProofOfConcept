# Shared project instructions

- Read README.md and docs/HANDOFF.md, docs/HARDWARE.md, docs/STATUS.md before editing.
- STM32U5A5RJT6, Devboard RevK, Numpad RevE. Active target is STM32U5A5xx; TMR is deferred.
- Current acceptance goal: one mechanical MX2 switch sends one USB A-key press and its release reliably.
- Keep detailed explanatory comments. Put adjustable pin, timing and usage settings in board_config.h.
- Use firmware/ as the single source of truth for both assistants. Coordinate file ownership before simultaneous edits.
- Do not regenerate the archived .ioc over the active CMake project. Preserve vendor licenses and original project.
- Build with tools/build.ps1 after firmware changes. Report hardware tests as pending unless actually observed or explicitly confirmed by Tyler.
- Update docs/STATUS.md and docs/HANDOFF.md with results. Keep responses to Tyler concise.
- Do not automatically flash, change option bytes, or add TMR/encoder/RGB functionality without Tyler's instruction.
