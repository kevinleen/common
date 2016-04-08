#include "url_coder.h"

namespace {
class UrlCoder {
  public:
    UrlCoder();
    ~UrlCoder() = default;

    std::string decode(const std::string& src) const;
    std::string encode(const std::string& src) const;

    int hex2int(char c) const {
        if (c >= '0' && c <= '9') return c - '0';
        return ::tolower(c) - 'a' + 10;
    }

  private:
    util::ascii_table _uncoded;
    util::ascii_table _hex;

    DISALLOW_COPY_AND_ASSIGN(UrlCoder);
};

/*
 * reserved characters:  ! ( ) * # $ & ' + , / : ; = ? @ [ ]
 *
 * unreserved characters:  a...z  A...Z  0...9  - _ . ~
 *
 * hex: 0...9  a...f  A...F
 */
UrlCoder::UrlCoder()
    : _uncoded(std::vector<char> {
                   '-', '_', '.', '~', /* unreserved */
                   '!', '*', '\'', '(', ')', ';', ':', '@', '&',
                   '=', '+', '$', ',', '/', '?', '#', '[', ']'
               }) {
    for (int i = 'A'; i <= 'Z'; ++i) {
        _uncoded.set(i);
        _uncoded.set(i + 'a' - 'A');
    }

    for (int i = '0'; i <= '9'; ++i) {
        _uncoded.set(i);
        _hex.set(i);
    }

    for (int i = 'A'; i <= 'F'; ++i) {
        _hex.set(i);
        _hex.set(i + 'a' - 'A');
    }
}

std::string UrlCoder::decode(const std::string& src) const {
    std::string dst;
    dst.reserve(src.size());

    for (int i = 0; i < src.size(); ++i) {
        if (_uncoded.check(src[i])) {
            dst.push_back(src[i]);
            continue;
        } /* for uncoded character */

        // ' ' may be encoded as '+'
        if (src[i] == '+') {
            dst.push_back(' ');
            continue;
        }

        // for encoded character: %xx
        CHECK_EQ(src[i], '%');
        CHECK_LT(i + 2, src.size());
        CHECK(_hex.check(src[i + 1])) << static_cast<int>(src[i + 1]);
        CHECK(_hex.check(src[i + 2])) << static_cast<int>(src[i + 2]);

        int h4 = this->hex2int(src[i + 1]);
        int l4 = this->hex2int(src[i + 2]);
        char c = (h4 << 4) | l4;

        dst.push_back(c);
        i += 2;
    }

    return dst;
}

std::string UrlCoder::encode(const std::string& src) const {
    std::string dst;

    for (int i = 0; i < src.size(); ++i) {
        if (_uncoded.check(src[i])) {
            dst.push_back(src[i]);
            continue;
        }

        dst.push_back('%');
        dst.push_back("0123456789ABCDEF"[static_cast<uint8>(src[i]) >> 4]);
        dst.push_back("0123456789ABCDEF"[static_cast<uint8>(src[i]) & 0x0F]);
    }

    return dst;
}

UrlCoder kUrlCoder;
} // namespace

namespace util {
std::string decode_url(const std::string& src) {
    return kUrlCoder.decode(src);
}

std::string encode_url(const std::string& src) {
    return kUrlCoder.encode(src);
}
} // namespace util
