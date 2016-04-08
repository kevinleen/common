#pragma once

#include "data_types.h"
#include "cclog/cclog.h"

#include <cstdio>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <string>
#include <fstream>
#include <vector>

namespace sys {
const char path_separ = '/';

inline bool file_exist(const std::string& path) {
    return ::access(path.c_str(), F_OK) == 0;
}

inline int64 mtime(const std::string& path) {
    struct stat st;
    return ::stat(path.c_str(), &st) == 0 ? st.st_mtim.tv_sec : -1;
}

inline int64 file_size(const std::string& path) {
    struct stat st;
    return ::stat(path.c_str(), &st) == 0 ? st.st_size : -1;
}

inline bool isdir(const std::string& path) {
    struct stat st;
    return ::stat(path.c_str(), &st) == 0 ? S_ISDIR(st.st_mode) : false;
}

inline bool symlink(const std::string& path, const std::string& link) {
    ::unlink(link.c_str());
    return ::symlink(path.c_str(), link.c_str()) == 0;
}

inline std::string readlink(const std::string& path) {
    std::string s(256, '\0');
    int r = ::readlink(path.c_str(), &s[0], s.size());
    if (r < 0) return std::string();

    s.resize(r);
    return s;
}

inline std::pair<std::string, std::string> split_path(const std::string& path) {
    size_t pos = path.rfind(path_separ);
    if (pos == std::string::npos) {
        return std::make_pair(std::string(), path);
    }

    if (pos == 0) {
        return std::make_pair(std::string(1, path_separ), path.substr(1));
    }

    return std::make_pair(path.substr(0, pos), path.substr(pos + 1));
}

inline std::string get_prog_name() {
    return sys::split_path(sys::readlink("/proc/self/exe")).second;
}

class file_base {
  public:
    bool exist() const {
        return sys::file_exist(_path);
    }

    int64 mtime() const {
        return sys::mtime(_path);
    }

    void update_path(const std::string& path) {
        _path = path;
        if (!_path.empty()) _mtime = this->mtime();
    }

    bool modified() {
        int64 mt = this->mtime();
        if (mt == _mtime) return false;

        _mtime = mt;
        return true;
    }

    int64 size() const {
        return sys::file_size(_path);
    }

    bool is_dir() const {
        return sys::isdir(_path);
    }

    const std::string& path() const {
        return _path;
    }

  protected:
    explicit file_base(const std::string& path)
        : _path(path) {
    }

    file_base() = default;
    ~file_base() = default;

    file_base(file_base&& x)
        : _path(std::move(x._path)), _mtime(std::move(x._mtime)) {
    }

    file_base& operator=(file_base&& x) {
        if (&x != this) {
            _path = std::move(x._path);
            _mtime = std::move(x._mtime);
        }
        return *this;
    }

    std::string _path;
    int64 _mtime;

    //DISALLOW_COPY_AND_ASSIGN(file_base);
};

/*
 * for reading file line by line
 */
class ifile : public file_base {
  public:
    explicit ifile(const std::string& path)
        : file_base(path), _ifs(path), _line(0) {
    }

    ifile() : _line(0) {
    }

    ~ifile() = default;

    ifile(const ifile&) = delete;
    ifile& operator=(const ifile&) = delete;

    ifile(ifile&&) = delete;
    ifile& operator=(ifile&&) = delete;

    bool valid() const {
        return _ifs;
    }

    std::string getline() {
        std::string s;
        if (std::getline(_ifs, s)) ++_line;
        return s;
    }

    int line() const {
        return _line;
    }

    void close() {
        _ifs.close();
    }

    bool open(const std::string& path) {
        _ifs.open(path.c_str());
        if (!this->valid()) return false;

        _line = 0;
        this->update_path(path);
        return true;
    }

    bool reopen() {
        this->close();
        if (_path.empty()) return false;
        return this->open(_path);
    }

  private:
    std::ifstream _ifs;
    int _line;
};

/*
 * base class for rfile and wfile
 */
class rwfile : public file_base {
  public:
    rwfile() {
        _f = NULL;
    }

    ~rwfile() {
        this->close();
    }

    rwfile(rwfile&& x)
        : file_base(std::move(x)), _f(std::move(x._f)) {
    }

