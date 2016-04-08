#include "cctest.h"

DEF_uint32(timeout, -1, "timeout in ms for test cases");
DEF_bool(a, false, "run all tests if true");

namespace cctest {

void init_cctest(int argc, char** argv) {
    auto args = ccflag::init_ccflag(argc, argv);
    cclog::init_cclog(*argv);

    Tests::instance()->set_args(args);
}

void run_tests() {
    Tests::instance()->run_tests();
}

const std::vector<std::string>& args() {
    return Tests::instance()->get_args();
}

void Tests::run_tests() {
    if (FLG_a) {
        for (auto i = 0; i < _tests.size(); ++i) {
            _runner.run_test(_tests[i]);
        }

    } else {
        for (auto i = 0; i < _tests.size(); ++i) {
            if (_tests[i]->on()) {
                _runner.run_test(_tests[i]);
            } else {
                delete _tests[i];
            }
        }
    }
}

void TestRunner::run_test(Test* test) {
    _test.reset(test);
    _ev.reset();

    CERR << "begin " << test->name() << " test >>>";
    _thread.reset(new Thread(std::bind(&TestRunner::thread_fun, this)));
    _thread->start();

    sys::timer t;
    std::pair<std::string, uint32> res;

    if (!_ev.timed_wait(FLG_timeout)) {
        CERR <<"end " << test->name() << " test] "
             << "timeout: " << t.ms() << "ms\n";
        _thread->cancel();

    } else {
        CERR <<"end " << test->name() << " test] "
             << "ok: " << t.ms() << "ms\n";
        _thread->join();
    }
}

} // namespace cctest
