#include "hal_audio.h"

#include <Arduino.h>
#include <Audio.h>
#include <SD.h>
#ifdef SONGBTN_DEBUG
#include <cstring>
#endif

#include "config.h"

// ESP32-audioI2S 4.0.1 (commit 5bf3392) declares these two non-pure virtuals on Decoder but
// never defines them, so Decoder's vtable is unresolved at link. Every codec overrides both;
// these bodies exist only to satisfy the linker. Remove them once upstream defines them.
const char* Decoder::getStreamTitle() { return ""; }
const char* Decoder::whoIsIt() { return ""; }

static Audio audio;
static volatile bool eof = false;

void audioBegin() {
  // This library version has no weak audio_eof_mp3(); events arrive via this callback.
  Audio::audio_info_callback = [](Audio::msg_t m) {
    if (m.e == Audio::evt_eof) eof = true;
#ifdef SONGBTN_DEBUG
    // Delivered from audio.loop(), so this is when loop() saw it, not when the decoder emitted it.
    if (m.e == Audio::evt_info && m.msg && strstr(m.msg, "stream ready"))
      Serial.printf("[%lld ms] stream ready\n", esp_timer_get_time() / 1000);
#endif
  };
  audio.setPinout(PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT);
  audio.setVolumeSteps(21);
  audio.setVolume(VOLUME);
  audio.forceMono(!STEREO);
}

bool audioPlay(const char* path) {
  eof = false;
  bool ok = audio.connecttoFS(SD, path);
#ifdef SONGBTN_DEBUG
  Serial.printf("[%lld ms] connecttoFS returned %d\n", esp_timer_get_time() / 1000, ok);
#endif
  return ok;
}

void audioLoop() { audio.loop(); }

bool audioRunning() { return audio.isRunning() && !eof; }

void audioStop() { audio.stopSong(); }
