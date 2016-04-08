#include "xxtea.h"
#include <stdlib.h>
#include <string.h>
#include <vector>

namespace xx {
static int xxtea_encode(const unsigned char* str, int slen, const unsigned char* key, int keylen, unsigned char* enc_chr, int enclen)
{
    unsigned int* v = NULL;
    int vlen;

    int n;
    int q;

    unsigned int z;
    unsigned int y;
    unsigned int delta;
    unsigned int mx, e;
    unsigned int sum;
    int i;
    unsigned int k[4];

    vlen = (slen & 3);
    if(vlen == 0){
        vlen = slen + 4;
    }
    else{
        vlen = slen + 8 - vlen;
    }

    if(enc_chr == NULL){
        return vlen;
    }
    if(enclen < vlen) return -1;

    enclen = vlen;

    if(enc_chr != str){
        memcpy(enc_chr, str, slen * sizeof(unsigned char));
    }


    v = (unsigned int*)enc_chr;
    vlen = enclen >> 2;

    v[vlen - 1] = slen;

    if((slen & 3) != 0){
        memset((enc_chr + slen), 0, (4 - (slen & 3)) * sizeof(unsigned char));
    }

    memcpy(k, key, keylen);

    n = vlen - 1;
    z = v[n];
    y = v[0];
    delta = 0x9E3779B9;
    q = 6 + 52 / (n + 1);
    sum = 0;
    while (0 < q--) {
        sum = sum + delta ;
        e = sum >> 2 & 3;
        for (i = 0; i < n; i++) {
            y = v[i + 1];
            mx = (unsigned int)(z >> 5 ^ y << 2) + (unsigned int)(y >> 3 ^ z << 4) ^ (sum ^ y) + (unsigned int)(k[(((unsigned int)i) & 3) ^ e] ^ z);
            z = v[i] = v[i] + mx ;
        }
        y = v[0];
        mx = (z >> 5 ^ y << 2) + (y >> 3 ^ z << 4) ^ (sum ^ y) + (k[(((unsigned int)i) & 3) ^ e] ^ z);
        z = v[n] = v[n] + mx & 0xffffffff;
    }

    return vlen << 2;
}

static int xxtea_decode(const unsigned char* str, int slen, const unsigned char* key, int keylen, unsigned char* dec_chr, int declen) {
    unsigned int* v = NULL;
    int vlen;
    int i;

    int n;
    unsigned int z;
    unsigned int y;
    unsigned int delta;
    unsigned int mx, e;
    int q;
    unsigned int sum;
    unsigned int k[4];

    vlen = slen;
    if((slen & 3) != 0){
        return -1;
    }

    if(dec_chr == NULL){
        return vlen;
    }
    if(declen < vlen) return -2;

    declen = vlen;

    if(dec_chr != str){
        memcpy(dec_chr, str, slen * sizeof(unsigned char));
    }

    v = (unsigned int*)dec_chr;
    vlen = declen >> 2;

    memcpy(k, key, keylen);

    n = vlen - 1;
    if (n == 0) {
        return -3;
    }
    z = v[n - 1];
    y = v[0];
    delta = 0x9E3779B9;
    q = 6 + 52 / (n + 1);
    sum = q * delta;

    while (sum != 0) {
        e = sum >> 2 & 3;
        for (i = n; i > 0; i--) {
            z = v[i - 1];
            mx = (unsigned int)(z >> 5 ^ y << 2) + (unsigned int)(y >> 3 ^ z << 4) ^ (sum ^ y) + (unsigned int)(k[(((unsigned int)i) & 3) ^ e] ^ z);
            y = v[i] = v[i] - mx;
        }
        z = v[n];
        mx = (z >> 5 ^ y << 2) + (y >> 3 ^ z << 4) ^ (sum ^ y) + (k[(((unsigned int)i) & 3) ^ e] ^ z);
        y = v[0] = v[0] - mx;
        sum = sum - delta;
    }

    n = v[vlen - 1];

    if(n >= 0 && n <= (vlen - 1) * 4){
        dec_chr[n] = '\0';
        return n;
    }
    return -4;
}
} // namespace xx

std::string xxtea_encode(const std::string &data, const std::string &key)
{
    int len = data.length()+8;
    std::vector<unsigned char> enc;
    enc.resize(len);

    len = xx::xxtea_encode((unsigned char *)data.c_str(), data.length(), (unsigned char *)key.c_str(), 16, &enc[0], len);
    std::string s((char *)&enc[0], len);

    return s;
}

std::string xxtea_decode(const std::string &data, const std::string &key)
{
    int len = data.length();
    std::vector<unsigned char> tbs;
    tbs.resize(len);

    len = xx::xxtea_decode((unsigned char *)data.c_str(), data.length(), (unsigned char *)key.c_str(), 16, &tbs[0], len);
    std::string s((char *)&tbs[0], len);

    return s;
}

int xxtea_encode(const char* str, int slen, const char* key, int keylen,
                 char* enc_chr, int enclen) {
    return xx::xxtea_encode((const unsigned char*) str, slen,
                            (const unsigned char*) key, keylen,
                            (unsigned char*) enc_chr, enclen);
}

int xxtea_decode(const char* str, int slen, const char* key, int keylen,
                 char* dec_chr, int declen) {
    return xx::xxtea_decode((const unsigned char*) str, slen,
                            (const unsigned char*) key, keylen,
                            (unsigned char*) dec_chr, declen);
}
