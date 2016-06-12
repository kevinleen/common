#include "string_util.h"
#include "ascii_table.h"
#include "cclog/cclog.h"

#include <wchar.h>
#include <iconv.h>
#include <errno.h>
#include <math.h>

namespace {
util::ascii_table kIntUnit {
    std::map<char, int> {
        {'k', 10}, {'K', 10},
        {'m', 20}, {'M', 20},
        {'g', 30}, {'G', 30},
        {'t', 40}, {'T', 40},
        {'p', 50}, {'P', 50}
    }
};

std::string wchar2str(const wchar_t* wstr, ::size_t wlen) {
    ::iconv_t cd = ::iconv_open("UTF-8", "WCHAR_T");
    if (cd == (::iconv_t) -1) return std::string();

    char* in = (char*) wstr;
    ::size_t in_size = wlen * sizeof(wchar_t);
    ::size_t out_size = wlen * 6;

    std::string s;
    s.resize(out_size);
    char* out = &s[0];

    ::size_t ret = ::iconv(cd, &in, &in_size, &out, &out_size);
    (ret != (::size_t) -1) ? s.resize(s.size() - out_size) : s.clear();

    ::iconv_close(cd);
    return s;
}
} // namespace

namespace util {

namespace xx {
bool pattern_match(const char* p, const char* e) {
    if (*p == '*' && *(p + 1) == '\0') return true;

    for (; *p != '\0' && *e != '\0';) {
        char c = *p;

        if (c == '*') {
            return pattern_match(p + 1, e) || pattern_match(p, e + 1);
        }

        if (c != '?' && c != *e) return false;
        ++p;
        ++e;
    }

    return (*p == '*' && *(p + 1) == '\0') || (*p == '\0' && *e == '\0');
}
}

bool pattern_match(const std::string& pattern, const std::string& expression) {
    return xx::pattern_match(pattern.c_str(), expression.c_str());
}

/*
 * As C++ 11 support move semantics, it's ok to return vector.
 */
std::vector<std::string> split_string(const std::string& s, char c) {
    std::vector<std::string> v;

    ::size_t pos = 0, from = 0;
    if (!s.empty() && s[0] == c) from = 1;

    while ((pos = s.find(c, from)) != std::string::npos) {
        v.push_back(s.substr(from, pos - from));
        from = pos + 1;
    }

    if (from < s.size()) v.push_back(s.substr(from));

    return v;
}

std::vector<std::string> split_string(const std::string& s,
                                      const std::string& separ) {
    std::vector<std::string> v;

    ::size_t pos = 0, from = 0;

    while ((pos = s.find(separ, from)) != std::string::npos) {
        if (pos != 0) v.push_back(s.substr(from, pos - from));
        from = pos + separ.size();
    }

    if (from < s.size()) v.push_back(s.substr(from));

    return v;
}

std::string trim_string(const std::string& s, char c/* = ' ' */) {
    if (s.empty() || (s[0] != c && *s.rbegin() != c)) return s;

    ::size_t bp = s.find_first_not_of(c);
    if (bp == s.npos) return std::string();

    ::size_t ep = s.find_last_not_of(c);
    return s.substr(bp, ep - bp + 1);
}

std::string replace_string(const std::string& s, const std::string& sub,
                           const std::string& to) {
    std::string x;
    ::size_t pos = 0, from = 0;

    while ((pos = s.find(sub, from)) != s.npos) {
        x += s.substr(from, pos - from) + to;
        from = pos + sub.size();
    }

    if (from == 0) return s;

    if (from < s.size()) x += s.substr(from);
    return x;
}

std::string to_string(const wchar_t* wstr) {
    return wchar2str(wstr, ::wcslen(wstr));
}

std::string to_string(const std::wstring& wstr) {
    return wchar2str(wstr.c_str(), wstr.size());
}

bool to_bool(const std::string& v, bool* r, std::string& err) {
    if (v == "true" || v == "1") {
        *r = true;
        return true;
    }

    if (v == "false" || v == "0") {
        *r = false;
        return true;
    }

    err = std::string("invalid value for bool") + ": " + v;
    return false;
}

bool to_double(const std::string& v, double* r, std::string& err) {
    char* end = NULL;
    double x = ::strtod(v.c_str(), &end);

    if (errno == ERANGE && (x == HUGE_VAL || x == -HUGE_VAL)) {
        errno = 0;
        err = std::string("out of range for double") + ": " + v;
        return false;
    }

    if (end != v.c_str() + v.size()) {
        err = std::string("invalid value for double") + ": " + v;
        return false;
    }

    *r = x;
    return true;
}

int64 to_int64(const char* v, uint32 len, std::string& err) {
    if (len == 0) return 0;

    if (!kIntUnit.check(v[len - 1])) {
        char* end = NULL;
        int64 x = ::strtoll(v, &end, 0);

        if (errno == ERANGE && (x == MIN_INT64 || x == MAX_INT64)) {
            errno = 0;
            goto range_err;
        }

        if (end == v + len) return x;

        err = std::string("invalid value for integer") + ": " + v;
        return 0;

    } else {
        int64 x = to_int64(v, len - 1, err);
        if (x == 0) return 0;

        int off = kIntUnit.get(v[len - 1]);
        if (x < (MIN_INT64 >> off) || x > (MAX_INT64 >> off)) goto range_err;
        return x << off;
    }

    range_err:
    err = std::string("out of range for int64") + ": " + v;
    return 0;
}

uint64 to_uint64(const char* v, uint32 len, std::string& err) {
    if (len == 0) return 0;

    if (!kIntUnit.check(v[len - 1])) {
        char* end = NULL;
        int64 x = ::strtoull(v, &end, 0);

        if (errno == ERANGE && (uint64) x == MAX_UINT64) {
            errno = 0;
            goto range_err;
        }

        if (end == v + len) return x;

        err = std::string("invalid value for integer") + ": " + v;
        return 0;

    } else {
        int64 x = to_uint64(v, len - 1, err);
        if (x == 0) return 0;

        int off = kIntUnit.get(v[len - 1]);
        if (abs(x) > (MAX_UINT64 >> off)) goto range_err;
        return x << off;
    }

    range_err:
    err = std::string("out of range for uint64") + ": " + v;
    return 0;
}

bool to_int64(const std::string& v, int64* r, std::string& err) {
    int64 x = to_int64(v.data(), v.size(), err);
    if (!err.empty()) return false;

    *r = x;
    return true;
}

bool to_uint64(const std::string& v, uint64* r, std::string& err) {
    uint64 x = to_uint64(v.data(), v.size(), err);
    if (!err.empty()) return false;

    *r = x;
    return true;
}

bool to_int32(const std::string& v, int32* r, std::string& err) {
    int64 x = to_int64(v.data(), v.size(), err);
    if (!err.empty()) {
        if (err[0] == 'o') goto err; // out of range
        return false;
    }

    if (x > MAX_INT32 || x < MIN_INT32) goto err;

    *r = static_cast<int32>(x);
    return true;

    err:
    err = std::string("out of range for int32") + ": " + v;
    return false;
}

bool to_uint32(const std::string& v, uint32* r, std::string& err) {
    int64 x = to_uint64(v.data(), v.size(), err);
    if (!err.empty()) {
        if (err[0] == 'o') goto err; // out of range
        return false;
    }

    if (abs(x) > MAX_UINT32) goto err;

    *r = static_cast<uint32>(x);
    return true;

    err:
    err = std::string("out of range for int32") + ": " + v;
    return false;
}

#define DEF_fun(type) \
    type to_##type(const std::string& v) { \
        type x; \
        std::string err; \
        bool r = to_##type(v, &x, err); \
        CHECK(r) << err; \
        return x; \
    }

DEF_fun(bool)
DEF_fun(double)
DEF_fun(int32)
DEF_fun(int64)
DEF_fun(uint32)
DEF_fun(uint64)

#undef DEF_fun
} // namespace util
