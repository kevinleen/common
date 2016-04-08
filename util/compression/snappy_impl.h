#pragma once

#include "compression.h"

namespace util {

class SnappyCompression : public Compression {
  public:
    SnappyCompression() {
    }
    virtual ~SnappyCompression() {
    }

  private:
    virtual void deflate(const io::ReadonlyStream* in,
                         io::SourceConcatenater* out);
    virtual void inflate(const io::ReadonlyStream* in,
                         io::SourceConcatenater* out);

    DISALLOW_COPY_AND_ASSIGN(SnappyCompression);
};

}
