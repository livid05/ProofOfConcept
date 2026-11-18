# Current architecture — 8K USB test

MX2 -> PB4 input -> TIM2-timed 125 us samples -> eager press / stable release -> event queue -> eight-byte HID report -> ST USB core -> HAL PCD / HS PHY -> PC.

## Timing

The 16 MHz external HSE crystal feeds a PLL for the 160 MHz CPU, and separately references the embedded high-speed USB PHY. HAL SysTick remains 1 ms for HAL timeouts and LED blinking.

`key_clock.c` reserves the 32-bit TIM2 counter at 1 MHz, with no timer interrupt. APB1=/1 gives TIM2=160 MHz; PSC=159 divides this to 1 MHz. The code checks this clock assumption. Unsigned counter subtraction handles rollover (~71.6 minutes). TIM2 freezes at debugger breakpoints.

`key_switch.c` targets a sample every 125 us. It advances the schedule when late but never invents past samples. Foreground scheduling and interrupts can introduce jitter; key_scan_count, key_scan_missed_periods and key_scan_max_gap_us allow investigation. USB submission runs every loop, including between sample times, so a completed endpoint need not wait another scan before taking the next queued event.

## Debounce

With KEY_EAGER_PRESS=1, the first sampled low immediately becomes a press. Subsequent bounce cannot produce another press while that key remains accepted as down. Release requires the input to stay continuously high for KEY_DEBOUNCE_MS (10 ms). Thus press response is faster, but release still has a deliberate filter delay. A brief electrical noise pulse may be accepted as a press. Set KEY_EAGER_PRESS=0 for symmetric stable filtering. Both modes need real switch testing.

## USB

The normal boot keyboard layout is unchanged: byte 0 modifier bits, byte 1 reserved, bytes 2–7 key usages. MX2 uses byte 2; an all-zero report releases it. No modifiers are set. OS keyboard layout, Caps Lock and host repeat still apply.

High-speed bInterval=1 requests one interrupt service opportunity per 125 us (8K). Full-speed fallback uses bInterval=1 for 1 ms (1K). Descriptor and endpoint initialization share constants in board_config.h. The other-speed descriptor remains correct. An idle keyboard normally sends only changed reports or HID idle repeats; 8K polling does not mean 8000 changing key reports per second. Actual scheduling/latency has not been measured.

A persistent transmit buffer stays valid until USB completion. A short interrupt mask serializes foreground submission with reset/completion. A 15-event queue preserves press and release while busy. Overflow requests all-keys-up and release before rearming. Reset/reconnect discards stale events and requires a stable released switch before a new press. Remote wakeup, DMA, LPM and RTOS are disabled.

## Status and scope

Tyler verified the prior 1K build types A and stops repeating on release. USBView confirmed High Speed and bInterval=4 for that build. The prior image/sources are archived under archive/verified-usb-1k. The new 8K firmware still needs board verification; expect High Speed and bInterval=0x01 after flashing and reconnecting.

USB 2.0 high-speed interrupt service intervals cannot be reduced below 125 us. Multiple packets/endpoints can raise aggregate report counts, which is different from uniformly spaced faster polls. This project uses one standard interrupt IN endpoint. USB SuperSpeed requires different hardware from this STM32U5A5 board.

TMR, encoder, RGB and full matrix expansion remain deferred.
