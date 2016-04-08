#pragma once

#include "include/thread_safe.h"
#include "./zkclient/smart_finder/smart_finder.h"

namespace util {

class ServerFinder {
  public:
    typedef std::pair<std::string, uint32> IpPort;
    typedef std::function<void(const std::string&, uint32)> Callback;

    explicit ServerFinder(SmartFinder* finder)
        : _finder(finder) {
        CHECK_NOTNULL(finder);
    }
    ~ServerFinder();

    bool init();

    IpPort next_server();

    void add_down_callback(const std::string& ip, uint32 port,
                           Callback cb) {
        _cbs[this->hash(ip, port)] = cb;
    }

  private:
    void on_server_up(const std::string& ip, uint32 port);
    void on_server_down(const std::string& ip, uint32 port);

    void on_last_server_down(const std::string& ip, uint32 port);
    void clear_last_server();

    void add_server(const std::string& ip, uint32 port);
    void del_server(const std::string& ip, uint32 port);

    void update_server_list();

    uint32 hash(const std::string& ip, uint32 port) {
        return ::SuperFastHash(ip + util::to_string(port));
    }

    uint32 hash(const IpPort& ip_port) {
        return this->hash(ip_port.first, ip_port.second);
    }

    void run_down_callback(const std::string& ip, uint32 port) {
        auto it = _cbs.find(this->hash(ip, port));
        if (it != _cbs.end()) {
            it->second(ip, port);
        }
    }

    std::string server_list();

  private:
    std::unique_ptr<SmartFinder> _finder;
    std::map<uint32, Callback> _cbs;

    RwLock _rw_lock;
    std::vector<IpPort> _server_list;
    std::unique_ptr<IpPort> _last_server;

    std::unique_ptr<StoppableThread> _update_thread;

    DISALLOW_COPY_AND_ASSIGN(ServerFinder);
};

template <typename T>
struct ThreadSafeClient : public ThreadSafeHandlerWrapper<T> {
    explicit ThreadSafeClient(typename ThreadSafeClient<T>::Delegate* delegate)
        : ThreadSafeHandlerWrapper<T>(delegate) {
    }

    virtual ~ThreadSafeClient() = default;
};

template <typename T>
class ThreadSafeDelegate : public ThreadSafeHandlerWrapper<T>::Delegate {
  public:
    ThreadSafeDelegate(std::shared_ptr<ServerFinder> finder,
                       ThreadSafeClient<T>* client)
        : _finder(finder), _client(client) {
        CHECK_NOTNULL(finder);
        CHECK_NOTNULL(client);
    }

    virtual ~ThreadSafeDelegate() = default;

  protected:
    std::pair<std::string, uint32> next_server ();

    bool find_thread(uint32 key);
    bool add_thread(uint32 key, uint64 thread_id);

    void on_server_down(const std::string& ip, uint32 port);

  private:
    std::shared_ptr<ServerFinder> _finder;
    ThreadSafeClient<T>* _client;

    RwLock _rw_lock;
    std::map<uint32, std::vector<uint64>> _threads;

    DISALLOW_COPY_AND_ASSIGN(ThreadSafeDelegate);
};

template<typename T>
inline bool ThreadSafeDelegate<T>::find_thread(uint32 key) {
    ReadLockGuard g(_rw_lock);
    return _threads.find(key) != _threads.end();
}

template<typename T>
inline bool ThreadSafeDelegate<T>::add_thread(uint32 key,
                                              uint64 thread_id) {
    WriteLockGuard g(_rw_lock);
    _threads[key].push_back(thread_id);
}

template<typename T>
void ThreadSafeDelegate<T>::on_server_down(const std::string& ip,
                                           uint32 port) {
    uint32 key = ::SuperFastHash(ip + util::to_string(port));
    {
        WriteLockGuard g(_rw_lock);
        auto it = _threads.find(key);
        if (it == _threads.end()) return;

        const auto& t = it->second;
        for (uint32 i = 0; i < t.size(); ++i) {
            _client->erase(t[i]);
        }

        _threads.erase(it);
    }
}

template<typename T>
std::pair<std::string, uint32> ThreadSafeDelegate<T>::next_server () {
    auto server = _finder->next_server();
    const auto& ip = server.first;
    const auto& port = server.second;
    CHECK(!ip.empty()) << "no cassandra server found...";

    uint32 key = ::SuperFastHash(ip + util::to_string(port));
    auto thread_id = pthread_self();

    if (!this->find_thread(key)) {
        _finder->add_down_callback(
            ip, port, std::bind(&ThreadSafeDelegate<T>::on_server_down,
                this, std::_1, std::_2));
    }

    this->add_thread(key, thread_id);

    return server;
}

} // namespace util
