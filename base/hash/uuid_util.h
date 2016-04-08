#pragma once

#include <cassert>
#include <uuid/uuid.h>
#include <string>

inline void generateUuid(std::string* uuid) {
  uuid_t out;
  uuid_generate_random(out);
  char buf[36] = { '\0' };
  uuid_unparse(out, buf);
  uuid->assign(buf, 36);
}

inline const std::string generateUuid() {
  std::string uuid;
  generateUuid(&uuid);
  return uuid;
}
