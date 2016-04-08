#include "base.h"
#include <cstdio>

bool execCmd(const std::string& cmd, std::string* out) {
  FILE *f = ::popen(cmd.data(), "r");
  if (f == NULL) {
    ELOG<< "popen error, cmd: " << cmd;
    return false;
  }

  char buf[8192] = { '\0' };
  while (out->size() < 1UL * 1024 * 1024) {
    auto readn = ::fread(buf, sizeof(char), sizeof(buf), f);
    if (readn == -1) {
      if (errno == EINTR) continue;
      ELOG<< "fread error, cmd: " << cmd;
      ::fclose(f);
      return false;
    }
    if (readn == 0) break;

    out->append(buf, readn);
  }

  ::fclose(f);
  return true;
}
