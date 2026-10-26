#include "machine.h"

#include <cstring>

Machine::Machine(Actions& a, Picker& p) : a_(a), picker_(p), rec_{}, path_{} {}

void Machine::goToSleep(bool stopFirst) {
  a_.ampEnable(false);
  if (stopFirst) a_.stop();
  state_ = State::Sleeping;  // set first: sleep() never returns on device, returns in tests
  a_.sleep();
}

void Machine::onWake() {
  if (!a_.storageReady() || !a_.indexLoaded(idx_)) {
    goToSleep(false);
    return;
  }
  a_.ampEnable(true);  // only now: the amp stays off through mount and any index rebuild
  for (int attempt = 0; attempt < MAX_OPEN_RETRIES; attempt++) {
    uint16_t n = picker_.pick(static_cast<uint16_t>(idx_.count()));
    if (!a_.indexRead(idx_, n, rec_)) continue;
    // filename is at most 47 chars + NUL, SONG_DIR is 7: always fits in path_.
    std::strcpy(path_, SONG_DIR);
    std::strcat(path_, rec_.filename);
    if (!a_.play(path_)) continue;
    state_ = State::Playing;
    wakeMillis_ = a_.millis();
    return;
  }
  goToSleep(false);
}

void Machine::tick(bool buttonPressedEdge) {
  if (state_ != State::Playing) return;
  bool stopRequested = buttonPressedEdge && (a_.millis() - wakeMillis_ >= WAKE_IGNORE_MS);
  bool ended = !a_.playing();
  if (stopRequested || ended) goToSleep(stopRequested);
}
