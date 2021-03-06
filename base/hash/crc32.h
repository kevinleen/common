#pragma once

#include <stddef.h>
#include <stdint.h>
#include <string>

// this file is copyed from leveldb.

// Return the crc32c of concat(A, data[0,n-1]) where init_crc is the
// crc32c of some string A.  Extend() is often used to maintain the
// crc32c of a stream of data.
uint32_t crc32Extend(uint32_t init_crc, const char* data, size_t n);
inline uint32_t crc32Extend(uint32_t init_crc, std::string& data) {
  return crc32Extend(init_crc, data.c_str(), data.size());
}

// Return the crc32c of data[0,n-1]
inline uint32_t crc32Value(const char* data, size_t n) {
  return crc32Extend(0, data, n);
}
inline uint32_t crc32Value(const std::string& data) {
  return crc32Value(data.c_str(), data.size());
}

static const uint32_t kMaskDelta = 0xa282ead8ul;

// Return a masked representation of crc.
//
// Motivation: it is problematic to compute the CRC of a string that
// contains embedded CRCs.  Therefore we recommend that CRCs stored
// somewhere (e.g., in files) should be masked before being stored.
inline uint32_t crc32Mask(uint32_t crc) {
  // Rotate right by 15 bits and add a constant.
  return ((crc >> 15) | (crc << 17)) + kMaskDelta;
}

// Return the crc whose masked representation is masked_crc.
inline uint32_t crc32Unmask(uint32_t masked_crc) {
  uint32_t rot = masked_crc - kMaskDelta;
  return ((rot >> 17) | (rot << 15));
}

