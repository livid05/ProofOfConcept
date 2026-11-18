#!/usr/bin/env python3

import lgpio
import time
import os
import board
import busio
import digitalio
from datetime import datetime
from PIL import Image, ImageDraw, ImageFont
from adafruit_rgb_display import st7735

# ──────────────────────────────────────────────
#  PASSCODE
# ──────────────────────────────────────────────
PASSCODE = '4000'  # Secret key

# ──────────────────────────────────────────────
#  DATA DIRECTORY & LOG FILE PATHS
#  Resolved once at startup, relative to the
#  parent of the current working directory.
#    cwd        → e.g. /home/pi/myproject
#    parent_dir → e.g. /home/pi
#    data_dir   → e.g. /home/pi/data
#    log_file   → e.g. /home/pi/data/access_log.txt
# ──────────────────────────────────────────────
CWD        = os.getcwd()
PARENT_DIR = os.path.dirname(CWD)
DATA_DIR   = os.path.join(PARENT_DIR, "log")
LOG_FILE   = os.path.join(DATA_DIR, "access_log.txt")


# ──────────────────────────────────────────────
#  TFT DISPLAY SETUP
#  Pins 5, 6, 12 stay clear of Keypad and SPI0
# ──────────────────────────────────────────────
cs_pin    = digitalio.DigitalInOut(board.D5)
dc_pin    = digitalio.DigitalInOut(board.D6)
reset_pin = digitalio.DigitalInOut(board.D12)

# busio.SPI is explicit and reliable on Pi 5
spi = busio.SPI(clock=board.SCLK, MOSI=board.MOSI)

disp = st7735.ST7735R(spi, rotation=270,
                      cs=cs_pin, dc=dc_pin, rst=reset_pin,
                      baudrate=24000000)

# Swap width/height because of rotation=270
width  = disp.height   # physical height becomes logical width
height = disp.width    # physical width  becomes logical height

print("TFT width: %d, height: %d" % (width, height))

# Load font once (reused by every draw call)
font = ImageFont.load_default()


# ──────────────────────────────────────────────
#  TFT HELPER FUNCTIONS
# ──────────────────────────────────────────────
def _blank_image():
    """Return a fresh black RGB image sized to the display."""
    img  = Image.new("RGB", (width, height), color=(0, 0, 0))
    draw = ImageDraw.Draw(img)
    return img, draw


def TFT_setup():
    """Show greeting message on TFT for 2 seconds."""
    img, draw = _blank_image()
    draw.text((10, 30), "Greetings!",   font=font, fill=(255, 255, 255))
    draw.text((10, 50), "From CET-429", font=font, fill=(255, 255, 255))
    disp.image(img)
    time.sleep(2)


def TFT_print(keypad_str):
    """
    Display the entered string and PASSED!/BLOCKED.. result on the TFT.
      - If keypad_str == PASSCODE → hide the string, show 'PASSED!' in green
      - Otherwise                 → show the string,  show 'BLOCKED..' in red
    """
    img, draw = _blank_image()

    # Line 1 – show what was typed (but hide the passcode itself)
    if keypad_str != PASSCODE:
        draw.text((10, 20), keypad_str,   font=font, fill=(255, 255, 0))

    # Line 2 – show result
    if keypad_str == PASSCODE:
        draw.text((10, 50), "PASSED!",    font=font, fill=(255, 0, 0))
        disp.image(img)
        do_approved_task()                # ← run the approved task on success
    else:
        draw.text((10, 50), "BLOCKED..",  font=font, fill=(0, 255, 0))
        disp.image(img)
        log_access(success=False, attempted=keypad_str)  # log failed attempt too

    time.sleep(2)


def TFT_show_typing(keypad_str):
    """Show asterisks on the display while the user is typing their code."""
    img, draw = _blank_image()
    draw.text((10, 20), "Enter code:",        font=font, fill=(200, 200, 200))
    draw.text((10, 50), "*" * len(keypad_str), font=font, fill=(255, 255, 0))
    disp.image(img)


def TFT_task_feedback(line1, line2="", color1=(0, 255, 255), color2=(255, 255, 255)):
    """Generic two-line status message shown during / after the approved task."""
    img, draw = _blank_image()
    draw.text((10, 20), line1, font=font, fill=color1)
    draw.text((10, 50), line2, font=font, fill=color2)
    disp.image(img)
    time.sleep(2)


def TFT_clear():
    """Blank the display."""
    img, _ = _blank_image()
    disp.image(img)


