#!/usr/bin/env python3

import lgpio
import time
import board
import busio
import digitalio
from PIL import Image, ImageDraw, ImageFont
from adafruit_rgb_display import st7735


# ─────────────────────────────────────────────
# PASSCODE
# ─────────────────────────────────────────────
PASSCODE = '4000'  # Secret key


# ─────────────────────────────────────────────
# TFT DISPLAY SETUP
# Pins 5, 6, 12 stay clear of Keypad and SPI0
# ─────────────────────────────────────────────
cs_pin = digitalio.DigitalInOut(board.D5)
dc_pin = digitalio.DigitalInOut(board.D6)
reset_pin = digitalio.DigitalInOut(board.D12)

# Use busio.SPI explicitly (more reliable on Pi 5 than board.SPI())
spi = busio.SPI(clock=board.SCLK, MOSI=board.MOSI)

disp = st7735.ST7735R(spi, rotation=270,
                     cs=cs_pin, dc=dc_pin, rst=reset_pin,
                     baudrate=24000000)

# Swap width/height because of rotation=270
width = disp.height   # physical height becomes logical width
height = disp.width   # physical width becomes logical height

print("TFT width: %d, height: %d" % (width, height))

# Load font once (reused by every draw call)
font = ImageFont.load_default()


def _blank_image():
    """Return a fresh black RGB image sized to the display."""
    img = Image.new("RGB", (width, height), color=(0, 0, 0))
    draw = ImageDraw.Draw(img)
    return img, draw


def TFT_setup():
    """Show greeting message on TFT for 2 seconds (mirrors LCD_setup)."""
    img, draw = _blank_image()
    draw.text((10, 30), "Greetings!", font=font, fill=(255, 255, 255))
    draw.text((10, 50), "From CET-429", font=font, fill=(255, 255, 255))
    disp.image(img)
    time.sleep(2)


def TFT_print(keypad_str):
    """
    Display the entered string and PASSED!/BLOCKED.. result on the TFT.
    Mirrors LCD_print():
      - If keypad_str == PASSCODE -> hide the string, show 'PASSED!'
      - Otherwise                 -> show the string, show 'BLOCKED..'
    """
    img, draw = _blank_image()

    # Row 0 — show what was typed (but hide the passcode itself)
    if keypad_str != PASSCODE:
        draw.text((10, 20), keypad_str, font=font, fill=(255, 255, 0))

    # Row 1 — show result
    if keypad_str == PASSCODE:
        draw.text((10, 50), "PASSED!", font=font, fill=(255, 0, 0))
        disp.image(img)
        do_approved_task()
    else:
        draw.text((10, 50), "BLOCKED..", font=font, fill=(0, 255, 0))
        disp.image(img)

    time.sleep(2)


def TFT_show_typing(keypad_str):
    """Update the display while the user is still typing."""
    img, draw = _blank_image()
    draw.text((10, 20), "Enter code:", font=font, fill=(200, 200, 200))
    # Show asterisks so the code stays hidden while typing
    draw.text((10, 50), "*" * len(keypad_str), font=font, fill=(255, 255, 0))
    disp.image(img)


def TFT_clear():
    """Blank the display (mirrors LCD_clear)."""
    img, _ = _blank_image()
    disp.image(img)


def do_approved_task():
    """Placeholder for whatever should happen on a correct passcode."""
    print("Approved task begins......")
    print(".....")
    print("...... Approved task ends.\n\n")


# ─────────────────────────────────────────────
# KEYPAD SETUP
# Pins are intentionally away from TFT / SPI lines
# ─────────────────────────────────────────────
ROWS = [18, 23, 26, 16]
COLS = [13, 19, 20, 21]

KEY_MAP = [
    ["1", "2", "3", "A"],
    ["4", "5", "6", "B"],
    ["7", "8", "9", "C"],
    ["*", "0", "#", "D"],
]

chip = lgpio.gpiochip_open(4)  # gpiochip4 is standard for Pi 5

for pin in ROWS:
    lgpio.gpio_claim_output(chip, pin, 0)  # start LOW

for pin in COLS:
    lgpio.gpio_claim_input(chip, pin)

print("Keypad ready! Press keys (Ctrl+C to stop).")


def read_keypad():
    """Scan the matrix and return the pressed key string, or None."""
    for row_idx, row_pin in enumerate(ROWS):
        lgpio.gpio_write(chip, row_pin, 1)  # drive row HIGH
        for col_idx, col_pin in enumerate(COLS):
            if lgpio.gpio_read(chip, col_pin) == 1:
                lgpio.gpio_write(chip, row_pin, 0)  # clean up before returning
                return KEY_MAP[row_idx][col_idx]
        lgpio.gpio_write(chip, row_pin, 0)  # drive row LOW again
    return None


# ─────────────────────────────────────────────
# MAIN
# ─────────────────────────────────────────────
try:
    TFT_setup()        # greeting screen for 2 seconds
    TFT_clear()

    last_key = None
    entered_code = ""  # accumulates digit/character presses

    while True:
        current_key = read_keypad()

        # Only act on a new key-down event (not a held key)
        if current_key and current_key != last_key:
            print(f"Key pressed: {current_key}")

            if current_key == '*':
                # '*' = clear everything and start over
                entered_code = ""
                TFT_clear()

            elif current_key == '#':
                # '#' = submit what was entered
                TFT_print(entered_code)
                entered_code = ""  # reset after submission
                TFT_clear()

            else:
                # Any other key → append to the running string
                entered_code += current_key
                TFT_show_typing(entered_code)  # live feedback while typing

            print(f"Current entry: {entered_code}")

        last_key = current_key
        time.sleep(0.1)

except KeyboardInterrupt:
    print("\nCleaning up...")

finally:
    TFT_clear()
    lgpio.gpiochip_close(chip)
    print("Done.")
