#pragma once

#include "base/base.h"

namespace util {

class Replacer {
  public:
    Replacer(std::string* str)
        : _str(str) {
      CHECK_NOTNULL(str);
    }
    ~Replacer() {
    }

    void replace() {
      for (auto it = _map.begin(); it != _map.end(); ++it) {
        const auto& src = it->first;
        const auto& dst = it->second;
        while (true) {
          auto pos = _str->find(src);
          if (pos == std::string::npos) {
            break;
          }

          _str->replace(pos, src.size(), dst);
        }
      }
    }

    void push(const std::string& src, const std::string& dst) {
      _map[src] = dst;
    }

  private:
    std::string* _str;

    std::map<std::string, std::string> _map;

    DISALLOW_COPY_AND_ASSIGN(Replacer);
};

}
