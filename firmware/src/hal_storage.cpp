#include "hal_storage.h"

#include <Arduino.h>
#include <Preferences.h>
#include <SD.h>
#include <SPI.h>

#include "config.h"

bool storageBegin() {
  SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  return SD.begin(PIN_SD_CS, SPI, 25000000);
}

bool SdIndexReader::open() {
  file_ = SD.open(INDEX_PATH, FILE_READ);
  return file_ && !file_.isDirectory();
}

bool SdIndexReader::readAt(uint32_t offset, uint8_t* buf, size_t len) {
  if (!file_ || !file_.seek(offset)) return false;
  return file_.read(buf, len) == len;
}

// Returns true and fills `stem` (no ".mp3") if `name` is an acceptable MP3 filename.
static bool mp3Stem(const char* name, char* stem) {
  size_t n = strlen(name);
  if (n <= 4 || n > 47 || name[0] == '.' || strcasecmp(name + n - 4, ".mp3") != 0) return false;
  for (size_t i = 0; i < n; i++)
    if ((uint8_t)name[i] < 0x20 || (uint8_t)name[i] > 0x7e || name[i] == '\t') return false;
  memcpy(stem, name, n - 4);
  stem[n - 4] = 0;
  return true;
}

// Writes the 32-byte header "SONGBUTTON 1 <count>", space-padded, newline-terminated.
static bool writeHeader(File& out, int count) {
  char hdr[INDEX_HEADER_LEN];
  memset(hdr, ' ', sizeof hdr);
  int n = snprintf(hdr, sizeof hdr, "SONGBUTTON 1 %d", count);
  hdr[n] = ' ';  // snprintf's terminator
  hdr[INDEX_HEADER_LEN - 1] = '\n';
  return out.write((const uint8_t*)hdr, sizeof hdr) == sizeof hdr;
}

// Writes one record per qualifying MP3 in SONG_DIR. Returns count, or -1 on error.
// getNextFileName() is readdir only: no per-entry open, so the walk stays linear.
static int writeRecords(File& out) {
  File dir = SD.open(SONG_DIR);
  if (!dir || !dir.isDirectory()) return -1;
  int count = 0;
  char rec[INDEX_RECORD_LEN], stem[48];
  for (;;) {
    bool isDir = false;
    String path = dir.getNextFileName(&isDir);
    if (path.length() == 0) break;
    int slash = path.lastIndexOf('/');
    const char* name = path.c_str() + slash + 1;  // basename; slash == -1 keeps the whole path
    if (isDir || !mp3Stem(name, stem) || count >= 65535) continue;
    memset(rec, ' ', sizeof rec);
    int n = snprintf(rec, sizeof rec, "%s\t%s", name, stem);
    rec[n] = ' ';  // snprintf's terminator
    rec[INDEX_RECORD_LEN - 1] = '\n';
    if (out.write((const uint8_t*)rec, sizeof rec) != sizeof rec) return -1;
    count++;
  }
  return count;
}

bool buildFallbackIndex() {
  File out = SD.open("/index.tmp", FILE_WRITE);
  if (!out) return false;
  // Placeholder header first, records as found, then rewrite the header with the real count.
  int count = writeHeader(out, 0) ? writeRecords(out) : -1;
  bool ok = count > 0 && out.seek(0) && writeHeader(out, count);
  out.close();
  if (!ok) {
    SD.remove("/index.tmp");
    return false;
  }
  SD.remove(INDEX_PATH);
  return SD.rename("/index.tmp", INDEX_PATH);
}

size_t NvsHistoryStore::load(uint16_t* buf, size_t max) {
  Preferences p;
  if (!p.begin(NVS_NAMESPACE, true)) return 0;
  size_t bytes = p.isKey(NVS_HISTORY_KEY) ? p.getBytesLength(NVS_HISTORY_KEY) : 0;
  size_t n = 0;
  if (bytes > 0 && bytes % 2 == 0) {
    n = bytes / 2 < max ? bytes / 2 : max;
    n = p.getBytes(NVS_HISTORY_KEY, buf, n * 2) / 2;
  }
  p.end();
  return n;
}

void NvsHistoryStore::save(const uint16_t* buf, size_t n) {
  Preferences p;
  if (!p.begin(NVS_NAMESPACE, false)) return;
  p.putBytes(NVS_HISTORY_KEY, buf, n * 2);
  p.end();
}
