# Random Song Button: Design

Date: 2026-10-25
Status: builder check complete, awaiting owner approval

## Intent

A small box that lives on a wall or desk for months on AA cells. It has one
large button. Press it and, within about half a second, a random song from a
library of roughly 1000 MP3s starts playing through a built-in speaker at
conversational volume for two or three people. The song plays to the end and
the box goes back to sleep. Press again while a song is playing and it stops
immediately.

The point is zero friction: no phone, no pairing, no menus, no charging
cable. Hit the button, get a song.

## Behaviour (decided by the owner)

The button is the only control. The box has exactly two states.

| State   | Button press                                   | Song ends          |
|---------|------------------------------------------------|--------------------|
| Asleep  | Wake, pick a random song, start playing        | n/a                |
| Playing | Stop playback immediately, go back to sleep    | Go back to sleep   |

- **One song and done is a hard requirement.** The box never plays a second
  song on its own.
- "Stop" means stop, not pause. The next press always starts a new random
  song, never resumes the old one.
- Wake-to-first-audio target is 500 ms or less. 1 s is the limit.
- Mono audio. No volume control, no display, no status LED.
- Power source is AA cells only. No lithium pack, no charging port.

## Goals

- Months of standby on one set of AA cells with a few plays per day.
- Instant-feeling response from a fully asleep state.
- Audio quality on par with a small Bluetooth speaker: clear vocals and
  mids, limited bass, no hiss at rest.
- Upgradable speaker and amp without rebuilding the box.

## Non-goals

- Loud enough for a party or a large room.
- Stereo (designed-for, not built).
- Playlists, favourites, skip-back, or any control beyond the single button.
- Wireless anything.
- Charging in place.

## Anti-goals (what would make it a failure even if it "works")

- Batteries dead after a few weeks of idle. Idle draw must be tens of
  microamps, not milliamps.
- A noticeable pause after the press. A 2 to 3 second boot kills the feel.
- A second song starting after the first ends.
- Having to rename 1000 files to a special scheme to load the card.

## Constraints

- 3 AA alkaline cells in series: 4.8 V fresh. The QT Py's AP2112 regulator
  plus its BAT-pad Schottky need about 0.45 V of headroom, so the box is
  effectively dead when the pack sags to about 3.75 V under load. That is an estimate, not a
  requirement; the owner has no stated minimum runtime per set. Every part
  must run directly across 3.7 to 4.8 V without a boost converter.
- Playback current budget: 150 to 250 mA average. Three AAs give roughly
  8 to 12 hours of total music per set. Add bulk capacitance (470 µF or
  more) across the pack near the amp so tired cells do not brown out the
  chip on bass transients.
- Idle current budget: under 300 µA total for the whole box, card included.
  The goal is months of standby; 300 µA is about a year on 2500 mAh cells,
  so there is no need to chase a lower figure at the cost of wake time.
  (Author's call after builder check: an earlier draft said 50 µA, which
  would have forced cold-mounting the card on every press and eaten the
  wake budget.)
- Enclosure size is the builder's call but must fit a 3 inch driver on the
  front baffle so a speaker upgrade does not need a new box.

## Hardware (owner chose route; part selection is the author's call)

The owner chose the "microcontroller decodes MP3 in software" route over a
dedicated MP3 module, because the cheap modules take 1 to 3 seconds to boot
and index a 1000-file card and cannot be kept powered within the idle budget.

### Parts

| Part | Choice | Why |
|------|--------|-----|
| Brain | Adafruit QT Py ESP32-S3 (4MB/2MB) | Native USB for flashing; BAT pads feed the regulator through a Schottky that isolates the pack from USB; ~70 µA deep sleep, inside the 300 µA budget |
| Amp | MAX98357A I2S class D breakout (Adafruit or equivalent) | 16-bit I2S input, 2.5 W into 4 Ω at 4.8 V, runs 2.5 to 5.5 V, has an enable/shutdown pin, sums stereo to mono by default |
| Speaker | 4 Ω, 3 W full-range driver, 2 to 3 inch | Matches amp output; 3 inch preferred if the box allows |
| Storage | microSD breakout, SPI wiring, with a named 16 or 32 GB SanDisk or Samsung card | Holds the MP3 library; card must be FAT32 (32 GB max) and idle under 100 µA, verified with a meter |
| Button | Large momentary arcade-style button, normally open | Satisfying to press, easy to hit on a wall |
| Power | 3 x AA holder, switched or unswitched | Owner requirement |
| Connectors | 2-pin JST for speaker, female header strips for the amp board | Upgrade hooks, see below |

