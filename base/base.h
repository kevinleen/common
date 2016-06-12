#pragma once

#include "ccflag/ccflag.h"
#include "cclog/cclog.h"
#include "cctest/cctest.h"

#include "data_types.h"
#include "shared_ptr.h"
#include "ascii_table.h"

#include "random.h"
#include "stream_buf.h"
#include "net_util.h"
#include "time_util.h"
#include "file_util.h"
#include "string_util.h"
#include "thread_util.h"
#include "signal_util.h"

#include "hash/md5.h"
#include "hash/crc16.h"
#include "hash/crc32.h"
#include "hash/xxtea.h"
#include "hash/base64.h"
#include "hash/xx_hash.h" // for xx hash.
#include "hash/uuid_util.h"
#include "hash/city_hash.h"
#include "hash/super_hash.h"
#include "hash/murmur_hash.h"

#include <sys/un.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>

#include <string>
#include <vector>
#include <map>
#include <bitset>
#include <deque>
#include <set>
#include <list>
#include <unordered_set>
#include <unordered_map>
#include <memory>
#include <functional>

namespace std {
using namespace std::placeholders;
}

#include <algorithm>

#define v16(c) (*(uint16*)c)
#define v32(c) (*(uint32*)c)
#define v64(c) (*(uint64*)c)
#define save16(c, v) (*((uint16*)c)  = v)
#define save32(c, v) (*((uint32*)c)  = v)
#define save64(c, v) (*((uint64*)c) = v)

#define setFdBlock(fd) \
  ::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) & ~O_NONBLOCK)
#define setFdNonBlock(fd) \
  ::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) | O_NONBLOCK)
#define setFdCloExec(fd) \
  ::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) | O_EXCL)

// stl utilites
template<typename T> inline void STLClear(T* t) {
  for (auto it = t->begin(); it != t->end(); ++it) {
    delete *it;
  }
  t->clear();
}
template<typename T> inline void STLMapClear(T* t) {
  for (auto it = t->begin(); it != t->end(); ++it) {
    delete it->second;
  }
  t->clear();
}

template<typename T> inline void STLUnRef(T* t) {
  for (auto it = t->begin(); it != t->end(); ++it) {
    it->Ref();
  }
  t->clear();
}
template<typename T> inline void STLMapUnRef(T* t) {
  for (auto it = t->begin(); it != t->end(); ++it) {
    it->second->UnRef();
  }
  t->clear();
}

template<typename T, typename K> inline void STLEarseAndDelete(T*t,
                                                               const K& k) {
  auto it = t->find(k);
  if (it != t->end()) {
    delete (*it);
    t->erase(it);
  }
}
template<typename T, typename K> inline void STLMapEarseAndDelete(T*t,
                                                                  const K& k) {
  auto it = t->find(k);
  if (it != t->end()) {
    delete it->second;
    t->erase(it);
  }
}

template<typename T, typename K> inline void STLEarse(T* t, const K& k) {
  auto item = t->find(k);
  if (item != t->end()) {
    t->erase(item);
  }
}
template<typename T, typename K> inline void STLMapEarse(T*t, const K& k) {
  auto it = t->find(k);
  if (it != t->end()) {
    t->erase(it);
  }
}

template<typename T, typename K> inline void STLEarseAndUnRef(T*t, const K& k) {
  auto it = t->find(k);
  if (it != t->end()) {
    (*it)->UnRef();
    t->earse(it);
  }
}
template<typename T, typename K> inline void STLMapEarseAndUnRef(T*t,
                                                                 const K& k) {
  auto it = t->find(k);
  if (it != t->end()) {
    it->second->UnRef();
    t->erase(it);
  }
}

template<typename T> inline void STLEraseFront(T*t, uint32 size) {
  if (t->size() < size) return;
  T tmp(t->begin() + size, t->end());
  *t = tmp;
}

template<typename S, typename D>
inline void STLAppend(const S& s, D* d) {
  for (auto it = s.begin(); it != s.end(); ++it) {
    d->insert(d->end(), *it);
  }
}

bool execCmd(const std::string& cmd, std::string* out);
