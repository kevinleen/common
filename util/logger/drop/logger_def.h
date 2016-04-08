#pragma once

namespace util {

enum RECORD_TYPE {
  START_RECORD = (1 << 0), LAST_RECORD = (1 << 1),
};

enum Status {
  OK = 0, FILE_,
};

#define BLOCK_SIZE (32U * 1024)
#define LOG_HEADER_SIZE 12

}