The ESP32-S3 drives the amp over I2S and the card over SPI. The amp's
enable pin is driven by a GPIO and held low whenever the chip is asleep or
not playing, so the amp contributes nothing to idle draw.

### Power

- The AA pack feeds the QT Py's BAT/GND pads on the underside (not the 5V
  pin, which bypasses the BAT Schottky) and the amp's VIN directly.
- The QT Py's on-board regulator supplies 3.3 V to the card breakout.
- No extra reverse-polarity diode. The QT Py's BAT pad already has a
  Schottky (about 0.35 V), and a second one would waste more of a budget
  that has none. Use a keyed battery holder instead.
- The card stays powered in deep sleep. Card idle draw varies from under
  100 µA to over 1 mA by brand, so the parts list names a specific card
  known to idle low (author's call: a current SanDisk or Samsung 16 or 32 GB
  card, verified with a meter during build). Cards over 32 GB are
  excluded; see the card preparation section.
- A master power switch on the battery pack is mandatory, not optional.
  On the QT Py the BAT-pad Schottky isolates the pack from USB in both
  directions, so plugging in USB with cells installed is safe on this
  board; the switch stays for storage and for any board without that
  diode. Still switch off before USB as a habit.
- Builder should measure idle draw with a meter after assembly and treat
  anything over 300 µA as a bug to find (usually a hungry card, a pull-up,
  or a floating enable pin).

### Why not a bare module or a different dev board

Most ESP32 dev boards leak 1 to 10 mA through their USB-serial chip and
regulator even in deep sleep, which alone blows the idle budget. The QT Py
ESP32-S3 (about 70 µA, mostly its regulator) and the XIAO ESP32-S3 (about
13 µA measured) both have native USB and no separate serial chip; the owner
chose the QT Py for its diode-protected battery pads. See
docs/board-qtpy-esp32s3.md. A bare module would be lower
still but needs an external programmer and a hand-built regulator, which is
more soldering than this project wants.

### Upgrade hooks (decided with the owner)

1. Speaker on a 2-pin JST plug so a driver swap is two minutes.
2. Amp board on female headers, not soldered to the harness, so a different
   I2S amp or a second MAX98357A for stereo drops in. Route the I2S bus,
   a second enable GPIO, and the channel-select line to the harness so
   stereo needs no new wiring to the brain.
3. The front baffle that holds the driver is a separate removable plate.
   Size the box interior for a 3 inch driver and a small passive radiator
   even if a 2 inch driver goes in first.

## Firmware

Arduino framework on the ESP32-S3, using the ESP32-audioI2S library for MP3
decoding and I2S output. Language and library are the author's call; a
builder who prefers ESP-IDF or a different decoder may substitute as long
as the wake timing holds.

### Wake and sleep

- Deep sleep with the button on an RTC-capable GPIO configured as the wake
  source (active low, internal or external pull-up as the deep-sleep budget
  allows).
- On wake from button: enable amp, mount card, read the index, pick a
  song, start streaming. Target under 500 ms from press to first decoded
  sample. "First audio" means the first decoded sample, not the first
  audible sound; leading silence inside a file is the file's problem and
  the card preparation step trims it. Builder should measure wake time and
  report it. The card is kept powered across sleep (see Power), so the
  mount is warm, not cold.
- Any press during playback is a stop: cut the amp enable first (avoids a
  pop), halt the decoder, go to deep sleep.
- When the decoder reports end of file: cut amp enable, deep sleep.
- Software debounce of about 50 ms on the button. Ignore presses in the
  first 300 ms after wake so the wake press itself does not read as a stop.
- Any error (no card, unreadable file, empty index): go straight back to
  deep sleep. No retries, no error indication. The next press tries again.

