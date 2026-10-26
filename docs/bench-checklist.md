# Bench checklist

Covers firmware plan Task 6 steps 2 and 3 and all of Task 7, plus the
checks the final code review added. Board: Adafruit QT Py ESP32-S3
(4 MB / 2 MB). Do these in order.

## Before flashing

- Wire per `docs/wiring.md`. Pack to the BAT/GND pads on the underside,
  not the 5 V pin.
- On first battery insertion, listen: there should be no pop or hiss before
  the first press (`powerBegin()` drives the amp enable low at cold boot).
- Card prepared with `cardprep` and at least 20 songs. Keep a second card
  with the full library for step 6.

## Flash and first play (debug build)

1. Battery switch OFF, USB in. `pio run -e qtpy_debug -t upload`, then
   `pio device monitor`. Press the button: a song plays within about 1 s.
   The log shows the timestamp chain: setup, SD mounted, index open,
   connecttoFS, stream ready, and "history entries loaded: N".
2. Press during playback: it stops within one loop and the log ends (the
   board slept). Press again: a different song. This is the first press
   after a real deep-sleep wake; if stop only fails from here on, the
   button pad is still on the RTC mux (`rtc_gpio_deinit` path).
3. Let a song run out: silence, no second song, log shows eof then sleep.
4. History: play three songs. On the fourth wake the log must say
   3 entries loaded. If it says 0 every time, NVS is being read before
   it is initialised.
5. Pull the card, press: nothing plays, sleeps within a second. Reinsert a
   card with no `index.txt`: slow first press, then plays. Afterwards
   `index.txt` exists, `head -c 32` shows `SONGBUTTON 1 20` padded, and no
   `._` names appear in it.
6. Repeat step 5 with the full library card and time it. Expect a second
   or two, not minutes.
7. Pick a very short file (under 64 KB) and a long one. Both must start
   and both must end in sleep.
8. Hold the button down through a wake for 2 s, release, wait, press: must
   stop. Hold through end of file: box sleeps and must not wake until a
   release and a new press.

## Measurements (release build)

9. `pio run -e qtpy -t upload`. USB out, cells in, switch on.
10. Deep-sleep current with the meter in series on the pack, button
    released: target under 300 µA, expect roughly 70 to 90 µA. If over:
    first suspect floating SD lines, then the card (try another brand),
    then the amp enable (should read 0 V on A1 while asleep), then the
    NeoPixel power pin (should read 0 V on GPIO 38).
11. Press-to-sound over 10 presses on 60 fps video: mean under 500 ms,
    max under 1 s. If over, flash the debug build and read the gap between
    "connecttoFS" and "stream ready"; that is the SD/directory term. The
    gap before "setup" is boot and is not tunable from firmware. The next
    lever is 8.3 filenames in cardprep.
12. Playback current at `VOLUME` with a loud track: expect 150 to 250 mA.
13. Listening pass in the real box. Adjust `VOLUME` in `config.h` once,
    reflash, record the value and why.
14. Record all four numbers, the card model, and the pack voltage in
    `docs/bench-results.md`.
