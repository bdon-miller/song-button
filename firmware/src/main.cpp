#include <Arduino.h>

#include "hal_audio.h"
#include "hal_power.h"
#include "hal_storage.h"
#include "machine.h"
#include "picker.h"

#ifdef SONGBTN_DEBUG
#define STAMP(what) Serial.printf("[%lld ms] %s\n", esp_timer_get_time() / 1000, what)
#else
#define STAMP(what) ((void)0)
#endif

class HalActions : public Actions {
 public:
  bool storageReady() override {
    storageBegin();
    STAMP("storageBegin done");
    bool opened = reader_.open();
    STAMP("reader_.open done");
    if (opened) return true;
    buildFallbackIndex();
    return reader_.open();
  }
  bool indexLoaded(SongIndex& idx) override { return idx.load(reader_); }
  bool indexRead(const SongIndex& idx, uint32_t n, SongRecord& out) override {
    return idx.read(reader_, n, out);
  }
  bool play(const char* path) override { return audioPlay(path); }
  bool playing() override { return audioRunning(); }
  void stop() override { audioStop(); }
  void ampEnable(bool on) override { ::ampEnable(on); }
  void sleep() override { deepSleep(); }
  uint32_t millis() override { return ::millis(); }

 private:
  SdIndexReader reader_;
};

// Neither constructor touches hardware or NVS, so these are safe as globals.
HalActions actions;
NvsHistoryStore store;

// Picker loads history from NVS in its constructor, which must not run during static init
// (NVS is not initialised yet), so it and Machine are built in setup().
static Machine* machine = nullptr;

void setup() {
  powerBegin();  // first: releases sleep holds before SPI.begin / setPinout touch those pads
#ifdef SONGBTN_DEBUG
  Serial.begin(115200);
#endif
  STAMP("setup entry");
  static Picker picker(store, rngU32);
#ifdef SONGBTN_DEBUG
  Serial.printf("history entries loaded: %u\n", (unsigned)picker.historySize());
#endif
  static Machine m(actions, picker);
  machine = &m;
  audioBegin();  // configures I2S only; onWake() enables the amp via Actions::ampEnable
  machine->onWake();
}

void loop() {
  audioLoop();
  machine->tick(buttonPressedEdge());
}
