"""
BLE Keyboard Latency Tester
----------------------------
HOLD DOWN any key (don't tap — hold it).
Windows fires repeated keydown events while the key is held.
The minimum interval between those events reflects your BLE connection interval.

Press Escape to stop and see the summary.
"""

import time
import statistics
from pynput import keyboard

intervals = []
last_time = None

def on_press(key):
    global last_time

    # Stop on Escape
    if key == keyboard.Key.esc:
        return False

    now = time.perf_counter() * 1000  # ms

    if last_time is not None:
        interval = now - last_time
        # Filter out gaps from lifting and re-pressing (> 200ms)
        if interval < 200:
            intervals.append(interval)
            print(f"  {interval:7.2f} ms   (n={len(intervals)})")

    last_time = now

def print_summary():
    if len(intervals) < 5:
        print("\nNot enough data — hold a key down longer next time.")
        return

    print("\n" + "="*45)
    print("  LATENCY SUMMARY")
    print("="*45)
    print(f"  Samples   : {len(intervals)}")
    print(f"  Min       : {min(intervals):.2f} ms  ← BLE floor")
    print(f"  Max       : {max(intervals):.2f} ms")
    print(f"  Average   : {statistics.mean(intervals):.2f} ms")
    print(f"  Median    : {statistics.median(intervals):.2f} ms")
    print(f"  Std Dev   : {statistics.stdev(intervals):.2f} ms  ← jitter")
    print("="*45)
    print("\nIdeal BLE minimum: ~7.5 ms")
    print("Ideal 2.4GHz (ESB) minimum: ~0.125 ms (8K polling)")

print("="*45)
print("  BLE Keyboard Latency Tester")
print("="*45)
print("  HOLD DOWN a key until you have 20+ samples.")
print("  Press Escape to stop and see summary.")
print("="*45 + "\n")

try:
    with keyboard.Listener(on_press=on_press) as listener:
        listener.join()
except KeyboardInterrupt:
    pass

print_summary()
