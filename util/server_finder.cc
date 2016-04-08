#include "server_finder.h"

namespace util {

ServerFinder::~ServerFinder() {
    if (_update_thread != NULL) {
        _update_thread->join();
        _update_thread.reset();
    }
}

bool ServerFinder::init() {
    auto down_cb = std::bind(&ServerFinder::on_server_down,
                             this, std::_1, std::_2);
    CHECK(_finder->init());
    _finder->setDownClosure(down_cb);

    auto up_cb = std::bind(&ServerFinder::on_server_up,
                           this, std::_1, std::_2);
    _finder->setUpClosure(up_cb);

    _update_thread.reset(
        new ::StoppableThread(
            std::bind(&ServerFinder::update_server_list, this), 5 * 60 * 1000));
    CHECK(_update_thread->start());

    _server_list.clear();

    for (auto i = 0; i < 100; ++i) {
        if (_finder->query(&_server_list)) break;

        if (i < 99) {
            ELOG << "query server list failed, will try again 1 second later...";
            sys::sleep(1);
        } else {
            CHECK(false) << "I have tried to query server list for 99 times,"
                << " now I have to die, goodbye!";
        }
    }

    LOG << "server finder init success, server num: " << _server_list.size()
        << ", servers: " << this->server_list();
    return true;
}

ServerFinder::IpPort ServerFinder::next_server() {
    ReadLockGuard g(_rw_lock);

    if (!_server_list.empty()) {
        auto index = sys::local_time.us() % _server_list.size();
        return _server_list[index];
    }

    CHECK(_last_server != NULL);
    return *_last_server;
}

void ServerFinder::on_server_up(const std::string& ip, uint32 port) {
    WriteLockGuard g(_rw_lock);
    TLOG("up") << "server up, ip: " << ip << ", port: " << port;

    if (_server_list.empty()) {
        this->clear_last_server();
    }

    this->add_server(ip, port);
}

void ServerFinder::on_server_down(const std::string& ip, uint32 port) {
    WriteLockGuard g(_rw_lock);
    TLOG("down") << "server down, ip: " << ip << ", port: " << port;

    this->del_server(ip, port);

    if (_server_list.empty()) {
        this->on_last_server_down(ip, port);
        return;
    }

    this->run_down_callback(ip, port);
}

void ServerFinder::on_last_server_down(const std::string& ip, uint32 port) {
    if (_last_server == NULL) {
        ELOG << "all server down...";
        _last_server.reset(new IpPort(ip, port));
        return;
    }

    const auto& serv = *_last_server;
    if (serv.first == ip && serv.second == port) return;

    ELOG << "last server down, ip: " << ip << ", port: " << port
         << ", but _last_server != NULL...";

    this->run_down_callback(ip, port);
}

void ServerFinder::add_server(const std::string& ip, uint32 port) {
    for (auto it = _server_list.begin(); it != _server_list.end(); ++it) {
        const auto& server = *it;
        if (server.first == ip && server.second == port) {
            return;
        }
    }

    _server_list.push_back(std::make_pair(ip, port));
}

void ServerFinder::del_server(const std::string& ip, uint32 port) {
    if (_server_list.size() < 2) return;

    for (auto it = _server_list.begin(); it != _server_list.end(); ++it) {
        const auto& server = *it;
        if (server.first == ip && server.second == port) {
            _server_list.erase(it);
            break;
        }
    }
}

void ServerFinder::update_server_list() {
    std::vector<IpPort> server_list;
    if (!_finder->query(&server_list)) {
        ELOG << "query server list from smart_finder failed..";
        return;
    }

    WriteLockGuard g(_rw_lock);
    _server_list.swap(server_list);

    DLOG("server_list") << "server_list: " << this->server_list();

    if (_last_server != NULL && !_server_list.empty()) {
        this->clear_last_server();
    }
}

void ServerFinder::clear_last_server() {
    if (_last_server == NULL) return;

    const auto& x = *_last_server;
    this->run_down_callback(x.first, x.second);
    _last_server.reset();
}

std::string ServerFinder::server_list() {
    if (_server_list.empty()) return std::string();

    ::StreamBuf sb;
    sb << '|';

    for (auto i = 0; i < _server_list.size(); ++i) {
        const auto& x = _server_list[i];
        sb << x.first << ":" << x.second << '|';
    }

    return sb.to_string();
}

} // namespace util
