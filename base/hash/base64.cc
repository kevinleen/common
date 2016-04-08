#include "base64.h"

static const char EnBase64Tab[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static const char DeBase64Tab[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0,
    62,        // '+'
    0, 0, 0,
    63,        // '/'
    52, 53, 54, 55, 56, 57, 58, 59, 60,
    61,        // '0'-'9'
    0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23, 24,
    25,        // 'A'-'Z'
    0, 0, 0, 0, 0, 0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50,
    51,        // 'a'-'z'
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};

namespace xx {
static int base64_encode(const unsigned char* pSrc, int nSrcLen, char* pDst,
                         bool change_line/* = false*/) {
    unsigned char c1, c2, c3;
    int nDstLen = 0;
    int nLineLen = 0;
    int nDiv = nSrcLen / 3;
    int nMod = nSrcLen % 3;

    for (int i = 0; i < nDiv; i++) {
        c1 = *pSrc++;
        c2 = *pSrc++;
        c3 = *pSrc++;

        *pDst++ = EnBase64Tab[c1 >> 2];
        *pDst++ = EnBase64Tab[((c1 << 4) | (c2 >> 4)) & 0x3f];
        *pDst++ = EnBase64Tab[((c2 << 2) | (c3 >> 6)) & 0x3f];
        *pDst++ = EnBase64Tab[c3 & 0x3f];
        nLineLen += 4;
        nDstLen += 4;

        if (change_line && nLineLen > 72) {
            *pDst++ = '\r';
            *pDst++ = '\n';
            nLineLen = 0;
            nDstLen += 2;
        }
    }

    if (nMod == 1) {
        c1 = *pSrc++;
        *pDst++ = EnBase64Tab[(c1 & 0xfc) >> 2];
        *pDst++ = EnBase64Tab[((c1 & 0x03) << 4)];
        *pDst++ = '=';
        *pDst++ = '=';
        nLineLen += 4;
        nDstLen += 4;

    } else if (nMod == 2) {
        c1 = *pSrc++;
        c2 = *pSrc++;
        *pDst++ = EnBase64Tab[(c1 & 0xfc) >> 2];
        *pDst++ = EnBase64Tab[((c1 & 0x03) << 4) | ((c2 & 0xf0) >> 4)];
        *pDst++ = EnBase64Tab[((c2 & 0x0f) << 2)];
        *pDst++ = '=';
        nDstLen += 4;
    }

    *pDst = '\0';

    return nDstLen;
}

static int base64_decode(const char* pSrc, int nSrcLen, unsigned char* pDst) {
    int nDstLen;
    int nValue;
    int i;
    i = 0;
    nDstLen = 0;

    while (i < nSrcLen) {
        if (*pSrc != '\r' && *pSrc != '\n') {
            if (i + 4 > nSrcLen) break;

            nValue = DeBase64Tab[int(*pSrc++)] << 18;
            nValue += DeBase64Tab[int(*pSrc++)] << 12;
            *pDst++ = (nValue & 0x00ff0000) >> 16;
            nDstLen++;
            if (*pSrc != '=') {
                nValue += DeBase64Tab[int(*pSrc++)] << 6;
                *pDst++ = (nValue & 0x0000ff00) >> 8;
                nDstLen++;
                if (*pSrc != '=') {
                    nValue += DeBase64Tab[int(*pSrc++)];
                    *pDst++ = nValue & 0x000000ff;
                    nDstLen++;
                }
            }

            i += 4;
        } else {
            pSrc++;
            i++;
        }
    }

    *pDst = '\0';
    return nDstLen;
}
} // namespace xx

std::string base64_encode(const std::string& data,
                         bool change_line/* = false*/) {
    if (data.empty()) return std::string();

    char *pDst = NULL;
    int iBufSize = (int) (data.size() * 1.4) + 6;
    pDst = new char[iBufSize];
    if (pDst == NULL) return std::string();

    int iDstLen = xx::base64_encode((unsigned char*) data.c_str(), data.size(),
                                    pDst, change_line);
    std::string ret(pDst, iDstLen);
    delete[] pDst;
    return ret;
}

std::string base64_decode(const std::string& data) {
    if (data.empty()) return std::string();

    unsigned char *pDst = NULL;
    pDst = new unsigned char[data.size()];
    if (pDst == NULL) return std::string();

    int iDstLen = xx::base64_decode(data.c_str(), data.size(), pDst);
    std::string ret((char*) pDst, iDstLen);
    delete[] pDst;
    return ret;
}

int base64_encode(const char* src, int src_len, char* dst,
                  bool change_line) {
    return xx::base64_encode((const unsigned char*) src, src_len, dst, change_line);
}

int base64_decode(const char* src, int src_len, char* dst) {
    return xx::base64_decode(src, src_len, (unsigned char*) dst);
}
