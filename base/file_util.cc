#include "file_util.h"

#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <sys/mman.h>

namespace {

bool Stat(const std::string& path, struct stat* st) {
  int fd;
  if (!openFile(path, &fd, O_RDONLY)) return false;

  int ret = ::fstat(fd, st);
  closeWrapper(fd);
  if (ret != -1) {
    WLOG<< "fstat error, path: " << path;
    return true;
  }

  return false;
}
}

bool openFile(const std::string& fpath, int* fd, int flag, int mode) {
  int ret = ::open(fpath.c_str(), flag, mode);
  if (ret == kInvalidFd) {
    WLOG<< "open error, path: " << fpath;
    return false;
  }

  *fd = ret;
  return true;
}

bool readFile(const std::string& path, std::string* data) {
  data->clear();

  int fd;
  if (!openFile(path, &fd, O_RDONLY)) return false;
  uint64 size;
  if (!FileSize(fd, &size)) {
    closeWrapper(fd);
    return false;
  }

  if (size == 0) {
    closeWrapper(fd);
    return true;
  }

  data->resize(size);
  while (true) {
    int64 ret = ::read(fd, &data->at(0), size);
    if (ret == size) break;
    else if (ret == -1) {
      switch (errno) {
        case EINTR:
          continue;
      }
    }

    WLOG<< "read error, path: " << path;
    closeWrapper(fd);
    return false;
  }

  closeWrapper(fd);
  return true;
}

bool writeFile(const std::string& path, const std::string& data) {
  int fd;
  if (!openFile(path, &fd, O_WRONLY | O_SYNC | O_TRUNC | O_CREAT)) {
    return false;
  }

  while (true) {
    int ret = ::write(fd, data.data(), data.size());
    if (ret == data.size()) break;
    else if (ret == -1) {
      switch (errno) {
        case EINTR:
          continue;
      }

      WLOG<< "write error, path: " << path;
      ::close(fd);
      return false;
    }
  }

  closeWrapper(fd);
  return true;
}

void RemoveFile(const std::string& path) {
  if (FileExist(path)) {
    int ret = ::unlink(path.c_str());
    if (ret != 0) WLOG<< "unlink failed: " << path;
  }
}

bool FileExist(const std::string& path) {
  return ::access(path.c_str(), F_OK) == 0;
}

bool IsDir(const std::string& path) {
  struct stat st;
  if (!Stat(path, &st)) return false;
  return S_ISDIR(st.st_mode);
}

bool IsRegular(const std::string& path) {
  struct stat st;
  if (!Stat(path, &st)) return false;
  return S_ISREG(st.st_mode);
}

bool FileSize(int fd, uint64* size) {
  struct stat st;
  int ret = ::fstat(fd, &st);
  if (ret != -1) {
    *size = st.st_size;
    return true;
  }

  WLOG<< "stat error, fd: " << fd;
  return false;
}

bool FileSize(const std::string& path, uint64* size) {
  int fd = kInvalidFd;
  if (!openFile(path, &fd, O_RDONLY)) return false;

  bool ret = FileSize(fd, size);
  closeWrapper(fd);
  return ret;
}

bool FlushData(int fd) {
  int ret = ::fdatasync(fd);
  if (ret == -1) {
    WLOG<< "fdatasync error, fd: " << fd;
    return false;
  }
  return true;
}

bool FlushFile(int fd) {
  int ret = ::fsync(fd);
  if (ret == -1) {
    WLOG<< "fsync error, fd: " << fd;
    return false;
  }
  return true;
}

bool FileTruncate(int fd, uint64 size) {
  CHECK_GT(size, 0);
  int ret = ::ftruncate(fd, size);
  if (ret != 0) {
    WLOG<< "ftruncate error, fd: " << fd << " size: " << size;
    return false;
  }
  return true;
}

bool FileTruncate(const std::string& path, uint64 size) {
  int ret = ::truncate(path.c_str(), size);
  if (ret != 0) {
    WLOG<< "ftruncate error, path: " << path << " size: " << size;
    return false;
  }
  return true;
}

