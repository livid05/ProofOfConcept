# Presentations and reports

Everything in `reports/` is coursework for the CET 497 capstone at Central Connecticut State University, kept so the project history is complete. These files are all rights reserved (see `LICENSE`, section 5). This guide says what each one is for and what it contains, so you can pick the right one without opening all of them. Contents are described from the files themselves; where a number is a placeholder in the deck, it is called a placeholder here.

## How the decks relate

The project had three presentation stages: a proposal, a midterm and the capstone presentation. The midterm went through several revisions (v1 to v3) before the final form. Several files exist as near-duplicates because they were saved from different folders or renamed copies.

| File | Slides or pages | Stage | What it is |
|---|---|---|---|
| `Project_Proposal_Presentation_Tyler Judkins.pptx` | 6 | Proposal | The first pitch of the keyboard project |
| `Project_Proposal_Presentation_Tyler Judkins (Documents copy).pptx` | 6 | Proposal | Same size as the file above but a different file (different checksum), so treat it as a separate save of the same deck |
| `TMR_Keyboard_Presentation.pptx`, `_v2`, `_v3` | varies | Midterm drafts | Successive drafts of the midterm deck |
| `TMR_Keyboard_Midterm_Presentation_v3.pptx` and `(1).pptx` | 12 | Midterm | The midterm deck in its third revision; `(1)` is a duplicate save |
| `TMR_Keyboard_Midterm_Presentation_v3.pdf` | 12 pages | Midterm | PDF of the v3 deck, titled "Per-Key Precision: A Programmable TMR Keyboard Platform" |
| `Capstone Midterm Presentation.pptx` | 9 | Midterm | A shorter midterm version titled "Customizable Keyboard Design Project" |
| `TMR_Keyboard_Final.pptx`, `TMR_Keyboard_Final_v2.pptx` | varies | Late drafts | Later iterations before the capstone deck |
| `Capstone Presentation.pptx` | 20 | Capstone, 22 Oct 2026 | The main capstone presentation, "Per-Key Precision" |
| `Capstone_Keyboard_Review.pptx` | 22 | Review | Design review of Dev board Rev K and Numpad Rev E, with appendices |
| `capstone-deck-source/` | 21 slides | Capstone | The source files of the capstone deck (see below) |
| `Keyboard_Project_Midterm_Feedback - Copy.docx` | 1 document | Feedback | Instructor feedback on the midterm |
| `U595 Dev Board (Rev H) Overview.docx` and `.pdf` | 13 pages (pdf) | Documentation | Full documentation of the Rev H dev board, dated 27 Sep 2026 |

Slide counts for the older drafts were not recorded; open them to check.

## What the midterm decks cover

The midterm material follows one story: a problem statement about keyboards whose switches limit what a keypad can sense; a background survey of mechanical, Hall effect and TMR sensing; the concept of a programmable keyboard platform built on TMR sensors; the proof of principle (the 4x4 matrix and the dev board); the timeline in five phases with a target of December 2026. These ideas are told in plain prose in [01-project-story.md](01-project-story.md) and [03-tmr-sensing-guide.md](03-tmr-sensing-guide.md).

## The capstone presentation (22 October 2026)

Twenty slides, in six sections:

1. Why a programmable keyboard matters and who benefits.
2. What has been built and delivered so far.
3. The requirements and the evidence for each. At the time of the deck two requirements were met, one was untested and four were still pending.
4. The block diagram, wiring, flow chart, code and bill of materials. The bill of materials slides carry cost placeholders, not final prices.
5. How success is measured and what is still unproven, including the 8K rate test that measured 7,999 reports per second with no drops.
6. Lessons learned and next steps.

The deck was authored as HTML slides (one file per slide, speaker notes inside each `<aside>`) and exported to PowerPoint. The source is in `reports/capstone-deck-source/`: `deck.json` lists the slide order and sections, and `slides/*.html` holds the 21 slides (cover, purpose, concept, society, solution, system, requirements, demo, blockdiagram, wiring, flow, timing, code1, code2, bom1, bom2, verification, ratetest, evaluation, conclusion, nextsteps). Open the HTML files in a browser; they use two web fonts (DM Sans and JetBrains Mono) loaded from Google Fonts. The deck predates the measurements of 18 November 2026; see [04-bench-bringup-2026-11-18.md](04-bench-bringup-2026-11-18.md) for what changed since.

## The design review deck

`Capstone_Keyboard_Review.pptx` (22 slides) reviews the two boards then in hand, Dev board Rev K and Numpad Rev E, with appendices. Use it with [09-kicad-guide.md](09-kicad-guide.md) when you need the board-level view.

## Instructor feedback on the midterm

The feedback document asks for these changes, which later drafts of the decks respond to:

- Reword the title so it states what the project is.
- Rework the problem statement and re-read the source cited for it (Dieter) before relying on it.
- Add a summary of the research behind the concept.
- Separate the concept from the specification.
- Give more detail on the proof of principle.
- Reduce the timeline to a single slide.

It is a record of how the project was shaped by review; it is included only as part of my own coursework record.

## The Rev H dev board overview

`U595 Dev Board (Rev H) Overview.docx` (and its PDF) is the most complete single document on the dev board: power, MCU, USB, SWD, UART, buttons and LEDs, the J8 sensor header, the J2 and J3 side headers, the stackup, the bill of materials and the cost (about $26.64 per board, of which the STM32U595 is $24.32). Its connector tables are copied into [09-kicad-guide.md](09-kicad-guide.md). Rev K later changed the MCU to the STM32U5A5 and the mux filter, so use the guide for the current values.

## Related reference material outside `reports/`

| Folder | What is in it |
|---|---|
| `cubemx/` | Three STM32CubeMX `.ioc` configuration files |
| `rp2040-reference/` | Python scripts for the earlier Raspberry Pi GPIO keypad and TFT work, two Numpad TMR1 reference archives and one VIA configuration (the RP2040 proof of concept, see [rp2040-proof-of-concept.md](rp2040-proof-of-concept.md)) |
| `binaries/` | Ready-to-flash `.bin` and `.hex` images of the verified firmware steps and the diagnostics, built from the code in `stm32u5a5-keypad/`. See [05-build-flash-test-guide.md](05-build-flash-test-guide.md) before flashing |

Two images from the RP2040 stage (a keyboard header photo and an exploded Hall-switch animation) are not included because they were not made by me and I could not confirm their license.

## Keeping this up to date

After the final presentation, add the final deck and report to `reports/`, update the table above, and replace the placeholder cost slides with the final bill of materials from `hardware/*/production/bom.csv`.