    rwfile& operator=(rwfile&& x) {
        if (&x == this) return *this;

        file_base::operator=(std::move(x));
        _f = std::move(x._f);

        return *this;
    }

    bool valid() const {
        return _f != NULL;
    }

    void close() {
        if (!this->valid()) return;
        ::fclose(_f);
        _f = NULL;
    }

    ::FILE* fd() const {
        return _f;
    }

  protected:
    ::FILE* _f;

    //DISALLOW_COPY_AND_ASSIGN(rwfile);
};

/*
 * for reading file
 */
class rfile : public rwfile {
  public:
    explicit rfile(const std::string& path) {
        this->open(path);
    }

    rfile() = default;
    ~rfile() = default;

    rfile(rfile&& x)
        : rwfile(std::move(x)) {
    }

    rfile& operator=(rfile&& x) {
        if (&x != this) rwfile::operator=(std::move(x));
        return *this;
    }

    bool open(const std::string& path) {
        _f = ::fopen(path.c_str(), "r");
        if (!this->valid()) return false;

        this->update_path(path);
        return true;
    }

    uint32 read(void* buf, uint32 size) {
        return ::fread(buf, 1, size, _f);
    }

    //DISALLOW_COPY_AND_ASSIGN(rfile);
};

/*
 * for writing file
 */
class wfile : public rwfile {
  public:
    wfile(const std::string& path, const char* mode = "a") {
        this->open(path, mode);
    }

    wfile() = default;
    ~wfile() = default;

    wfile(wfile&& x)
        : rwfile(std::move(x)) {
    }

    wfile& operator=(wfile&& x) {
        if (&x != this) rwfile::operator=(std::move(x));
        return *this;
    }

    bool open(const std::string& path, const char* mode = "a") {
        _f = ::fopen(path.c_str(), mode);
        if (!this->valid()) return false;

        this->update_path(path);
        return true;
    }

    uint32 write(const void* buf, uint32 size) {
        return ::fwrite(buf, 1, size, _f);
    }

    uint32 write(const std::string& s) {
        return this->write(s.data(), s.size());
    }

    uint32 write(const char c) {
        return this->write(&c, 1);
    }

    void flush() {
        if (this->valid()) ::fflush(_f);
    }

    //DISALLOW_COPY_AND_ASSIGN(wfile);
};
} // namespace sys

namespace detail {

class FileAbstract {
  public:
    virtual ~FileAbstract() {
    }

    const std::string& fpath() const {
      return _fpath;
    }

    virtual bool Init() = 0;

  protected:
    explicit FileAbstract(const std::string& fpath)
        : _fpath(fpath) {
      CHECK(!fpath.empty());
    }

    const std::string _fpath;

  private:
    DISALLOW_COPY_AND_ASSIGN(FileAbstract);
};
}  // end for namespace named detail

void RemoveFile(const std::string& path);

bool IsDir(const std::string& path);
bool IsRegular(const std::string& path);
bool FileExist(const std::string& path);

bool FileSize(int fd, uint64* size);
bool FileSize(const std::string& path, uint64* size);

bool FlushData(int fd);
bool FlushFile(int fd);

bool FileTruncate(int fd, uint64 size);
bool FileTruncate(const std::string& path, uint64 size);

class AutoClosedFileHandle {
  public:
    AutoClosedFileHandle() {
      fd = kInvalidFd;
      self_close = true;
    }
    explicit AutoClosedFileHandle(int file_handle, bool auto_close = true)
        : fd(file_handle), self_close(auto_close) {
    }
    ~AutoClosedFileHandle() {
      close();
    }

    void close() {
      if (self_close) {
        closeWrapper(fd);
      }
    }

    int fd;
    bool self_close;

  private:
    DISALLOW_COPY_AND_ASSIGN(AutoClosedFileHandle);
};

class SequentialReadonlyFile : public detail::FileAbstract {
  public:
    explicit SequentialReadonlyFile(const std::string& fpath)
        : FileAbstract(fpath), _stream(NULL) {
    }
    ~SequentialReadonlyFile() {
      if (_stream != NULL) {
        ::fclose(_stream);
        _stream = NULL;
      }
    }

    virtual bool Init();

