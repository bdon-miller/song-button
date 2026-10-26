#pragma once
// Plain C++ only: no Arduino includes, so host (native) tests can use it.

// Tunables
#define VOLUME 17  // library scale 0..21
#define STEREO 0  // One MAX98357A with SD_MODE at 3.3 V plays only the LEFT channel, so
                  // STEREO=1 with one amp is left-only. STEREO=0 sets forceMono, making
                  // both channels identical.

// Pins (Adafruit QT Py ESP32-S3; labels are the silkscreen names)
#define PIN_BUTTON 18          // A0, RTC-capable, active low, internal pull-up
#define PIN_AMP_ENABLE 17      // A1, high = amp on
#define PIN_SD_CS 9            // A2
#define PIN_SD_SCK 36          // SCK
#define PIN_SD_MISO 37         // MI
#define PIN_SD_MOSI 35         // MO
#define PIN_I2S_BCLK 5         // TX
#define PIN_I2S_LRC 16         // RX
#define PIN_I2S_DOUT 6         // SCL
#define PIN_NEOPIXEL_POWER 38  // internal, NeoPixel VDD; held low so the pixel is unpowered

// Timing
#define DEBOUNCE_MS 50
#define WAKE_IGNORE_MS 300

// Pick history and retries
#define HISTORY_SIZE 50
#define MAX_OPEN_RETRIES 5
#define NVS_NAMESPACE "songbtn"
#define NVS_HISTORY_KEY "hist"

// Card layout and index format
#define INDEX_PATH "/index.txt"
#define SONG_DIR "/songs/"
#define INDEX_HEADER_LEN 32
#define INDEX_RECORD_LEN 128
