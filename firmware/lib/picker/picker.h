#pragma once
// Plain C++ only: no Arduino includes, so host (native) tests can use it.
#include <cstddef>
#include <cstdint>

#include "config.h"  // HISTORY_SIZE

// NVS on device, in-memory fake in tests.
struct HistoryStore {
  virtual size_t load(uint16_t* buf, size_t max) = 0;  // returns entries loaded, oldest-first
  virtual void save(const uint16_t* buf, size_t n) = 0;  // entries oldest-first
  virtual ~HistoryStore() = default;
};

using RngFn = uint32_t (*)();

class Picker {
 public:
  // Loads the persisted history from `store`. Both must outlive the Picker.
  Picker(HistoryStore& store, RngFn rng);

  // Uniform-ish index in [0, count) that avoids the last HISTORY_SIZE picks when possible
  // (re-rolls at most HISTORY_SIZE times, then accepts). Records the pick and saves history.
  // count == 0 or 1 returns 0 immediately: no rng draw, nothing recorded, no save.
  uint16_t pick(uint16_t count);

  // Entries currently in the history ring; right after construction, the number loaded.
  size_t historySize() const { return size_; }

 private:
  bool recent(uint16_t idx) const;

  HistoryStore& store_;
  RngFn rng_;
  uint16_t history_[HISTORY_SIZE];  // oldest-first; entries >= count are stale but keep their slot
  size_t size_ = 0;
};