### Card preparation (author's call after builder check)

The owner's MP3s come from mixed sources, so a one-time computer-side
script prepares the card. It is part of this project and lives in the
repo. It never touches the owner's original files; it writes copies to the
card. Given a source folder and a card path, it:

1. Formats or verifies the card as FAT32. Cards over 32 GB arrive as
   exFAT, which the Arduino SD stack does not read, so the parts list caps
   at 32 GB.
2. Copies each MP3 under a short ASCII filename (for example a zero-padded
   number plus a slug), since non-ASCII long filenames are a known failure
   in the Arduino FAT build. The original title is recorded in the index.
3. Strips embedded album art and oversized ID3 tags, which sit before the
   first audio frame and cost SPI read time against the wake budget.
4. Trims leading silence and applies loudness normalisation to a common
   target, since the box has a fixed volume and a 1000-song library easily
   spans 10 dB.
5. Writes the index file: a header line with the entry count, then one
   line per song with filename and original title.

This replaces the earlier idea of the firmware walking the folder to
detect a changed card. The reviewer pointed out that counting entries to
detect a change needs the same walk the index exists to avoid.

### Song library and random pick

- Songs live in one folder on the card as written by the preparation
  script. The owner never renames anything by hand.
- The firmware trusts the index. If the index is missing, it walks the
  folder once and builds a minimal index (filenames only) so the box still
  works with a hand-loaded card; this is a fallback, not the normal path.
  The owner confirmed drag-and-drop must keep working this way.
- Files dragged onto an already-prepared card are invisible until the
  index is rebuilt. The documented procedure is: delete the index file
  from the card, and the next press rebuilds it (that press will be slow,
  a second or two). The firmware does not try to detect changes itself.
- On a normal press the firmware reads only the header and the one line it
  needs, so index reads do not add to wake time.
- Pick is uniformly random over the index using the chip's hardware random
  source. Keep a ring of the last 50 picks in non-volatile storage and
  re-roll if the pick is in the ring. If the picked file does not open
  (stale index, removed file), re-roll up to 5 times before giving up and
  sleeping. The ring, its size, and the retry count are the author's
  call; the owner asked only for "random".

### Volume

Fixed in firmware, two layers:

- The MAX98357A's hardware gain pin is strapped to its middle setting
  (9 dB) at build time and never touched again.
- The decoder's software volume (the ESP32-audioI2S library exposes a
  0 to 21 scale) is a single constant at the top of the sketch. Default is
  a few steps below maximum so loud passages do not clip as the cells sag.

Changing the level after the build means editing that constant and
reflashing over USB, with the master switch off first. The owner is the
builder, so the toolchain is at hand. There is no trimmer or hidden
control. Part of first assembly is a listening pass to set the constant
once with the real speaker and box. Stereo versus mono is likewise a
single compile-time flag.

## Audio quality expectations (set with the owner)

Small Bluetooth speaker territory. Decoder and amp are not the bottleneck;
the driver and enclosure are. A sealed, rigid box with light stuffing is the
single biggest contributor. A 2 to 3 inch driver rolls off around 150 to
200 Hz, so bass is implied rather than felt. Mono is acceptable to the
owner. Upgrade order if unsatisfied: bigger driver with passive radiator,
then stereo via a second amp board.

## Left to the builder

- Enclosure material and construction (3D print, wood, project box).
- Exact button model and mounting.
- Driver model within the 2 to 3 inch, 4 Ω, 3 W envelope.
- Wall-mount method (keyhole slot, French cleat, adhesive strip).
- Wiring layout inside the box.
- GPIO assignment, as long as the button pin is RTC-capable.

## Open questions resolved during design

- Why not an iPod shuffle or a commercial MP3 player: no "one song and
  stop" mode, pause is not stop, aging internal batteries, still needs an
  amp and power switching.
- Why not a DFPlayer-class module: 1 to 3 s cold boot, or 15 to 20 mA
  idle if left powered.
- Why 3 AAs, not 2 or 4: 2 is too low for the amp near end of life; 4 fresh
  cells exceed the amp's 5.5 V limit.
