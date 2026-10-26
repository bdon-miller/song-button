# Bill of Materials and Size Estimate

Prices are approximate US retail as of late 2026 and should be rechecked
before ordering. Links are to the reference part; equivalents are fine
where the spec allows.

## Electronics

| # | Part | Reference | Qty | Est. | Notes |
|---|------|-----------|-----|------|-------|
| 1 | Adafruit QT Py ESP32-S3 (4MB flash / 2MB PSRAM) | Adafruit 5700 | 1 | $12.50 | Solder the pack to the BAT/GND pads on the underside |
| 2 | MAX98357A I2S amp breakout | Adafruit 3006 | 1 | $6 | Clones work, ~$2 |
| 3 | microSD breakout, SPI | Adafruit 4682 or generic | 1 | $4 | 3.3 V logic, no level shifter |
| 4 | microSD card, 32 GB | SanDisk Ultra or Samsung EVO | 1 | $8 | FAT32; verify idle draw under 100 µA |
| 5 | Speaker, 3 in, 4 Ω, 3 W full range | Adafruit 1314 or Dayton Audio equivalent | 1 | $6 | 2 in acceptable if box is tight |
| 6 | Arcade button, 60 mm momentary | generic "big dome" arcade button | 1 | $5 | 30 mm (~$3) if space-limited |
| 7 | 3 x AA holder with slide switch | generic | 1 | $2 | Switch is mandatory per spec |
| 8 | Electrolytic capacitor, 470 µF 10 V | generic | 1 | $1 | Across pack near amp |
| 9 | JST-XH 2-pin pair | generic | 1 | $1 | Speaker plug |
| 10 | Female header strip, 2.54 mm | generic | 2 | $1 | Amp board socket |
| 11 | Hookup wire, 24 AWG | generic | — | $3 | |
| 12 | USB-C cable for flashing | already owned | 1 | $0 | |

Electronics subtotal: about $50.

## Enclosure

| Part | Est. | Notes |
|------|------|-------|
| 3D-printed box and removable baffle | $3 filament | Preferred; sealed, 3 mm walls |
| or ABS project box ~120 x 100 x 60 mm | $8 | Needs a cut baffle plate |
| Speaker grille cloth or printed grille | $2 | |
| Light polyfill stuffing | $1 | |
| Keyhole hanger or adhesive strip | $2 | Wall mount |
| 3 x AA alkaline cells | $3 | |

Total build: roughly $60 to $70.

## Size estimate

Driven by the speaker, the button depth, and the battery holder.

| Item | Footprint | Depth |
|------|-----------|-------|
| 3 in speaker | 78 mm diameter | 35 mm |
| 60 mm arcade button | 60 mm diameter | 55 mm behind panel |
| 3 x AA holder | 58 x 48 mm | 17 mm |
| QT Py ESP32-S3 | 22 x 18 mm | 6 mm |
| MAX98357A on headers | 25 x 18 mm | 12 mm |
| microSD breakout | 25 x 25 mm | 5 mm |

A sealed box for a 3 in driver wants roughly 0.5 to 1 L of internal
volume. Putting the button beside the speaker on the front face and the
batteries behind gives a box around:

- **130 x 100 x 65 mm** (about 5 x 4 x 2.5 in), roughly 0.6 L internal
- weight with batteries about 300 g

With a 30 mm button and a 2 in driver the box can shrink to about
100 x 80 x 50 mm, at the cost of bass.
