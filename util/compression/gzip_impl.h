#pragma once

#include <zlib.h>
#include "compression.h"

namespace io {
class Source;
}

namespace util {

class GzipCompression : public Compression {
  public:
    enum cmp_type {
      ZLIB, GZIP,
    };

    explicit GzipCompression(cmp_type type = ZLIB);
    virtual ~GzipCompression() {
    }

  private:
    const cmp_type _type;

    z_stream_s _stream;

    virtual void deflate(const io::ReadonlyStream* in,
                         io::SourceConcatenater* out);
    virtual void inflate(const io::ReadonlyStream* in,
                         io::SourceConcatenater* out);

    bool deflateFragment(io::Source** data);
    bool inflateFragment(io::Source** data);

    DISALLOW_COPY_AND_ASSIGN(GzipCompression);
};

}

