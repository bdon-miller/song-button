# Wiring

Every wire in the box, both ends. Pin numbers match `firmware/include/config.h`;
if you change a pin there, change it here too. Board: Adafruit QT Py ESP32-S3.

## Overview

```
                 3 x AA pack (4.8 V fresh)
                 + ──[slide switch]──┬─────────────┬──────────────┐
                                     │             │              │
                                 470 µF (+)    QT Py BAT pad   Amp VIN
                                 cap  (−)      (underside)
                 − ──────────────────┴─────── QT Py GND pad ─── Amp GND
                                                (underside)

  QT Py ESP32-S3                         MAX98357A amp
  ┌──────────────┐                       ┌────────────┐
  │ TX  (GPIO 5) ├───────────────────────┤ BCLK       │
  │ RX  (GPIO16) ├───────────────────────┤ LRC        │
  │ SCL (GPIO 6) ├───────────────────────┤ DIN        │      ┌─────────┐
  │ A1  (GPIO17) ├───────────────────────┤ SD         │  +  ─┤ JST ─ speaker +
  │              │                       │ GAIN  (nc) │  −  ─┤ JST ─ speaker −
  │              │                       └────────────┘      └─────────┘
  │              │                       microSD breakout
  │ 3V           ├───────────────────────┤ 3V         │
  │ GND          ├───────────────────────┤ GND        │
  │ SCK (GPIO36) ├───────────────────────┤ CLK        │
  │ MI  (GPIO37) ├───────────────────────┤ SO / MISO  │
  │ MO  (GPIO35) ├───────────────────────┤ SI / MOSI  │
  │ A2  (GPIO 9) ├───────────────────────┤ CS         │
  │              │                       │ DET   (nc) │
  │              │                       └────────────┘
  │ A0  (GPIO18) ├──── button ──── GND
  └──────────────┘
```

## Wire list

| # | From | To | Note |
|---|------|----|------|
| 1 | Pack + | Slide switch | Most AA holders have the switch built in |
| 2 | Switch out | QT Py **BAT** pad (underside) | Not the 5V pin: that bypasses the board's protection diode |
| 3 | Switch out | Amp VIN | Amp runs straight off the pack, not the 3.3 V rail |
| 4 | Pack − | QT Py **GND** pad (underside) | |
| 5 | Pack − | Amp GND | |
| 6 | 470 µF cap + / − | Amp VIN / Amp GND | Solder at the amp end. Stripe on the can marks − |
| 7 | QT Py TX (GPIO 5) | Amp BCLK | |
| 8 | QT Py RX (GPIO 16) | Amp LRC | |
| 9 | QT Py SCL (GPIO 6) | Amp DIN | |
| 10 | QT Py A1 (GPIO 17) | Amp SD | Enable. Firmware holds it low in sleep |
| 11 | Amp + / − | JST socket | Speaker plugs in here |
| 12 | QT Py 3V | SD breakout 3V | Card stays powered in sleep by design |
| 13 | QT Py GND | SD breakout GND | |
| 14 | QT Py SCK (GPIO 36) | SD CLK | |
| 15 | QT Py MI (GPIO 37) | SD SO / MISO | |
| 16 | QT Py MO (GPIO 35) | SD SI / MOSI | |
| 17 | QT Py A2 (GPIO 9) | SD CS | |
| 18 | QT Py A0 (GPIO 18) | Button, one leg | Firmware enables the pull-up; no resistor needed |
| 19 | Button, other leg | GND | Any GND point |

Leave unconnected: amp GAIN (floating = 9 dB, the spec's middle setting),
SD breakout DET, the STEMMA QT socket.

## Notes

- **Amp SD pin and mono.** Driven to 3.3 V, the MAX98357A plays only the left
  channel. The firmware sums the song to mono first (`STEREO 0` in `config.h`
  sets `forceMono`), so left carries the whole mix. No resistor divider is
  needed. The Adafruit board's 1 MΩ pull-up on SD to VIN leaks about 5 µA into
  the held-low pin during sleep, inside the budget.
- **BAT pads.** They are small. Tin the pad and the wire separately, then join
  them with a quick touch. Strain-relieve the battery leads with a dab of hot
  glue so a tug can't lift the pad.
- **Stereo upgrade path.** If you think you'll add a second amp later, run an
  extra wire from a spare pad (A3, GPIO 8) to the amp header now, per the
  spec's upgrade hooks. It is unused by the current firmware.
- **USB with cells in.** Safe on this board (diodes on both inputs), but switch
  the pack off before plugging in anyway.
