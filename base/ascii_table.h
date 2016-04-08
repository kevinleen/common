#pragma once

#include <map>
#include <vector>

namespace util {
/*
 * usage:
 *   ascii_table tab {
 *       std::map { {'x', 1}, {'y', 2}, {'z', 3} }
 *   };
 *
 *   tab.check('x');  // return true;
 *   tab.check('o');  // return false;
 *
 *   tab.get('z');    // return 3
 *
 *       ascii_table tab {
 *           std::vector { 'x', 'y', 'z' }
 *       };
 *   <==>
 *       ascii_table tab {
 *           std::map { {'x', 1}, {'y', 1}, {'z', 1} }
 *       };
 */
class ascii_table {
  public:
    ascii_table() {
        _table.resize(128);
    }

    ascii_table(const std::map<char, int>& x) {
        _table.resize(128);

        for (auto it = x.begin(); it != x.end(); ++it) {
            if (it->first >= 0) _table[it->first] = it->second;
        }
    }

    ascii_table(const std::vector<char>& x) {
        _table.resize(128);

        for (auto it = x.begin(); it != x.end(); ++it) {
            if (*it >= 0) _table[*it] = 1;
        }
    }

    ~ascii_table() = default;

    ascii_table(const ascii_table&) = delete;
    void operator=(const ascii_table&) = delete;

    void set(char c, int value = 1) {
        if (c >= 0) _table[c] = value;
    }

    int get(char c) const {
        return c >= 0 ? _table[c] : 0;
    }

    bool check(char c) const {
        return c >= 0 ? _table[c] != 0 : false;
    }

  private:
    std::vector<int> _table;
};
} // namespace ut