    void skip(uint64 len);
    int32 read(char* buf, uint32 len);

  private:
    FILE *_stream;

    DISALLOW_COPY_AND_ASSIGN(SequentialReadonlyFile);
};

class RandomAccessFile : public detail::FileAbstract {
  public:
    explicit RandomAccessFile(const std::string& fpath)
        : FileAbstract(fpath), _fd(kInvalidFd) {
    }
    explicit RandomAccessFile(int fd)
        : detail::FileAbstract("unknown"), _fd(fd) {
    }
    virtual ~RandomAccessFile() {
      closeWrapper(_fd);
    }

    virtual bool Init();

    int32 read(char* buf, uint32 len, off_t offset);

    bool flush(bool only_flush_data = true);
    int32 write(const char* buf, uint32 len, off_t offset);

  private:
    int _fd;

    DISALLOW_COPY_AND_ASSIGN(RandomAccessFile);
};

class writeableFile : public detail::FileAbstract {
  public:
    explicit writeableFile(const std::string& fpath)
        : detail::FileAbstract(fpath) {
    }
    virtual ~writeableFile() {
    }

    virtual bool Init() = 0;

    virtual bool flush() = 0;
    virtual int32 write(const char* buf, uint32 len) = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(writeableFile);
};

class AppendonlyFile : public writeableFile {
  public:
    explicit AppendonlyFile(const std::string& fpath)
        : writeableFile(fpath), _stream(NULL) {
    }
    virtual ~AppendonlyFile() {
      if (_stream != NULL) {
        ::fclose(_stream);
        _stream = NULL;
      }
    }

    virtual bool Init();

    virtual bool flush();
    virtual int32 write(const char* buf, uint32 len);

  private:
    FILE* _stream;

    DISALLOW_COPY_AND_ASSIGN(AppendonlyFile);
};

class AppendonlyMmapedFile : public writeableFile {
  public:
    explicit AppendonlyMmapedFile(const std::string& fpath, uint32 mapped_size =
                                      0)
        : writeableFile(fpath), _fd(kInvalidFd) {
      _mem = _pos = _end = NULL;
      _mapped_size = mapped_size == 0 ? kDefaultMappedSize : mapped_size;
      _flushed_size = 0;
      _mapped_offset = 0;
    }
    virtual ~AppendonlyMmapedFile();

    virtual bool Init();

    virtual bool flush();
    virtual int32 write(const char* buf, uint32 len);

  private:
    int _fd;

    char* _mem;
    char* _pos;
    char* _end;

    uint32 _mapped_size;
    uint32 _flushed_size;
    uint64 _mapped_offset;

    bool doMap();
    void unMap();

    const static uint32 kDefaultMappedSize = 8192;

    DISALLOW_COPY_AND_ASSIGN(AppendonlyMmapedFile);
};

// not threadsafe.
class DirIterator : public detail::FileAbstract {
  public:
    enum Type {
      REG_FILE = 0, DIR_TYPE, SYMBOLIC_LINK,
    };

    explicit DirIterator(const std::string& path)
        : detail::FileAbstract(path), _dir(NULL) {
    }
    virtual ~DirIterator() {
      if (_dir != NULL) {
        ::closedir(_dir);
        _dir = NULL;
      }
    }

    virtual bool Init();

    const std::string* next();
    const std::string* next(Type type);

  private:
    DIR* _dir;

    struct dirent _entry;
    std::string _name_cache;

    uint8 typeConvert(Type type) const;

    DISALLOW_COPY_AND_ASSIGN(DirIterator);
};

class FileLocker {
  public:
    explicit FileLocker(const std::string& path)
        : _path(path), _fd(kInvalidFd) {
      CHECK(!path.empty());
    }
    ~FileLocker() {
      if (_fd != kInvalidFd) {
        closeWrapper(_fd);
        RemoveFile(_path);
      }
    }

    void readLock();
    void writeLock();
    void unLock();

  private:
    const std::string _path;

    int _fd;
    bool openFile(int mode);

    DISALLOW_COPY_AND_ASSIGN(FileLocker);
};

bool openFile(const std::string& fpath, int* fd, int flag, int mode = 0644);

bool readFile(const std::string& path, std::string* data);
bool writeFile(const std::string& path, const std::string& data);
