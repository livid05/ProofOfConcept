# Lessons learned

Short, specific, and each tied to something that happened.

- **Check the footprint against the actual part.** The first numpad read every sensor as open because the pads were sized for the 1.6 mm package while the parts were 2 mm. Compare the datasheet drawing with the real part before ordering boards.
- **Put the sensor supply and the ADC reference on the same rails.** Ratiometric sensors rest at half their own supply, so a different supply shifts all keys together. Stable readings came from using the mux header's 3V3 and GND.
- **Re-zero whenever switches move.** A fitted switch lowers the rest reading by about 100 mV. A stored zero from before the move gives phantom offsets.
- **A deliberately empty input will read OPEN.** Sixteen mux inputs and fifteen sensors means one open input is normal. Say so in the report to avoid chasing it.
- **Read rules before blaming hardware.** Long "held" events and keys that "registered once" came from the release rule, not from bad sensors. Check what the code is defined to do first.
- **Measure before estimating.** An early 130 us scan estimate was too optimistic; the measured 730 us showed about 13 us of per-channel overhead that the estimate ignored. Treat every unmeasured figure as an estimate, and label it.
- **Keep every working stage.** The one-key 1 kHz, one-key 8K and matrix images are each kept in `archive/` or `future_work/`, so a regression can always be compared with the last good state.
- **Separate USB rate from scan rate.** The 7999 reports per second measurement proves the USB path only. It says nothing about how fresh each report's data is.
- **JLC details cost money.** Board size thresholds, the 90 ohm stackup being unavailable with Economic PCBA, per-part loading fees for Extended parts, and panel fees for mixed designs all drove revisions (Rev I, Rev J, TriMode60 Rev E, and ordering the numpad on its own).
- **Via-in-pad and edge clearance.** The numpad needed a whole revision (Rev C) to move 36 vias out of pads and another small one for an edge-clearance DRC error.
- **Keep tools from hiding the problem.** The editor's task window cannot read single keystrokes; run the console in a normal terminal.
