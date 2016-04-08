#pragma once

#include "base/base.h"
#include "logger_def.h"

namespace util {

// not threadsafe.
class LogWriter {
  public:
    LogWriter(AppendonlyMmapedFile* log_file, bool enable_crc_check = true)
        : _log_file(log_file), _crc_check(enable_crc_check), _block_offset(0) {
    }
    ~LogWriter() {
    }

    void flush();
    bool append(const char* data, uint32 len);
    bool append(const std::string& log) {
      return append(log.data(), log.size());
    }

  private:
    std::unique_ptr<AppendonlyMmapedFile> _log_file;

    const bool _crc_check;

    uint32 _block_offset;
    char _block[BLOCK_SIZE];

    bool append(uint32 type, const char* data, uint32 len);

    DISALLOW_COPY_AND_ASSIGN(LogWriter);
};
}
