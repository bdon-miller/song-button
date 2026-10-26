#include <unity.h>

#include <cstring>
#include <string>
#include <vector>

#include "songindex.h"

struct MemReader : IndexReader {
  std::vector<uint8_t> data;
  bool readAt(uint32_t offset, uint8_t* buf, size_t len) override {
    if (static_cast<size_t>(offset) + len > data.size()) return false;
    std::memcpy(buf, data.data() + offset, len);
    return true;
  }
};

// Pads `text` with spaces to width-1 bytes and appends '\n'.
static std::string padLine(const std::string& text, size_t width) {
  std::string s = text;
  s.resize(width - 1, ' ');
  s.push_back('\n');
  return s;
}

static void append(MemReader& r, const std::string& s) {
  r.data.insert(r.data.end(), s.begin(), s.end());
}

static void appendHeader(MemReader& r, const std::string& text) {
  append(r, padLine(text, INDEX_HEADER_LEN));
}

static void appendRecord(MemReader& r, const std::string& text) {
  append(r, padLine(text, INDEX_RECORD_LEN));
}

void setUp() {}
void tearDown() {}

void test_load_parses_count() {
  MemReader r;
  appendHeader(r, "SONGBUTTON 1 1000");
  SongIndex idx;
  TEST_ASSERT_TRUE(idx.load(r));
  TEST_ASSERT_EQUAL_UINT32(1000, idx.count());
}

void test_load_rejects_bad_magic() {
  MemReader r;
  appendHeader(r, "BADMAGIC 1 5");
  SongIndex idx;
  TEST_ASSERT_FALSE(idx.load(r));
}

void test_load_rejects_zero_count() {
  MemReader r;
  appendHeader(r, "SONGBUTTON 1 0");
  SongIndex idx;
  TEST_ASSERT_FALSE(idx.load(r));
}

void test_read_record_n() {
  MemReader r;
  appendHeader(r, "SONGBUTTON 1 2");
  appendRecord(r, "0001-a.mp3\tA");
  appendRecord(r, "0002-b.mp3\tB");
  SongIndex idx;
  TEST_ASSERT_TRUE(idx.load(r));
  SongRecord rec;
  TEST_ASSERT_TRUE(idx.read(r, 1, rec));
  TEST_ASSERT_EQUAL_STRING("0002-b.mp3", rec.filename);
  TEST_ASSERT_EQUAL_STRING("B", rec.title);
}

void test_read_past_count_false() {
  MemReader r;
  appendHeader(r, "SONGBUTTON 1 1");
  appendRecord(r, "0001-a.mp3\tA");
  appendRecord(r, "0002-b.mp3\tB");
  SongIndex idx;
  TEST_ASSERT_TRUE(idx.load(r));
  SongRecord rec;
  TEST_ASSERT_FALSE(idx.read(r, 1, rec));
}

void test_read_short_file_false() {
  MemReader r;
  appendHeader(r, "SONGBUTTON 1 3");
  appendRecord(r, "0001-a.mp3\tA");
  SongIndex idx;
  TEST_ASSERT_TRUE(idx.load(r));
  SongRecord rec;
  TEST_ASSERT_FALSE(idx.read(r, 2, rec));
}

void test_read_malformed_record_false() {
  MemReader r;
  appendHeader(r, "SONGBUTTON 1 2");
  appendRecord(r, "");                  // all spaces
  appendRecord(r, "0002-b.mp3 no tab"); // no tab
  SongIndex idx;
  TEST_ASSERT_TRUE(idx.load(r));
  SongRecord rec;
  TEST_ASSERT_FALSE(idx.read(r, 0, rec));
  TEST_ASSERT_FALSE(idx.read(r, 1, rec));
}

static bool readOne(const std::string& rec, SongRecord& out) {
  MemReader r;
  appendHeader(r, "SONGBUTTON 1 1");
  appendRecord(r, rec);
  SongIndex idx;
  if (!idx.load(r)) return false;
  return idx.read(r, 0, out);
}

