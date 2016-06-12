#pragma once

#include "base/base.h"

namespace util {
template <typename Key>
class KvRandom {
  public:
    typedef uint32 Value;
    typedef std::map<Key, Value> KvMap;
    typedef std::map<Value, Key> VkMap;

    KvRandom()
        : _random(77111777), _n(0) {
    }

    KvRandom(const KvMap& kv_map)
        : _random(77111777), _n(0) {
        this->init(kv_map);
    }

    ~KvRandom() = default;

    void init(const KvMap& kv_map) {
        _kv_map = kv_map;
        this->update();
    }

    void add(const Key& key, Value value) {
        _kv_map[key] = value;
        this->update();
    }

    void erase(const Key& key) {
        _kv_map.erase(key);
        this->update();
    }

    // return next key
    Key next() {
        auto it = _vk_map.lower_bound(static_cast<Value>(_random.Uniform(_n)));
        if (it != _vk_map.end()) return it->second;

        return Key();
    }

  private:
    Random _random;
    KvMap _kv_map;
    VkMap _vk_map;

    int _n;

    void update() {
        _n = 0;
        _vk_map.clear();

        std::multimap<Value, Key> vk;

        for (auto it = _kv_map.begin(); it != _kv_map.end(); ++it) {
            if (it->second == 0) continue;
            _n += it->second;
            vk.insert(std::make_pair(it->second, it->first));
        }

        if (vk.empty()) return;

        Value key = -1;
        for (auto it = vk.begin(); it != vk.end(); ++it) {
            key += it->first;
            _vk_map[key] = it->second;
        }
    }

    DISALLOW_COPY_AND_ASSIGN(KvRandom);
};
}
