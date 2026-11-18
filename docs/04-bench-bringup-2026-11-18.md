# Bench bring-up, 2026-11-18

What happened on the bench with the numpad and dev board (board Rev K, numpad Rev E), in the order it was found. All hardware observations here were made by the author; software-only checks are labelled.

## Result

- 14 of 15 keys PASS, with swings of about -405 to -566 mV.
- K4 (J8 hole 4, J1 pin 20) is still OPEN and parked. Next step is a meter check from J1 pin 20 to J8 hole 4, and from pin 20 to pin 21 (ENC_CLK) to look for a bridge.
- Blocking scan measured at 730 us for 16 inputs (max 788 us) over 279,902 scans, with 0 USB bytes dropped.

## What was found, in order

1. **Four keys gave no output; the table showed five open.** Three sensors (K5, K10, K15) were open and resoldered. The fifth "open" input was J8 hole 2, which is deliberately empty (sixteen mux inputs, fifteen sensors).
2. **Readings from keys changed when switches were moved.** Only five or six switches were on hand, so switches were being moved between sockets. A switch's magnet lowers the rest reading by about 100 mV, so a stored zero is wrong after a move. Fix: re-zero with the switches in.
3. **Keys appeared to register only once while being tapped quickly.** The release rule needs a return within 100 mV of rest. Quick taps that never fully let go give one long "held" event. Early on this was misread as a hardware fault on specific keys; the later data showed it was the rule.
4. **All keys swung together by 85 to 96 mV at rest.** This pointed at the supply, not at the sensors. Taking 3V3 and GND from the dev board's mux header made every channel hold at +0 mV, with rests of 49.3 to 50.1 %.
5. **The console could not read single keys in the editor's task window.** Run `tmr-console.ps1` in a normal terminal, or use a serial terminal with DTR asserted (the board enumerates as USB 0483:5712).
6. **Scan time.** The 730 us blocking scan is about 45.6 us per channel: a 10 us settle, four samples and about 13 us of per-channel overhead. That is the reason the fast image exists.

## Corrections made along the way

These are recorded because they show how the data was read, not only the final answer.

- An early reading of "other keys moved 115 mV or less" was blamed on one key being held. It was actually a stale zero on keys whose switch bias had changed.
- A very long hold on one key was reported as the key being held for minutes. The author had not held it that long; the rule had not seen a return to within 100 mV of rest.
- One key was first guessed to be weak hardware, until later data showed it was normal.
- An early estimate of about 130 us for a fast scan was too optimistic. The measured per-channel overhead showed it needed rework, which is what the pipelined scan does.

## Still pending

- Matrix columns 3 and 4: recheck that a-to-o types in order after the software swap fix.
- A meter check of the numpad 3.3 V (indirectly supported by the stable 49.5 % rests, but not measured).
- Everything about the fast scan: it has not been built for the board or run.