# ──────────────────────────────────────────────
#  LOG HELPER
# ──────────────────────────────────────────────
def log_access(success: bool, attempted: str = ""):
    """
    Append one timestamped line to access_log.txt.
    Called for both successful and failed attempts so there is a full audit trail.
    """
    timestamp = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    if success:
        entry = f"[{timestamp}]  ACCESS GRANTED\n"
    else:
        # Never log the actual PASSCODE – only log non-matching attempts
        entry = f"[{timestamp}]  ACCESS DENIED   (entered: '{attempted}')\n"

    try:
        with open(LOG_FILE, "a") as f:
            f.write(entry)
        print(f"Log updated: {entry.strip()}")
    except OSError as e:
        print(f"WARNING: could not write to log: {e}")


# ──────────────────────────────────────────────
#  APPROVED TASK
# ──────────────────────────────────────────────
def do_approved_task():
    """
    Approved task triggered on correct passcode:

    1. Ensure <parent_of_cwd>/data/  directory exists (create if absent).
    2. Append a timestamped ACCESS GRANTED entry to access_log.txt inside it.
    3. Show live feedback on the TFT at each step.

    Directory layout example
    ────────────────────────
        /home/pi/            ← PARENT_DIR
        /home/pi/myproject/  ← CWD  (wherever the script is run from)
        /home/pi/data/       ← DATA_DIR  (created here if missing)
        /home/pi/data/access_log.txt  ← LOG_FILE
    """
    print("Approved task begins......")

    # — Step 1: Create the data directory if it does not already exist —
    if not os.path.exists(DATA_DIR):
        try:
            os.makedirs(DATA_DIR)
            print(f"Created directory: {DATA_DIR}")
            TFT_task_feedback("data/ created", DATA_DIR[-16:], color1=(0, 255, 255))
        except OSError as e:
            print(f"ERROR creating directory: {e}")
            TFT_task_feedback("Dir error!", str(e)[:20], color1=(255, 0, 0))
            return
    else:
        print(f"Directory already exists: {DATA_DIR}")
        TFT_task_feedback("data/ exists", "OK", color1=(0, 200, 200))

    # — Step 2: Write / update the access log —
    log_access(success=True)

    # — Step 3: Count total entries in the log and show on TFT —
    try:
        with open(LOG_FILE, "r") as f:
            total_entries = sum(1 for line in f if line.strip())
        print(f"Log now has {total_entries} entr{'y' if total_entries == 1 else 'ies'}.")
        TFT_task_feedback(
            "Log updated!",
            f"Entries: {total_entries}",
            color1=(0, 255, 0),
            color2=(200, 200, 200),
        )
    except OSError as e:
        print(f"WARNING: could not read log for count: {e}")

    print("...... Approved task ends.\n")


# ──────────────────────────────────────────────
#  KEYPAD SETUP
#  Pins moved away from TFT / SPI lines
# ──────────────────────────────────────────────
ROWS = [18, 23, 26, 16]
COLS = [13, 19, 20, 21]

KEY_MAP = [
    ["1", "2", "3", "A"],
    ["4", "5", "6", "B"],
    ["7", "8", "9", "C"],
    ["*", "0", "#", "D"],
]

chip = lgpio.gpiochip_open(4)   # gpiochip4 is standard for Pi 5

for pin in ROWS:
    lgpio.gpio_claim_output(chip, pin, 0)   # start LOW

for pin in COLS:
    lgpio.gpio_claim_input(chip, pin)

print("Keypad ready! Press keys (Ctrl+C to stop).")


def read_keypad():
    """Scan the matrix and return the pressed key string, or None."""
    for r_idx, row_pin in enumerate(ROWS):
        lgpio.gpio_write(chip, row_pin, 1)          # drive row HIGH
        for c_idx, col_pin in enumerate(COLS):
            if lgpio.gpio_read(chip, col_pin) == 1:
                lgpio.gpio_write(chip, row_pin, 0)  # clean up before returning
                return KEY_MAP[r_idx][c_idx]
        lgpio.gpio_write(chip, row_pin, 0)          # drive row LOW again
    return None


# ──────────────────────────────────────────────
#  MAIN
# ──────────────────────────────────────────────
try:
    TFT_setup()    # greeting screen for 2 seconds
    TFT_clear()

    last_key     = None
    entered_code = ""   # accumulates digit / character presses

    while True:
        current_key = read_keypad()

        # Only act on a new key-down event (not a held key)
        if current_key and current_key != last_key:
            print(f"Key pressed: {current_key}")

            if current_key == '*':
                # '*' → clear everything and start over
                entered_code = ""
                TFT_clear()

            elif current_key == '#':
                # '#' → submit the accumulated entry
                TFT_print(entered_code)
                entered_code = ""    # reset after submission
                TFT_clear()

            else:
                # Any other key → append to the running string
                entered_code += current_key
                TFT_show_typing(entered_code)   # live asterisk feedback

            print(f"Current entry: {entered_code}")

        last_key = current_key
        time.sleep(0.1)

except KeyboardInterrupt:
    print("\nCleaning up...")

finally:
    TFT_clear()
    lgpio.gpiochip_close(chip)
    print("Done.")
