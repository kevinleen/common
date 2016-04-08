#pragma once

#include <string>

//#define XXTEA_MX ((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4)) ^ ((sum ^ y) + ((k[(p & 3) ^ e]) ^ z))
//#define XXTEA_DELTA 0x9e3779b9

std::string xxtea_encode(const std::string& data, const std::string& key);
std::string xxtea_decode(const std::string& data, const std::string& key);

int xxtea_encode(const char* str, int slen, const char* key, int keylen,
                 char* enc_chr, int enclen);

int xxtea_decode(const char* str, int slen, const char* key, int keylen,
                 char* dec_chr, int declen);
