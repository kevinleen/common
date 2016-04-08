#pragma once

#include <string>
#include <cassert>

#include "../string_util.h"
#include "../data_types.h"

/*
 * This is an OpenSSL-compatible implementation of the RSA Data Security, Inc.
 * MD5 Message-Digest Algorithm (RFC 1321).
 *
 * Homepage:
 * http://openwall.info/wiki/people/solar/software/public-domain-source-code/md5
 *
 * Author:
 * Alexander Peslyak, better known as Solar Designer <solar at openwall.com>
 *
 * This software was written by Alexander Peslyak in 2001.  No copyright is
 * claimed, and the software is hereby placed in the public domain.
 * In case this attempt to disclaim copyright and place the software in the
 * public domain is deemed null and void, then the software is
 * Copyright (c) 2001 Alexander Peslyak and it is hereby released to the
 * general public under the following terms:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted.
 *
 * There's ABSOLUTELY NO WARRANTY, express or implied.
 *
 * See md5.c for more information.
 */

/* Any 32-bit or wider unsigned integer data type will do */
typedef unsigned int MD5_u32plus;

typedef struct {
    MD5_u32plus lo, hi;
    MD5_u32plus a, b, c, d;
    unsigned char buffer[64];
    MD5_u32plus block[16];
} MD5_CTX;

extern void MD5_Init(MD5_CTX *ctx);
extern void MD5_Update(MD5_CTX *ctx, const void *data, unsigned long size);
extern void MD5_Final(unsigned char *result, MD5_CTX *ctx);

class Md5Hash {
  public:
    Md5Hash() {
      MD5_Init(&_ctx);
    }
    ~Md5Hash() {
    }

    void update(const char* data, uint32 len) {
      MD5_Update(&_ctx, (const u_char *) data, len);
    }
    void update(const std::string& data) {
      update(data.c_str(), data.size());
    }

    const std::string toString();

  private:
    MD5_CTX _ctx;

    DISALLOW_COPY_AND_ASSIGN(Md5Hash);
};

inline void md5Encode(const char* data, uint32 len, std::string* out) {
  assert(data != NULL);
  Md5Hash md5;
  md5.update(data, len);
  *out = md5.toString();
}

inline void md5Encode(const std::string& data, std::string* out) {
  md5Encode(data.data(), data.size(), out);
}

std::string md5sum(const std::string& path);
