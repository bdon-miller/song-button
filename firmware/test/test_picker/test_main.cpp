#include <unity.h>

#include <deque>
#include <vector>

#include "picker.h"

struct MemStore : HistoryStore {
  std::vector<uint16_t> data;   // what load() returns
  std::vector<uint16_t> saved;  // last save() payload, oldest-first
  int saveCalls = 0;
  size_t load(uint16_t* buf, size_t max) override {
    size_t n = data.size() < max ? data.size() : max;
    for (size_t i = 0; i < n; i++) buf[i] = data[i];
    return n;
  }
  void save(const uint16_t* buf, size_t n) override {
    saved.assign(buf, buf + n);
    saveCalls++;
  }
};

// Scripted rng: pops from the queue, returns 0 once exhausted.
static std::deque<uint32_t> g_rng;
static uint32_t scriptedRng() {
  if (g_rng.empty()) return 0;
  uint32_t v = g_rng.front();
  g_rng.pop_front();
  return v;
}

void setUp() { g_rng.clear(); }
void tearDown() {}

void test_pick_is_in_range() {
  MemStore store;
  Picker p(store, scriptedRng);
  g_rng = {0, 1, 2, 3, 4};
  for (int i = 0; i < 5; i++) TEST_ASSERT_LESS_THAN_UINT16(10, p.pick(10));
}

void test_pick_avoids_recent() {
  MemStore store;
  store.data = {3};
  Picker p(store, scriptedRng);
  g_rng = {3, 7};
  TEST_ASSERT_EQUAL_UINT16(7, p.pick(10));
}

void test_pick_records_history() {
  MemStore store;
  Picker p(store, scriptedRng);
  g_rng = {4};
  uint16_t r = p.pick(10);
  TEST_ASSERT_EQUAL_UINT16(4, r);
  TEST_ASSERT_EQUAL_UINT(1, store.saved.size());
  TEST_ASSERT_EQUAL_UINT16(r, store.saved.back());
  TEST_ASSERT_LESS_OR_EQUAL_UINT(HISTORY_SIZE, store.saved.size());
}

void test_single_song_library_terminates() {
  MemStore store;
  store.data = {0};
  Picker p(store, scriptedRng);
  g_rng = {0};
  TEST_ASSERT_EQUAL_UINT16(0, p.pick(1));
  TEST_ASSERT_EQUAL_UINT(1, g_rng.size());  // no draw consumed
  TEST_ASSERT_EQUAL_INT(0, store.saveCalls);  // nothing recorded
}

void test_history_rolls_over_at_50() {
  MemStore store;
  Picker p(store, scriptedRng);
  for (uint32_t i = 0; i < HISTORY_SIZE + 1; i++) {
    g_rng = {i};
    p.pick(1000);
  }
  TEST_ASSERT_EQUAL_UINT(HISTORY_SIZE, store.saved.size());
  TEST_ASSERT_EQUAL_UINT16(1, store.saved.front());  // pick 0 dropped
  TEST_ASSERT_EQUAL_UINT16(HISTORY_SIZE, store.saved.back());
}

void test_count_zero_returns_zero_and_does_not_record() {
  MemStore store;
  Picker p(store, scriptedRng);
  g_rng = {5};
  TEST_ASSERT_EQUAL_UINT16(0, p.pick(0));
  TEST_ASSERT_EQUAL_INT(0, store.saveCalls);
  TEST_ASSERT_EQUAL_UINT(1, g_rng.size());  // rng not consumed
}

void test_stale_history_entry_is_ignored_but_kept() {
  MemStore store;
  store.data = {900};  // card shrank: 900 >= count
  Picker p(store, scriptedRng);
  g_rng = {2};
  TEST_ASSERT_EQUAL_UINT16(2, p.pick(10));
  TEST_ASSERT_EQUAL_UINT(2, store.saved.size());
  TEST_ASSERT_EQUAL_UINT16(900, store.saved[0]);
  TEST_ASSERT_EQUAL_UINT16(2, store.saved[1]);
}

void test_rerolls_are_bounded_then_accepts() {
  MemStore store;
  store.data = {3};
  Picker p(store, scriptedRng);
  for (int i = 0; i < 200; i++) g_rng.push_back(3);  // always the recent one
  TEST_ASSERT_EQUAL_UINT16(3, p.pick(10));
  TEST_ASSERT_EQUAL_UINT(200 - (1 + HISTORY_SIZE), g_rng.size());
}

void test_reload_full_ring_avoids_all_entries() {
  MemStore store;
  for (uint16_t i = 0; i < HISTORY_SIZE; i++) store.data.push_back(i);  // full ring 0..49
  Picker p(store, scriptedRng);
  TEST_ASSERT_EQUAL_UINT(HISTORY_SIZE, p.historySize());
  g_rng = {0, 1, 2, 49, 50};  // every reloaded entry is avoided, including the newest
  TEST_ASSERT_EQUAL_UINT16(50, p.pick(100));
  TEST_ASSERT_EQUAL_UINT(HISTORY_SIZE, store.saved.size());
  TEST_ASSERT_EQUAL_UINT16(1, store.saved.front());  // oldest entry 0 rolled out
  TEST_ASSERT_EQUAL_UINT16(50, store.saved.back());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_pick_is_in_range);
  RUN_TEST(test_pick_avoids_recent);
  RUN_TEST(test_pick_records_history);
  RUN_TEST(test_single_song_library_terminates);
  RUN_TEST(test_history_rolls_over_at_50);
  RUN_TEST(test_count_zero_returns_zero_and_does_not_record);
  RUN_TEST(test_stale_history_entry_is_ignored_but_kept);
  RUN_TEST(test_rerolls_are_bounded_then_accepts);
  RUN_TEST(test_reload_full_ring_avoids_all_entries);
  return UNITY_END();
}
