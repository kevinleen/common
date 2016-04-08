#pragma once

#include "base/base.h"

namespace io {
class ReadonlyStream;
class SourceConcatenater;
}

namespace util {

class Compression {
  public:
    virtual ~Compression() {
    }

    // compress.
    virtual void deflate(const io::ReadonlyStream* in,
                         io::SourceConcatenater* out) = 0;
    // decompress.
    virtual void inflate(const io::ReadonlyStream* in,
                         io::SourceConcatenater* out) = 0;

  protected:
    const static uint32 kBufferSize = 64 * 1024;

    Compression() {
    }

  private:
    DISALLOW_COPY_AND_ASSIGN(Compression);
};

}