bool SequentialReadonlyFile::Init() {
  if (_stream != NULL) return false;

  _stream = ::fopen(_fpath.c_str(), "r");
  if (_stream == NULL) {
    WLOG<< "fopen error, path: " << _fpath;
    return false;
  }
  return true;
}

void SequentialReadonlyFile::skip(uint64 len) {
  int ret = ::fseek(_stream, len, SEEK_SET);
  if (ret != 0) {
    WLOG<< "fseek error: " << _fpath;
    abort();
  }
}

int32 SequentialReadonlyFile::read(char* buf, uint32 len) {
  CHECK_GE(len, 0);
  int32 readn = ::fread(buf, 1, len, _stream);
  if (readn == -1) {
    WLOG<< "fread error, path: " << _fpath;
    return -1;
  }

  return readn;
}

bool RandomAccessFile::Init() {
  if (_fd != kInvalidFd) return true;
  return openFile(_fpath, &_fd, O_RDWR);
}

int32 RandomAccessFile::read(char* buf, uint32 len, off_t offset) {
  uint32 left = len;
  while (left > 0) {
    int32 readn = ::pread(_fd, buf, left, offset);
    if (readn == 0) return 0;
    else if (readn == -1) {
      if (errno == EINTR) continue;
      WLOG<< "pread error, path: " << _fpath;
      return -1;
    }

    buf += readn;
    left -= readn;
    offset += readn;
  }
  return len - left;
}

bool RandomAccessFile::flush(bool only_flush_data) {
  if (_fd != kInvalidFd) {
    if (only_flush_data) return FlushData(_fd);
    return FlushFile(_fd);
  }
  return false;
}

int32 RandomAccessFile::write(const char* buf, uint32 len, off_t offset) {
  uint32 left = len;
  while (left > 0) {
    int32 writen = ::pwrite(_fd, buf, left, offset);
    if (writen == -1) {
      if (errno == EINTR) continue;
      WLOG<< "pread error, path: " << _fpath;
      return -1;
    }

    buf += writen;
    left -= writen;
    offset += writen;
  }
  return len - left;
}

bool AppendonlyFile::Init() {
  _stream = ::fopen(_fpath.c_str(), "a");
  if (_stream == NULL) {
    WLOG<< "fopen error";
    return false;
  }

  return true;
}

bool AppendonlyFile::flush() {
  if (_stream != NULL) {
    ::fflush(_stream);
    return true;
  }
  return false;
}

int32 AppendonlyFile::write(const char* buf, uint32 len) {
  if (_stream != NULL) {
    int32 ret = ::fwrite(buf, len, 1, _stream);
    if (ret == -1) {
      WLOG<< "fwrite error";
      return -1;
    }
    return ret;
  }

  return -1;
}

AppendonlyMmapedFile::~AppendonlyMmapedFile() {
  if (_fd != kInvalidFd) {
    if (_pos != _end) {
      uint64 file_size = _mapped_offset - (_end - _pos);
      CHECK(FileTruncate(_fd, file_size));
    }

    closeWrapper(_fd);
  }
}

bool AppendonlyMmapedFile::Init() {
//  uint32 flags = O_APPEND | O_CREAT | O_CLOEXEC | O_WRONLY;
  uint32 flags = O_RDWR | O_CREAT | O_CLOEXEC | O_TRUNC;
  if (!openFile(_fpath, &_fd, flags)) {
    return false;
  }
  CHECK_NE(_fd, kInvalidFd);
  if (!FileSize(_fd, &_mapped_offset)) {
    closeWrapper(_fd);
    return false;
  }

  return true;
}

bool AppendonlyMmapedFile::flush() {
  if (_fd == kInvalidFd) return false;
  if (_end != _mem) {
    int32 size = _end - _mem - _flushed_size;
    if (size > 0) {
      int ret = ::msync(_mem + _flushed_size, size, MS_SYNC);
      if (ret != 0) {
        WLOG<< "msync error";
        return false;
      }
      _flushed_size += size;
    }
  }
  return true;
}

