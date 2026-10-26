#pragma once
// Plain C++ only: no Arduino includes, so host (native) tests can use it.
#include <cstdint>

#include "picker.h"
#include "songindex.h"

// Implemented by main.cpp over the HAL, faked in tests.
struct Actions {
  virtual bool storageReady() = 0;  // card mounted and index loaded (builds fallback index if missing)
  virtual bool indexLoaded(SongIndex& idx) = 0;
  virtual bool indexRead(const SongIndex& idx, uint32_t n, SongRecord& out) = 0;
  virtual bool play(const char* path) = 0;  // false if file cannot be opened
  virtual bool playing() = 0;               // false once decoder hits EOF
  virtual void stop() = 0;
  virtual void ampEnable(bool on) = 0;
  virtual void sleep() = 0;  // never returns on device
  virtual uint32_t millis() = 0;
  virtual ~Actions() = default;
};

enum class State { Booting, Playing, Sleeping };

class Machine {
 public:
  Machine(Actions& a, Picker& p);
  void onWake();                      // called once from setup(); ends in Playing or Sleeping
  void tick(bool buttonPressedEdge);  // called every loop(); edge = new press after release, debounced
  State state() const { return state_; }

 private:
  void goToSleep(bool stopFirst);

  Actions& a_;
  Picker& picker_;
  SongIndex idx_;
  SongRecord rec_;
  char path_[64];
  static_assert(sizeof(SONG_DIR) - 1 + sizeof(SongRecord::filename) <= sizeof(path_),
                "SONG_DIR + longest filename + NUL must fit in path_");
  State state_ = State::Booting;
  uint32_t wakeMillis_ = 0;
};