static bool loadHeader(const std::string& h) {
  MemReader r;
  appendHeader(r, h);
  SongIndex idx;
  return idx.load(r);
}

void test_filename_47_accepted_48_rejected() {
  SongRecord rec;
  std::string f47(47, 'a'), f48(48, 'a');
  TEST_ASSERT_TRUE(readOne(f47 + "\tT", rec));
  TEST_ASSERT_EQUAL_STRING(f47.c_str(), rec.filename);
  SongRecord untouched;
  std::memset(&untouched, 'x', sizeof(untouched));
  rec = untouched;
  TEST_ASSERT_FALSE(readOne(f48 + "\tT", rec));
  TEST_ASSERT_EQUAL_MEMORY(&untouched, &rec, sizeof(rec));
}

void test_title_truncated_to_79() {
  SongRecord rec;
  std::string t79(79, 'b');
  TEST_ASSERT_TRUE(readOne("a.mp3\t" + t79, rec));
  TEST_ASSERT_EQUAL_STRING(t79.c_str(), rec.title);
  TEST_ASSERT_TRUE(readOne("a.mp3\t" + std::string(80, 'c'), rec));
  TEST_ASSERT_EQUAL_UINT(79, strlen(rec.title));
  TEST_ASSERT_EQUAL_CHAR('\0', rec.title[79]);
  TEST_ASSERT_TRUE(readOne("a.mp3\t" + std::string(100, 'd'), rec));
  TEST_ASSERT_EQUAL_UINT(79, strlen(rec.title));
}

void test_non_ascii_filename_rejected() {
  SongRecord rec;
  TEST_ASSERT_FALSE(readOne("caf\xC3\xA9.mp3\tT", rec));
}

void test_filename_slash_or_control_rejected() {
  SongRecord rec;
  TEST_ASSERT_FALSE(readOne("dir/a.mp3\tT", rec));
  TEST_ASSERT_FALSE(readOne(std::string("a\x01.mp3\tT"), rec));
  TEST_ASSERT_FALSE(readOne(std::string("a\x7F.mp3\tT"), rec));
}

void test_title_keeps_tab_and_internal_spaces() {
  SongRecord rec;
  TEST_ASSERT_TRUE(readOne("a.mp3\tOne  Two\tThree", rec));
  TEST_ASSERT_EQUAL_STRING("One  Two\tThree", rec.title);
  TEST_ASSERT_EQUAL_STRING("a.mp3", rec.filename);
}

void test_load_count_limits() {
  TEST_ASSERT_TRUE(loadHeader("SONGBUTTON 1 65535"));
  TEST_ASSERT_FALSE(loadHeader("SONGBUTTON 1 65536"));
}

void test_load_rejects_bad_headers() {
  TEST_ASSERT_FALSE(loadHeader("SONGBUTTON 1 99999999999"));
  TEST_ASSERT_FALSE(loadHeader("SONGBUTTON 1 "));
  TEST_ASSERT_FALSE(loadHeader("SONGBUTTON 1 5 junk"));
  TEST_ASSERT_FALSE(loadHeader("SONGBUTTON 1 5x"));
  TEST_ASSERT_FALSE(loadHeader("SONGBUTTON 2 5"));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_load_parses_count);
  RUN_TEST(test_load_rejects_bad_magic);
  RUN_TEST(test_load_rejects_zero_count);
  RUN_TEST(test_read_record_n);
  RUN_TEST(test_read_past_count_false);
  RUN_TEST(test_read_short_file_false);
  RUN_TEST(test_read_malformed_record_false);
  RUN_TEST(test_filename_47_accepted_48_rejected);
  RUN_TEST(test_title_truncated_to_79);
  RUN_TEST(test_non_ascii_filename_rejected);
  RUN_TEST(test_filename_slash_or_control_rejected);
  RUN_TEST(test_title_keeps_tab_and_internal_spaces);
  RUN_TEST(test_load_count_limits);
  RUN_TEST(test_load_rejects_bad_headers);
  return UNITY_END();
}
