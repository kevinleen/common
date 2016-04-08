#pragma once

#include "data_types.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>

class StreamBuf {
  public:
    explicit StreamBuf(uint32 size = 32) {
        _pbeg = (char*) ::malloc(size);
        _pcur = _pbeg;
        _pend = _pbeg + size;
    }

    ~StreamBuf() {
        ::free(_pbeg);
    }

    uint32 size() const {
        return static_cast<uint32>(_pcur - _pbeg);
    }

    uint32 capacity() const {
        return static_cast<uint32>(_pend - _pbeg);
    }

    bool empty() const {
        return _pcur == _pbeg;
    }

    // not null-terminated
    const char* data() const {
        return _pbeg;
    }

    std::string to_string() const {
        return std::string(this->data(), this->size());
    }

    void clear() {
        _pcur = _pbeg;
    }

    void resize(uint32 n) {
        uint32 size = this->size();
        if (n > size) ::memset(_pcur, 0, n - size);
        _pcur = _pbeg + n;
    }

    bool reserve(uint32 n) {
        uint32 size = this->size();
        if (n <= size) return true;

        char* p = (char*) ::realloc(_pbeg, n);
        if (p == NULL) return false;

        _pbeg = p;
        _pcur = p + size;
        _pend = p + n;

        return true;
    }

    StreamBuf& append(const void* data, uint32 size) {
        if (static_cast<uint32>(_pend - _pcur) < size) {
            if (!this->reserve(this->capacity() + size + 32)) {
                return *this;
            }
        }

        ::memcpy(_pcur, data, size);
        _pcur += size;
        return *this;
    }

    StreamBuf& operator<<(bool v) {
        return v ? this->append("true", 4) : this->append("false", 5);
    }

    StreamBuf& operator<<(char v) {
        return this->append(&v, 1);
    }

    StreamBuf& operator<<(unsigned char v) {
        return this->write("%u", v);
    }

    StreamBuf& operator<<(short v) {
        return this->write("%d", v);
    }

    StreamBuf& operator<<(unsigned short v) {
        return this->write("%u", v);
    }

    StreamBuf& operator<<(int v) {
        return this->write("%d", v);
    }

    StreamBuf& operator<<(unsigned int v) {
        return this->write("%u", v);
    }

    StreamBuf& operator<<(long v) {
        return this->write("%ld", v);
    }

    StreamBuf& operator<<(unsigned long v) {
        return this->write("%lu", v);
    }

    StreamBuf& operator<<(long long v) {
        return this->write("%lld", v);
    }

    StreamBuf& operator<<(unsigned long long v) {
        return this->write("%llu", v);
    }

    StreamBuf& operator<<(float v) {
        return this->write("%.7g", v);
    }

    StreamBuf& operator<<(double v) {
        return this->write("%.7g", v);
    }

    StreamBuf& operator<<(const char* v) {
        return this->append(v, ::strlen(v));
    }

    StreamBuf& operator<<(const std::string& v) {
        return this->append(v.data(), v.size());
    }

    StreamBuf& operator<<(const void* v) {
        return this->write("0x%llx", v);
    }

    operator bool() const {
        return false;
    }

  private:
    template<typename T>
    StreamBuf& write(const char* fm, T t) {
        int x = static_cast<int>(_pend - _pcur);
        int r = ::snprintf(_pcur, x, fm, t);
        if (r < 0) return *this;

        if (r < x) {
            _pcur += r;
            return *this;
        }

        if (this->reserve(this->capacity() + r + 32)) {
            r = ::snprintf(_pcur, _pend - _pcur, fm, t);
            if (r >= 0) {
                _pcur += r;
                return *this;
            }
        }

        return *this;
    }

  private:
    char* _pbeg;
    char* _pcur;
    char* _pend;

    DISALLOW_COPY_AND_ASSIGN(StreamBuf);
};
