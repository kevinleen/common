#pragma once

#include <string>
#include "base/data_types.h"

uint16 crc16(const char *data, int len);

inline uint16 crc16(const std::string& data) {
  return crc16(data.data(), data.size());
}

