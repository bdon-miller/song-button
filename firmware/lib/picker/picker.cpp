#include "picker.h"

Picker::Picker(HistoryStore& store, RngFn rng) : store_(store), rng_(rng) {
  size_ = store_.load(history_, HISTORY_SIZE);
  if (size_ > HISTORY_SIZE) size_ = HISTORY_SIZE;
}

bool Picker::recent(uint16_t idx) const {
  for (size_t i = 0; i < size_; i++) {
    if (history_[i] == idx) return true;
  }
  return false;
}

uint16_t Picker::pick(uint16_t count) {
  if (count <= 1) return 0;  // nothing to choose: no rng draw, no history entry

  uint16_t choice = static_cast<uint16_t>(rng_() % count);
  for (int retry = 0; retry < HISTORY_SIZE && recent(choice); retry++) {
    choice = static_cast<uint16_t>(rng_() % count);
  }

  if (size_ == HISTORY_SIZE) {
    for (size_t i = 1; i < size_; i++) history_[i - 1] = history_[i];
    size_--;
  }
  history_[size_++] = choice;
  store_.save(history_, size_);
  return choice;
}
