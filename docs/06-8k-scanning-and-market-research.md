# Scanning at 8 kHz, and what shipping TMR keyboards do

## The time budget

An 8 kHz report needs a fresh reading every 125 us. The USB side was measured separately and does 7999 reports per second, so the open problem is the sensor scan.

The original blocking scan took 730 us for 16 inputs. That is about 45.6 us per channel, made of a 10 us settle, four conversions and about 13 us of per-call overhead. At that speed the readings behind each report would be several frames old.

The fast image targets 16 channels inside 125 us (under 8 us per channel) by starting the next conversion while the mux settles, using direct register access and the cycle counter for timing. Whether it reaches that, and at what noise, is exactly what the sweep measures. Until a sweep has been run on the board, every figure for it in this document is an estimate.

## Tradeoffs

- **Averaging against speed.** One conversion per channel is noisier than four. Short sampling windows are faster but give the filter and the sensor's source less time to charge the ADC input. The sweep prints noise for each setting so the choice rests on numbers.
- **Settling against channel memory.** Switching the mux and sampling too soon leaves a trace of the previous channel in the next reading. With the Rev K filter the design estimate is about five time constants of settling in 2.5 us, but this is not verified. A 1000 ppm leak of a 100 mV step is about 0.1 mV, small next to a 150 mV press threshold, but could matter for fine rapid-trigger distances.
- **14-bit against 12-bit.** The U5A5 ADC can run 14-bit, which costs conversion time. With 400 to 590 mV of press travel, resolution is probably not the limit; noise is. If 14-bit cannot meet the budget cleanly, 12-bit with a little oversampling is the fallback.
- **Scan rate against report rate.** Scanning at 8 kHz only helps if a report goes out each 125 us frame. The USB path already supports that.
- **Rapid trigger against noise.** A small release distance needs a quiet signal, otherwise the key chatters. The fast image's 60 mV default is a starting point to test.
- **Power.** Scanning at 8 kHz all the time costs battery. A later design should scan fast only while a key is moving and idle slowly with a wake on activity; the cost is a few milliseconds of wake latency.

## Market research (read 2026-11-18)

These findings come from vendor and review pages. Vendor claims are marked as such; none of this was measured by the author.

- 8 kHz TMR keyboards are already on sale, for example the Cherry Xtrfy K5 Pro TMR and MX 8.2 Pro TMR (8000 Hz wired and wireless), the Logitech G512 X TMR (75 and 98-key, 8K), and Keychron's Q HE 8K series, plus MonsGeek and Gamakay boards.
- A sensor vendor (MDT) says its TMR2103H has a 2 us power-on time that lets analog keyboards exceed 1 kHz and reach 8 kHz. This is a vendor claim for a different part than the TMR2615; check the TMR2615 datasheet before relying on it.
- Scan rate and polling rate are different things, and a keyboard advertising 8K polling may not scan at 8K internally. Several sources make this point. This is the situation the 730 us scan was in.
- Marketing scan rates such as 256 kHz or 512 kHz are almost certainly per-sensor or oversampled figures, not full-keyboard sweeps, and are not a target for this project.
- Reviews are mixed on whether TMR beats Hall in practice. One hardware review of a Hall and a TMR version of the same board could not tell them apart by behavior, and rapid trigger responded the same. Calibration, filtering, scan design and firmware seem to matter as much as the sensor.
- One hobby build treats the TMR sensor as idle at half supply, matching the ratiometric rest seen here. A review advises turning rapid trigger off on any key with a mechanical switch installed, to avoid double clicks, which is worth building into the key profiles.
- No independent end-to-end latency test of an 8K TMR board, and no public detail of a commercial firmware's scan pipeline, was found.

### Sources

- [MagneticsMag, TMR2103H](https://magneticsmag.com/tmr2103h-elevating-analog-keyboards-for-unmatched-gaming-performance/)
- [Cherry Xtrfy K5 Pro TMR Compact press release](https://www.cherry.de/en-us/company/news/press/article/cherry-xtrfy-launches-new-compact-magnetic-switch-keyboard-with-tmr-technology)
- [Notebookcheck, Logitech G512 X TMR](https://www.notebookcheck.net/Logitech-launches-G512-X-TMR-analog-mechanical-gaming-keyboard.1284897.0.html)
- [Keychron Q HE 8K series](https://mechanicalkeyboard.net/?p=1366)
- [Gamakay TK75 TMR review](https://www.hlplanet.com/?p=8777)
- [HWBusters, Epomaker HE75 V2 TMR review](https://hwbusters.com/peripherals/epomaker-he75-v2-tmr-review/5/)
- [Evetech, TMR vs Hall effect](https://evezone.evetech.co.za/quick-bytes/are-tmr-keyboards-better-than-hall-effect-gaming)
- [mechanicalkeyboard.net, polling rate vs scan rate](https://mechanicalkeyboard.net/?p=1627)
- [Attack Shark, PCB matrix scanning and latency](https://attackshark.com/blogs/knowledges/pcb-matrix-scanning-keyboard-latency)
- [EWEADN, polling rate, scan rate and latency](https://www.eweadn.com/blogs/news/what-are-polling-rate-scan-rate-latency-keyboard-performance-explained)
- [Hack Club Blueprint, a TMR keyboard build](https://blueprint.hackclub.com/projects/5612)
