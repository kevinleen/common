#pragma once

#include <city.h>

inline uint32 cityHash32(const char* data, uint32 len) {
  return CityHash32(data, len);
}

inline uint32 cityHash32(const std::string& data) {
  return cityHash32(data.data(), data.size());
}

inline uint64 cityHash64(const char* data, uint32 len) {
  return CityHash64(data, len);
}

inline uint64 cityHash64(const std::string& data) {
  return cityHash64(data.data(), data.size());
}
