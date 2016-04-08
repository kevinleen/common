#pragma once

#include "data_types.h"
#include "stream_buf.h"

#include <string>
#include <vector>
#include <set>
#include <map>

namespace util {
/*
 * split string @s by character @c
 *
 *   split_string("x||x", '|');  ==>  { "x", "", "x" }
 */
std::vector<std::string> split_string(const std::string& s, char c);

/*
 * split_string("xxaxxbxxc", "xx");  ==>  { "a", "b", "c" }
 */
std::vector<std::string> split_string(const std::string& s,
                                      const std::string& separ);

/*
 * remove spaces at the beginning or end of string
 *
 *   trim_string("$$hello world$$", '$') ==> "hello world"
 */
std::string trim_string(const std::string& s, char c = ' ');

/*
 * for string s, replace all sub-strings @sub to string @to
 *
 *   replace_string("xxabcxx", "xx", "x");  "xxabcxx" ==> "xabcx"
 */
std::string replace_string(const std::string& s, const std::string& sub,
                           const std::string& to);

inline std::string replace_string(const std::string& s, const std::string& sub,
                                  const char to) {
    return util::replace_string(s, sub, std::string(1, to));
}

inline std::string replace_string(const std::string& s, const char sub,
                                  const std::string& to) {
    return util::replace_string(s, std::string(1, sub), to);
}

inline std::string replace_string(const std::string& s, const char sub,
                                  const char to) {
    return util::replace_string(s, std::string(1, sub), std::string(1, to));
}

/*
 * map to string: |k1:v1|k2:v2|k3:v3|
 *                |v1|v2|v3|            ignore_key = true, c = '|'
 */
template <typename K, typename V>
std::string to_string(const std::map<K, V>& v, bool ignore_key = false,
                      char c = '|') {
    if (v.empty()) return std::string();

    StreamBuf sb;
    sb << c;

    for (auto it = v.begin(); it != v.end(); ++it) {
        if (ignore_key) {
            sb << it->second << c;
        } else {
            sb << it->first << ':' << it->second << c;
        }
    }

    return sb.to_string();
}

/*
 * vector to string
 *
 *   vec = { "uuu", "vvv" };   to_string(vec, '|') ==> "|uuu|vvv|"
 *
 *   vec = { "", "xx", "" };   to_string(vec, '|') ==> "||xx||"
 *
 *   vec<int> = { 3, 2, 1 };   to_string(vec, '|') ==> "|3|2|1|"
 */
template <typename T>
std::string to_string(const std::vector<T>& v, char c = '|') {
    if (v.empty()) return std::string();

    StreamBuf sb;
    sb << c;

    for (uint32 i = 0; i < v.size(); ++i) {
        sb << v[i] << c;
    }

    return sb.to_string();
}

/*
 * set to string
 *
 *   set<int> = { 3, 2, 1 };   to_string(set, '|') ==> "|1|2|3|"
 */
template <typename T>
std::string to_string(const std::set<T>& v, char c = '|') {
    if (v.empty()) return std::string();

    StreamBuf sb;
    sb << c;

    for (auto it = v.begin(); it != v.end(); ++it) {
        sb << *it << c;
    }

    return sb.to_string();
}

/*
 * basic data types to string
 *
 *   to_string(3.141);  ==> "3.141"
 *
 *   to_string(12345);  ==> "12345"
 */
template <typename T>
inline std::string to_string(T t) {
    StreamBuf sb(32);
    sb << t;
    return sb.to_string();
}

/*
 * wchar to string, @wstr must be null-terminated
 */
std::string to_string(const wchar_t* wstr);

std::string to_string(const std::wstring& wstr);

/*
 * string to basic data types.
 *
 *   CHECK failed on any error
 */
int32 to_int32(const std::string& v);
int64 to_int64(const std::string& v);

uint32 to_uint32(const std::string& v);
uint64 to_uint64(const std::string& v);

bool to_bool(const std::string& v);

double to_double(const std::string& v);

/*
 * convert string to basic data types
 *
 *   return false on any error, and set error info to @err
 */
bool to_int32(const std::string& v, int32* r, std::string& err);
bool to_int64(const std::string& v, int64* r, std::string& err);

bool to_uint32(const std::string& v, uint32* r, std::string& err);
bool to_uint64(const std::string& v, uint64* r, std::string& err);

bool to_bool(const std::string& v, bool* r, std::string& err);

bool to_double(const std::string& v, double* r, std::string& err);
} // namespace ut