int32 AppendonlyMmapedFile::write(const char* buf, uint32 len) {
  if (_fd == kInvalidFd) return -1;

  uint32 left = len;
  while (left > 0) {
    uint32 avail_size = _end - _pos;
    if (avail_size == 0) {
      if (!doMap()) return -1;
      continue;
    }

    uint32 writen = std::min(avail_size, left);
    if (writen > 0) {
      ::memcpy(_pos, buf, writen);
      _pos += writen;
      buf += writen;
      left -= writen;
    }
  }

  return len - left;
}

#ifdef __linux__
#define Mmap mmap64
#else
#define Mmap  mmap
#endif

bool AppendonlyMmapedFile::doMap() {
  if (_mem != NULL) unMap();

  bool ret = FileTruncate(_fd, _mapped_offset + _mapped_size);
  if (!ret) {
    return false;
  }
  _mem = (char*) Mmap(NULL, _mapped_size, PROT_WRITE, MAP_SHARED, _fd,
                      _mapped_offset);
  if (_mem == MAP_FAILED) {
    WLOG<< "mmap64 error, fd: " << _fd;
    return false;
  }

  _pos = _mem;
  _end = _mem + _mapped_size;

  _flushed_size = 0;
  _mapped_offset += _mapped_size;

  return true;
}

void AppendonlyMmapedFile::unMap() {
  if (_mem != NULL) {
    ::munmap(_mem, _mapped_size);
    _mem = _pos = _end = NULL;
    _flushed_size = 0;
  }
}

bool DirIterator::Init() {
  if (_dir != NULL) return false;

  _dir = ::opendir(_fpath.c_str());
  if (_dir == NULL) {
    WLOG<< "opendir error, path: " << _fpath;
    return false;
  }
  return true;
}

uint8 DirIterator::typeConvert(Type type) const {
  switch (type) {
    case REG_FILE:
      return DT_REG;
    case DIR_TYPE:
      return DT_DIR;
    case SYMBOLIC_LINK:
      return DT_LNK;
  }

  CHECK(false) << "should not here";
  return 0;
}

const std::string* DirIterator::next(Type type) {
  uint8 entry_type = typeConvert(type);
  while (next() != NULL) {
    if (_entry.d_type & entry_type) {
      return &_name_cache;
    }
  }
  return NULL;
}

const std::string* DirIterator::next() {
  if (_dir != NULL) {
    struct dirent* dummy;
    int ret = ::readdir_r(_dir, &_entry, &dummy);
    if (ret != 0) {
      WLOG<< "readdir_r error, path: " << _fpath;
      return NULL;
    }

    if (dummy != NULL) {
      _name_cache = _entry.d_name;
      return &_name_cache;
    }
  }

  return NULL;
}

bool FileLocker::openFile(int mode) {
  CHECK_EQ(_fd, kInvalidFd);
  if (FileExist(_path)) {
    _fd = ::open(_path.c_str(), mode);
    if (_fd == kInvalidFd) {
      WLOG<< "open exist file error: " << _path;
      return false;
    }
    return true;
  }

  _fd = ::open(_path.c_str(), O_CREAT);
  if (_fd == kInvalidFd) {
    WLOG<< "open error: " << _path;
    return false;
  }

  return true;
}

void FileLocker::readLock() {
  CHECK(openFile(O_RDONLY));
  int ret = ::flock(_fd, LOCK_SH);
  if (ret != 0) {
    WLOG<< "flock error: " << _path;
    ::abort();
  }
}

void FileLocker::writeLock() {
  CHECK(openFile(O_WRONLY));
  int ret = ::flock(_fd, LOCK_EX);
  if (ret != 0) {
    WLOG<< "flock error: " << _path;
    ::abort();
  }
}

void FileLocker::unLock() {
  CHECK_NE(_fd, kInvalidFd);
  int ret = ::flock(_fd, LOCK_UN);
  if (ret != 0) {
    WLOG<< "unlock error: " << _path;
    ::abort();
  }
}
