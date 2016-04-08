#include "log_writer.h"

namespace util {

bool LogWriter::append(uint32 type, const char* data, uint32 len) {
  uint32 space_len = BLOCK_SIZE - _block_offset;
  if (space_len < LOG_HEADER_SIZE + len) {
    return false;
  }

  // length + type + crc32
  char* buf = _block + _block_offset;
  save32(buf, len);  // length
  buf += 4;
  save32(buf, type);  // type
  buf += 4;
  uint32 crc = 0;
  if (_crc_check) crc = crc32Value(data, len);
  save32(buf, crc);  // crc32
  buf += 4;

  ::memcpy(buf, data, len);
  _block_offset += LOG_HEADER_SIZE + len;
  return true;
}

void LogWriter::flush() {
  if (_block_offset != 0) {
    int32 writen = _log_file->write(_block, _block_offset);
    CHECK_EQ(writen, _block_offset);
    _log_file->flush();
    _block_offset = 0;
  }
}

bool LogWriter::append(const char* data, uint32 len) {
  for (uint32 pos = 0; pos != len; /* empty */) {
    uint32 space_size = BLOCK_SIZE - _block_offset;
    if (space_size <= LOG_HEADER_SIZE) {
      ::memset(_block + _block_offset, '\0', space_size);
      flush();
      continue;
    }

    CHECK_GT(space_size, LOG_HEADER_SIZE);
    uint32 avail_len = std::min(space_size, len - pos);
    uint32 type = 0;
    if (pos == 0) type |= START_RECORD;
    if (pos + avail_len == len) type |= LAST_RECORD;

    if (!append(type, data + pos, avail_len)) {
      return false;
    }

    pos += avail_len;
  }

  return true;
}

}

