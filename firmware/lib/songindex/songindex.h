#pragma once
// Plain C++ only: no Arduino includes, so host (native) tests can use it.
#include <cstddef>
#include <cstdint>

#include "config.h"  // INDEX_HEADER_LEN, INDEX_RECORD_LEN, INDEX_PATH, SONG_DIR

// Implemented by hal_storage on device, by a memory fake in tests.
struct IndexReader {
  virtual bool readAt(uint32_t offset, uint8_t* buf, size_t len) = 0;  // false on short read
  virtual ~IndexReader() = default;
};

struct SongRecord {
  char filename[48];
  char title[80];
};

class SongIndex {
 public:
  // Parses the header. False if magic/version is wrong or count is not a decimal in 1..65535.
  bool load(IndexReader& r);
  uint32_t count() const { return count_; }
  // False past count, on short read, or on a malformed record. `out` is untouched on failure.
  bool read(IndexReader& r, uint32_t n, SongRecord& out) const;

 private:
  uint32_t count_ = 0;
};
