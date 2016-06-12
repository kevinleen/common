#include "db_key.h"

namespace util {

void KeyEncoder::encodeUint32(uint32 i) {
  _key.append((char*) &i, sizeof(i));
}

void KeyEncoder::encodeUint64(uint64 i) {
  _key.append((char*) &i, sizeof(i));
}

void KeyEncoder::encodeBytes(const std::string& bytes) {
  _key.append(bytes);
}

KeyDecoder::KeyDecoder(key_type type, const std::string& key,
                       const std::string& prefix)
    : _type(type), _key(key) {
  CHECK(!key.empty());
  CHECK_EQ(type, key[0]);
  _index = 1;
  if (!prefix.empty()) {
    CHECK_EQ(key.substr(_index, prefix.size()), prefix);
    _index += prefix.size();
  }
}

void KeyDecoder::checkLeft(uint32 len) {
  CHECK_GE(_key.size(), _index + len);
}

void KeyDecoder::decode(char* buf, uint32 len) {
  checkLeft(len);
  ::memcpy(buf, _key.data() + _index, len);
  _index += len;
}

uint32 KeyDecoder::decodeUint32() {
  uint32 i;
  decode((char*) &i, sizeof(i));
  return i;
}

uint64 KeyDecoder::decodeUint64() {
  uint64 i;
  decode((char*) &i, sizeof(i));
  return i;
}

const std::string KeyDecoder::decodeString() {
  auto ret = _key.substr(_index);
  _index = _key.size();
  return ret;
}

const std::string KeyDecoder::decodeBytes(uint32 len) {
  std::vector<char> buf(len);
  decode(&buf[0], len);
  return std::string(buf.begin(), buf.end());
}

}
