#include <unity.h>

#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

#include "machine.h"

struct MemStore : HistoryStore {
  size_t load(uint16_t*, size_t) override { return 0; }
  void save(const uint16_t*, size_t) override {}
};

static uint32_t zeroRng() { return 0; }

// Serves a valid 10-song index header; records are never read through it.
struct HeaderReader : IndexReader {
  bool readAt(uint32_t offset, uint8_t* buf, size_t len) override {
    char header[INDEX_HEADER_LEN];
    std::memset(header, ' ', sizeof(header));
    std::memcpy(header, "SONGBUTTON 1 10", 15);
    header[INDEX_HEADER_LEN - 1] = '\n';
    if (offset + len > sizeof(header)) return false;
    std::memcpy(buf, header + offset, len);
    return true;
  }
};

struct FakeActions : Actions {
  std::vector<std::string> log;
  bool storage = true;
  bool loaded = true;
  bool readOk = true;
  std::deque<bool> playResults;  // popped per play(); true once exhausted
  bool isPlaying = true;
  uint32_t now = 0;
  HeaderReader reader;

  bool storageReady() override { return storage; }
  bool indexLoaded(SongIndex& idx) override { return loaded && idx.load(reader); }
  bool indexRead(const SongIndex&, uint32_t, SongRecord& out) override {
    if (!readOk) return false;
    std::strcpy(out.filename, "0007-x.mp3");
    std::strcpy(out.title, "x");
    return true;
  }
  bool play(const char* path) override {
    log.push_back(std::string("play(") + path + ")");
    if (playResults.empty()) return true;
    bool r = playResults.front();
    playResults.pop_front();
    return r;
  }
  bool playing() override { return isPlaying; }
  void stop() override { log.push_back("stop"); }
  void ampEnable(bool on) override { log.push_back(on ? "ampEnable(true)" : "ampEnable(false)"); }
  void sleep() override { log.push_back("sleep"); }
  uint32_t millis() override { return now; }
};

static void assertLog(const FakeActions& f, std::vector<std::string> expected) {
  TEST_ASSERT_EQUAL_UINT(expected.size(), f.log.size());
  for (size_t i = 0; i < expected.size() && i < f.log.size(); i++)
    TEST_ASSERT_EQUAL_STRING(expected[i].c_str(), f.log[i].c_str());
}

static size_t count(const FakeActions& f, const char* what) {
  size_t n = 0;
  for (auto& s : f.log)
    if (s == what) n++;
  return n;
}

void setUp() {}
void tearDown() {}

void test_wake_plays_random_song() {
  FakeActions f;
  MemStore store;
  Picker p(store, zeroRng);
  Machine m(f, p);
  m.onWake();
  assertLog(f, {"ampEnable(true)", "play(/songs/0007-x.mp3)"});
  TEST_ASSERT_TRUE(m.state() == State::Playing);
}

void test_wake_no_card_sleeps() {
  FakeActions f;
  f.storage = false;
  MemStore store;
  Picker p(store, zeroRng);
  Machine m(f, p);
  m.onWake();
  assertLog(f, {"ampEnable(false)", "sleep"});
  TEST_ASSERT_TRUE(m.state() == State::Sleeping);
}

void test_wake_retries_unopenable_then_sleeps() {
  FakeActions f;
  f.playResults = {false, false, false, false, false};
  MemStore store;
  Picker p(store, zeroRng);
  Machine m(f, p);
  m.onWake();
  TEST_ASSERT_EQUAL_UINT(5, count(f, "play(/songs/0007-x.mp3)"));
  TEST_ASSERT_EQUAL_STRING("ampEnable(false)", f.log[f.log.size() - 2].c_str());
  TEST_ASSERT_EQUAL_STRING("sleep", f.log.back().c_str());
  TEST_ASSERT_EQUAL_UINT(1, count(f, "sleep"));
  TEST_ASSERT_TRUE(m.state() == State::Sleeping);
}

void test_wake_retry_succeeds_on_second() {
  FakeActions f;
  f.playResults = {false, true};
  MemStore store;
  Picker p(store, zeroRng);
  Machine m(f, p);
  m.onWake();
  TEST_ASSERT_EQUAL_UINT(2, count(f, "play(/songs/0007-x.mp3)"));
  TEST_ASSERT_EQUAL_UINT(0, count(f, "sleep"));
  TEST_ASSERT_TRUE(m.state() == State::Playing);
}

void test_press_before_300ms_ignored() {
  FakeActions f;
  MemStore store;
  Picker p(store, zeroRng);
  Machine m(f, p);
  f.now = 1000;
  m.onWake();
  f.now = 1299;
  m.tick(true);
  TEST_ASSERT_TRUE(m.state() == State::Playing);
  assertLog(f, {"ampEnable(true)", "play(/songs/0007-x.mp3)"});
}

void test_press_after_300ms_stops() {
  FakeActions f;
  MemStore store;
  Picker p(store, zeroRng);
  Machine m(f, p);
  f.now = 1000;
  m.onWake();
  f.now = 1300;
  m.tick(true);
  assertLog(f, {"ampEnable(true)", "play(/songs/0007-x.mp3)", "ampEnable(false)", "stop", "sleep"});
  TEST_ASSERT_TRUE(m.state() == State::Sleeping);
}

void test_eof_sleeps() {
  FakeActions f;
  MemStore store;
  Picker p(store, zeroRng);
  Machine m(f, p);
  m.onWake();
  f.isPlaying = false;
  m.tick(false);
  assertLog(f, {"ampEnable(true)", "play(/songs/0007-x.mp3)", "ampEnable(false)", "sleep"});
  TEST_ASSERT_TRUE(m.state() == State::Sleeping);
}

void test_eof_and_press_same_tick_one_sleep() {
  FakeActions f;
  MemStore store;
  Picker p(store, zeroRng);
  Machine m(f, p);
  m.onWake();
  f.now = 5000;
  f.isPlaying = false;
  m.tick(true);
  TEST_ASSERT_EQUAL_UINT(1, count(f, "sleep"));
  m.tick(true);  // already sleeping: no further calls
  TEST_ASSERT_EQUAL_UINT(1, count(f, "sleep"));
  TEST_ASSERT_TRUE(m.state() == State::Sleeping);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_wake_plays_random_song);
  RUN_TEST(test_wake_no_card_sleeps);
  RUN_TEST(test_wake_retries_unopenable_then_sleeps);
  RUN_TEST(test_wake_retry_succeeds_on_second);
  RUN_TEST(test_press_before_300ms_ignored);
  RUN_TEST(test_press_after_300ms_stops);
  RUN_TEST(test_eof_sleeps);
  RUN_TEST(test_eof_and_press_same_tick_one_sleep);
  return UNITY_END();
}
