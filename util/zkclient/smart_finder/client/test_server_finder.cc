#include "test_server_finder.h"

DEC_string(serv_name);
DEC_string(serv_path);

namespace test {

void on_server_down(const std::string& ip, uint32 port) {
    ELOG << "on server down, ip: " << ip << ", port: " << port;
}

void TestServerFinder::start() {
    _serv_finder.reset(
        new util::ServerFinder(
            util::CreateSmartFinder(FLG_serv_name, FLG_serv_path)));

    CHECK(_serv_finder->init());

    auto cb = std::bind(&on_server_down, std::_1, std::_2);

    for (auto i = 0; i < 3; ++i) {
        auto x = _serv_finder->next_server();
        _serv_finder->add_down_callback(x.first, x.second, cb);
    }

    while (true) {
        sys::sleep(1);
    }
}

}
