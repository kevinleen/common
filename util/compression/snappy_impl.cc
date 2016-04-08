#undef DISALLOW_COPY_AND_ASSIGN
#include <snappy.h>
#undef DISALLOW_COPY_AND_ASSIGN
#include "snappy_impl.h"

#include "rpc/io/source_impl.h"
#include "rpc/io/readonly_stream.h"

namespace util {

void SnappyCompression::deflate(const io::ReadonlyStream* in,
                                io::SourceConcatenater* out) {
  const char* data;
  uint64 size = kBufferSize;
  for (; in->read(&data, &size); size = kBufferSize) {
    if (size == 0) continue;

    uint64 max_size = snappy::MaxCompressedLength(size);
    io::AppendOnlyStream* entry = new io::AppendOnlyStream;
    entry->ensureLeft(max_size);
    snappy::RawCompress(data, size, entry->peekW(), &max_size);
    entry->skipWrite(max_size);

    entry->push();
    out->push(entry);
  }
}

void SnappyCompression::inflate(const io::ReadonlyStream* in,
                                io::SourceConcatenater* out) {
  const char* data;
  uint64 size = kBufferSize;
  for (; in->read(&data, &size); size = kBufferSize) {
    if (size == 0) continue;

    uint64 dst_len;
    if (!snappy::GetUncompressedLength(data, size, &dst_len)) {
      ELOG<< "decode compressed length error";
      return;
    }

    io::AppendOnlyStream* entry = new io::AppendOnlyStream;
    entry->ensureLeft(dst_len);
    snappy::RawUncompress(data, size, entry->peekW());
    entry->skipWrite(dst_len);

    entry->push();
    out->push(entry);
  }
}

}

