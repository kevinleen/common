#pragma once

#include "base/base.h"

namespace util {

enum key_type {
  user = 'a',
};

class KeyEncoder {
  public:
    explicit KeyEncoder(key_type type, const std::string& prefix = "")
        : _type(type) {
      _key.append(1, type);
      if (!prefix.empty()) {
        _key.append(prefix);
      }
    }
    ~KeyEncoder() = default;

    const std::string& asString() const {
      return _key;
    }

    void encodeUint32(uint32 i);
    void encodeUint64(uint64 i);
    void encodeBytes(const std::string& bytes);

  private:
    key_type _type;
    std::string _key;

    DISALLOW_COPY_AND_ASSIGN(KeyEncoder);
};

class KeyDecoder {
  public:
    KeyDecoder(key_type type, const std::string& key,
               const std::string& prefix = "");
    ~KeyDecoder() = default;

    uint32 decodeUint32();
    uint64 decodeUint64();

    const std::string decodeString();
    const std::string decodeBytes(uint32 len);

  private:
    key_type _type;
    const std::string& _key;

    uint32 _index;
    void checkLeft(uint32 len);
    void decode(char* buf, uint32 len);

    DISALLOW_COPY_AND_ASSIGN(KeyDecoder);
};

}
