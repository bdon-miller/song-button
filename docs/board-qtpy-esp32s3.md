# Can the Adafruit QT Py ESP32-S3 replace the XIAO ESP32-S3?

Date: 2026-10-25. Researched from Adafruit's docs, the board's Eagle
schematic, the Arduino pin header, part datasheets, and independent
measurements. Verdict: **yes, with the BAT pads as the power input.**

## Power path (from the schematic)

- Regulator U1 is an **AP2112K-3.3** with enable tied to input, so it is
  always on. Quiescent current 55 µA typical, 80 µA max. Dropout 250 mV
  typical at the full 600 mA, far less at our ~100 mA. Max input 6 V.
- Two **NSR0320** Schottky diodes OR the supplies: USB → D1 → regulator,
  BAT pad → D2 → regulator. The pack can never backfeed USB and USB can
  never backfeed the pack. Forward drop roughly 0.3 to 0.35 V at 100 mA.
- The **5V header pin is after the diodes**, on the regulator-input node.
  Feeding the pack into it bypasses D2 and loses the isolation. Use the
  **BAT/GND pads on the underside** instead; that is what they are for
  ("external battery packs from 3V to 6V").
- The NeoPixel's VDD is driven **directly by GPIO 38** (NEOPIX_PWR net).
  Pull it low before sleep and the pixel is unpowered.

## Idle current estimate

| Item | µA |
|---|---|
| AP2112 quiescent | 55 typ, 80 max |
| ESP32-S3 deep sleep, RTC pull-up on | 8 to 10 |
| NeoPixel with power pin low | ~0 |
| Diode reverse leakage at 5 V | negligible |
| **Total** | **65 to 90** |

Adafruit's own spec for the board is "deep sleep at ~70 µA", which agrees.
The design budget is 300 µA, so this passes with about 3x margin. For
comparison the XIAO ESP32-S3 measures 13.4 µA (Qoitech, Otii Ace, 3.8 V
on the battery pad). On three AA cells either figure is years of standby;
cell self-discharge dominates.

No independent, bench-measured Arduino deep-sleep number for the QT Py
ESP32-S3 itself was found. The 70 µA is Adafruit's figure. Task 7 of the
firmware plan measures it.

## End-of-life pack voltage

Regulator output 3.3 V + dropout at ~100 mA (~0.1 V, conservative) +
Schottky drop (~0.35 V) ≈ **3.75 V** under load. The XIAO estimate in the
spec is 3.7 V. Same runtime for practical purposes. The amp is fed straight
from the pack, not through the board, so it sees the full pack voltage.
Three fresh alkalines (4.8 V) are inside the regulator's 6 V limit; four
are not.

## Pin map for `config.h`

GPIO numbers from the Arduino core's `adafruit_qtpy_esp32s3_n4r2` variant.
Only GPIO 0 to 21 can wake the chip from deep sleep, so the button must be
on one of A0 to A3, SDA, SCL, TX or RX.

| Signal | QT Py label | GPIO | Note |
|---|---|---|---|
| Button | A0 | 18 | RTC-capable, ext0 wake |
| Amp enable | A1 | 17 | held LOW through sleep |
| SD chip select | A2 | 9 | |
| SD SCK / MISO / MOSI | SCK / MI / MO | 36 / 37 / 35 | hardware SPI, not RTC (fine) |
| I2S BCLK / LRC / DOUT | TX / RX / SCL | 5 / 16 / 6 | |
| NeoPixel power | internal | 38 | drive LOW, hold through sleep |
| NeoPixel data | internal | 39 | unused |

## Firmware changes if switching

1. `platformio.ini`: `board = adafruit_qtpy_esp32s3_n4r2` (PlatformIO knows
   this id; the no-PSRAM variant is `adafruit_qtpy_esp32s3_nopsram`).
2. `include/config.h`: the pin values above, plus `PIN_NEOPIXEL_POWER 38`.
3. `src/hal_power.cpp`: in `deepSleep()`, drive GPIO 38 LOW and `gpio_hold_en`
   it alongside the amp pin (it is not an RTC pin, so it needs the same
   `gpio_deep_sleep_hold_en` treatment the amp pin already gets).
4. BOM: swap the brain; add "solder the pack to the BAT/GND pads".

Everything else is unchanged. The master power switch stays mandatory per
the spec even though the diodes make USB-with-cells safe on this board.

## Other differences

- 4 MB flash / 2 MB PSRAM vs 8 MB / 8 MB. The linked image is about 1.44 MB (the audio library pulls in TLS and codec code), which does not fit the default OTA layout on 4 MB, so the build uses the no-OTA partition table. Fine for this project.
- About $12.50 vs about $8.
- The onboard NeoPixel can serve as the optional press-confirmation blink
  the spec left out, at no extra parts.
- STEMMA QT connector is unused; its 3.3 V is unswitched but nothing is
  plugged in.

## Sources

- Adafruit QT Py ESP32-S3 pinouts: https://learn.adafruit.com/adafruit-qt-py-esp32-s3/pinouts
- Adafruit product page (deep sleep ~70 µA, AP2112 600 mA): https://www.adafruit.com/product/5426
- Eagle schematic (U1 AP2112K-3.3, D1/D2 NSR0320, NEOPIX_PWR on IO38): https://github.com/adafruit/Adafruit-QT-Py-ESP32-S3-PCB
- Arduino variant pins: https://github.com/espressif/arduino-esp32/blob/master/variants/adafruit_qtpy_esp32s3_n4r2/pins_arduino.h
- AP2112 quiescent and dropout: https://www.embeddedrelated.com/parts/p/AP2112K-3.3 and https://www.digikey.com/short/b2m9hq2c
- NSR0320 ratings: https://www.digikey.fr/en/products/detail/onsemi/NSR0320MW2T1G/1218949
- XIAO ESP32-S3 measured 13.4 µA: https://www.qoitech.com/blog/esp32-s3-c3-c6-sleep-power-consumption/
- Feather S3 note on NeoPixel/I2C power for low sleep: https://www.adafruit.com/product/5477
