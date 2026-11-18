# TMR sensing guide

How a key press becomes a number in firmware on this hardware, and what to check when the numbers look wrong.

## The sensor

Each key has a TMR2615D sensor under a magnet in the switch. The sensor has three pins: VOUT, VDD and GND. Its output is ratiometric, which means that with no magnetic field it rests at about half of its own supply (about 1.65 V on 3.3 V). Moving the magnet toward or away from the sensor moves the output away from that rest point.

Limits taken from the datasheet and recorded in the Rev K notes: at most 10 nF of load on VOUT, at least 10 k of DC load resistance, and about 30 kHz of sensor bandwidth.

Two practical consequences:

- If the sensor supply differs from the ADC reference (VDDA), every key shifts together. That is a common-mode shift, and it looks like all keys drifting at once.
- A switch's magnet at rest already lowers the reading by about 100 mV compared with an empty socket. Pressing moves it down a further 400 to 590 mV on this board. So a key with no switch fitted reads differently from a key with one, and the stored zero has to be retaken after switches are moved.

## The signal path

```
TMR2615D (one per key) -> J1 pad -> wire -> J8 hole on the dev board
  -> 100 ohm + 4.7 nF RC filter (one per input)
  -> CD74HC4067 16:1 mux (S0..S3 = PA7, PB0, PB1, PB2)
  -> PC4 = ADC1_IN13, 14-bit, HSE 16 MHz as ADC kernel clock
```

Sixteen mux inputs serve fifteen sensors, so one input is always unused. On the author's wiring, J8 hole 2 is deliberately left empty and always reads OPEN in the self-test. This is not a fault, and it is the reason a count of "5 open keys" can show up when only 4 sensors are actually open.

### Wiring map (numpad J1 pad to dev board J8 hole)

The sensor outputs TMR_OUT1, 5, 9, 13, 2, 6, 10, 14, 15, 11, 7, 3, 12, 8, 4 arrive on J1 pads 6 to 20 and go to J8 holes 16, 14, 12, 10, 7, 5, 3, 1, 15, 13, 11, 9, 8, 6, 4. In firmware this is the table `TMR_KEY_OF_CHANNEL {14,0,10,4,6,8,2,12,3,13,7,9,11,5,15,1}`, in `tmr_config.h`. If the key labels in the console do not match the physical keys, use the console's `m` (learn wiring order) command and paste the printed line into that file.

### Physical key positions

On the numpad (KiCad U1 to U15) the columns are at x = 134.62, 153.67, 172.72, 191.77 mm and the rows at y = 73.46, 92.51, 111.56, 130.61 mm. K1 to K4 are the top row and K13 to K15 the bottom row; the bottom-right position has no sensor.

## Supply wiring lesson

During bring-up the sensor supply was taken from a separate wire on the board. Rest readings then swung by tens of millivolts together and the live deltas drifted at rest. Taking the numpad 3V3 and GND from the dev board's mux header (the same rails as the ADC reference) made every channel hold at +0 mV and rest at 49.3 to 50.1 % of the supply for keys without switches. If readings swing together, check the supply wiring before anything else.

## The self-test

For each input the diagnostic image: probes the line with the pin's pull-up and pull-down (an open input follows the pull, a connected sensor does not), measures rest voltage and noise, and then reports a status. After the self-test, press a key to get a swing figure; the self-test table lists per key the number of presses and the peak up and down in mV. A key passes when its press swing is clearly present. Observed swings on the bench were about -405 to -566 mV.

## Press and release logic in the diagnostic

- DOWN when the reading is at least 150 mV from rest (`TMR_PRESS_MV`).
- UP when it returns to within 100 mV of rest (`TMR_RELEASE_MV`).
- Rest tracking follows slow drift only while the key is up and within 30 mV of rest.

Because release needs a return to within 100 mV of rest, a key that is tapped repeatedly without letting go fully reads as one long hold. That is the rule behaving as written, not a hardware fault.

The fast image adds a rapid-trigger rule: UP when the reading backs off 60 mV from the deepest point of the press, and DOWN again after it goes 60 mV deeper than the lowest point since release. These are defaults chosen for the first test, not tuned values.

## What to check, in order

1. All keys moving together: the sensor supply and ground wiring.
2. One input reading OPEN: the sensor solder joints (the two small pins are the usual culprit), then the wire between J1 and J8 with a meter.
3. A key that reads odd after switches were moved: re-zero (`z`, hands off).
4. Reads fine but wrong key label: learn the wiring order (`m`).
