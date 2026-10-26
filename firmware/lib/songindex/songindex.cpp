#include "songindex.h"

#include <cstring>

bool SongIndex::load(IndexReader& r) {
  count_ = 0;
  uint8_t buf[INDEX_HEADER_LEN];
  if (!r.readAt(0, buf, sizeof(buf))) return false;

  static const char kMagic[] = "SONGBUTTON 1 ";
  const size_t magicLen = sizeof(kMagic) - 1;
  if (std::memcmp(buf, kMagic, magicLen) != 0) return false;

  // Digits run until the first space (padding) or newline.
  uint64_t value = 0;
  size_t digits = 0;
  size_t i = magicLen;
  for (; i < sizeof(buf) && buf[i] >= '0' && buf[i] <= '9'; ++i, ++digits) {
    value = value * 10 + (buf[i] - '0');
    if (value > 65535) return false;  // the picker is 16-bit
  }
  if (digits == 0) return false;
  // Remainder must be only padding spaces ending in newline.
  for (; i + 1 < sizeof(buf); ++i) {
    if (buf[i] != ' ') return false;
  }
  if (buf[sizeof(buf) - 1] != '\n') return false;
  if (value == 0) return false;

  count_ = static_cast<uint32_t>(value);
  return true;
}

bool SongIndex::read(IndexReader& r, uint32_t n, SongRecord& out) const {
  if (n >= count_) return false;
  uint8_t buf[INDEX_RECORD_LEN];
  uint32_t offset = INDEX_HEADER_LEN + static_cast<uint32_t>(INDEX_RECORD_LEN) * n;
  if (!r.readAt(offset, buf, sizeof(buf))) return false;

  // Strip the newline and trailing spaces.
  size_t len = sizeof(buf);
  if (buf[len - 1] == '\n') --len;
  while (len > 0 && buf[len - 1] == ' ') --len;
  if (len == 0) return false;

  size_t tab = 0;
  while (tab < len && buf[tab] != '\t') ++tab;
  if (tab == len) return false;  // no tab
  if (tab == 0) return false;    // empty filename
  if (tab > sizeof(out.filename) - 1) return false;
  for (size_t i = 0; i < tab; ++i) {
    // ASCII only, no control bytes or DEL, no '/' (filename is appended to SONG_DIR).
    if (buf[i] < 0x20 || buf[i] >= 0x7F || buf[i] == '/') return false;
  }

  std::memcpy(out.filename, buf, tab);
  out.filename[tab] = '\0';

  size_t titleLen = len - tab - 1;
  if (titleLen > sizeof(out.title) - 1) titleLen = sizeof(out.title) - 1;
  std::memcpy(out.title, buf + tab + 1, titleLen);
  out.title[titleLen] = '\0';
  return true;
}
