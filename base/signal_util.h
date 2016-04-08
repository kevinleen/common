#pragma once

#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <map>
#include <deque>
#include <functional>

/*
 * signal_handler:
 *
 *   for setting multiple handlers to one signal
 *
 *   sys::signal.add_handler(SIGINT, std::function<void()> a);
 *   sys::signal.add_handler(SIGINT, std::function<void()> b);
 *
 * SIGSEGV, 11, segmentation fault
 * SIGABRT, 6,  abort()
 * SIGFPE,  8,  devided by zero
 * SIGTERM, 15, terminate, default signal by kill
 * SIGINT,  2,  Ctrl + C
 * SIGQUIT, 3,  Ctrl + \
 */

namespace sys {
namespace xx {
typedef void (*handler_t)(int);
typedef void (*action_t)(int, siginfo_t*, void*);

struct signal {
    static inline void init_sigaction(struct ::sigaction& sa, handler_t handler,
                                      int flag = 0) {
        memset(&sa, 0, sizeof(sa));
        sigemptyset(&sa.sa_mask);

        sa.sa_flags = flag;
        sa.sa_handler = handler;
    }

    static inline bool set_handler(int sig, handler_t handler, int flag = 0) {
        struct sigaction sa;
        init_sigaction(sa, handler, flag);
        return sigaction(sig, &sa, NULL) != -1;
    }

    static inline handler_t get_handler(int sig) {
        struct sigaction sa;
        sigaction(sig, NULL, &sa);
        return sa.sa_handler;
    }

    // set handler to SIG_IGN
    static inline bool ignore(int sig) {
        return set_handler(sig, SIG_IGN);
    }

    // reset handler to SIG_DFL
    static inline void reset(int sig) {
        set_handler(sig, SIG_DFL);
    }

    static inline void kill(int sig) {
        ::kill(getpid(), sig);
    }

    static void add_handler(int sig, std::function<void()> cb, int flag = 0);
    static void add_handler(int sig, handler_t handler, int flag = 0);
    static void del_handler(int sig);
};

class signal_handler {
  public:
    typedef std::function<void()> callback_t;
    typedef std::deque<callback_t> cbque;

    static void add(int sig, callback_t cb, int flag = 0) {
        signal_handler::instance()->add_handler(sig, cb);
    }

    static void del(int sig) {
        signal_handler::instance()->del_handler(sig);
    }

  private:
    signal_handler() = default;
    ~signal_handler() = default;

    std::map<int, cbque> _map;

    static signal_handler* instance() {
        static signal_handler sh;
        return &sh;
    }

    static void on_signal(int sig) {
        signal_handler::instance()->handle_signal(sig);
    }

    void add_handler(int sig, callback_t cb, int flag = 0) {
        auto it = _map.find(sig);
        auto& cbs = (it != _map.end()) ? it->second : (_map[sig] = cbque());
        if (it != _map.end()) {
            cbs.push_back(cb);
            return;
        }

        handler_t oh = signal::get_handler(sig);
        if (oh != SIG_DFL && oh != SIG_IGN && oh != SIG_ERR) {
            cbs.push_back(std::bind(oh, sig));
        }

        signal::set_handler(sig, &signal_handler::on_signal, flag);
        cbs.push_back(cb);
    }

    void del_handler(int sig) {
        _map.erase(sig);
        signal::reset(sig);
    }

    void handle_signal(int sig) {
        auto it = _map.find(sig);
        auto& cbs = it->second;

        while (!cbs.empty()) {
            auto cb = cbs.back();
            cbs.pop_back();
            cb();
        }

        signal::reset(sig);
        signal::kill(sig);
    }
};

inline void signal::add_handler(int sig, std::function<void()> cb, int flag) {
    signal_handler::add(sig, cb);
}

inline void signal::add_handler(int sig, handler_t handler, int flag) {
    signal_handler::add(sig, std::bind(handler, sig), flag);
}

inline void signal::del_handler(int sig) {
    signal_handler::del(sig);
}
} // namespace xx

extern xx::signal signal;
} // namespace sys
