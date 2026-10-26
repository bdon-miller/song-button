#pragma once
#include <FS.h>

#include "picker.h"
#include "songindex.h"

bool storageBegin();  // SPI + SD.begin; call once per boot

class SdIndexReader : public IndexReader {
 public:
  bool open();  // false if INDEX_PATH is missing
  bool readAt(uint32_t offset, uint8_t* buf, size_t len) override;

 private:
  File file_;
};

// Scans SONG_DIR and writes INDEX_PATH. False if no MP3s or on any write error.
bool buildFallbackIndex();

class NvsHistoryStore : public HistoryStore {
 public:
  size_t load(uint16_t* buf, size_t max) override;
  void save(const uint16_t* buf, size_t n) override;
};
