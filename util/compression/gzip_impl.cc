#include "gzip_impl.h"
#include "rpc/io/source_impl.h"
#include "rpc/io/readonly_stream.h"

namespace util {

GzipCompression::GzipCompression(cmp_type type)
    : _type(type) {
  _stream.next_in = NULL;
  _stream.avail_in = 0;
  _stream.total_in = 0;
  _stream.next_out = NULL;
  _stream.avail_out = 0;
  _stream.total_out = 0;
  _stream.msg = NULL;
  _stream.state = NULL;
  _stream.zalloc = NULL;
  _stream.zfree = NULL;
  _stream.opaque = NULL;
  _stream.data_type = 0;
  _stream.adler = 0;
  _stream.reserved = 0;
}

bool GzipCompression::deflateFragment(io::Source** data) {
  scoped_ptr<io::AppendOnlyStream> src(new io::AppendOnlyStream);
  src->ensureLeft(_stream.avail_in);

  while (_stream.avail_in != 0) {
    _stream.next_out = (Bytef*) src->peekW();
    uint32 left_size = src->writeableSize();
    _stream.avail_out = left_size;

    int ret = ::deflate(&_stream, Z_SYNC_FLUSH);
    src->skipWrite(left_size - _stream.avail_out);
    switch (ret) {
      case Z_OK:
        CHECK_EQ(_stream.avail_in, 0);
        *data = src.release();
        return false;
      case Z_STREAM_END:
        CHECK_EQ(_stream.avail_in, 0);
        *data = src.release();
        return true;
      default:
        ELOG<< "deflate error, ret: " << ret;
        return true;
      }
    }

  return false;
}

void GzipCompression::deflate(const io::ReadonlyStream* in,
                              io::SourceConcatenater* out) {
  deflateInit(&_stream, Z_DEFAULT_COMPRESSION);

  const char* data;
  uint64 len;
  while (in->read(&data, &len)) {
    if (len == 0) continue;
    _stream.next_in = (Bytef*) data;
    _stream.avail_in = len;

    io::Source* src;
    bool is_end = deflateFragment(&src);
    if (src != NULL) out->push(src);
    if (is_end) {
      ::deflateEnd(&_stream);
      break;
    }
  }
}

bool GzipCompression::inflateFragment(io::Source** data) {
  scoped_ptr<io::AppendOnlyStream> src(new io::AppendOnlyStream);
  src->ensureLeft(_stream.avail_in);

  while (_stream.avail_in != 0) {
    _stream.next_out = (Bytef*) src->peekW();
    uint32 left_size = src->writeableSize();
    _stream.avail_out = left_size;

    int ret = ::inflate(&_stream, Z_SYNC_FLUSH);
    src->skipWrite(left_size - _stream.avail_out);
    switch (ret) {
      case Z_OK:
        CHECK_EQ(_stream.avail_in, 0);
        *data = src.release();
        return false;
      case Z_STREAM_END:
        CHECK_EQ(_stream.avail_in, 0);
        *data = src.release();
        return true;
      default:
        ELOG<< "deflate error, ret: " << ret;
        return true;
      }
    }

  return false;
}

void GzipCompression::inflate(const io::ReadonlyStream* in,
                              io::SourceConcatenater* out) {
  inflateInit2(&_stream, _type == ZLIB ? 15 : 15 | 16);
  const char* data;
  uint64 len;
  while (in->read(&data, &len)) {
    if (len == 0) continue;
    _stream.next_in = (Bytef*) data;
    _stream.avail_in = len;

    io::Source* src;
    bool is_end = inflateFragment(&src);
    if (src != NULL) out->push(src);
    if (is_end) {
      ::inflateEnd(&_stream);
      break;
    }
  }
}

}
