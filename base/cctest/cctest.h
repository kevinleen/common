#pragma once

#include "../data_types.h"
#include "../thread_util.h"
#include "../time_util.h"
#include "../ccflag/ccflag.h"
#include "../cclog/cclog.h"
#include <memory>

DEC_uint32(timeout);  // timeout in ms for test cases
DEC_bool(a);          // run all tests if true

namespace cctest {

void init_cctest(int argc, char** argv);

void run_tests();

const std::vector<std::string>& args();

struct Test {
    Test() = default;
    virtual ~Test() = default;

    virtual void run() = 0;

    virtual const std::string& name() = 0;

    virtual bool on() = 0;

    DISALLOW_COPY_AND_ASSIGN(Test);
};

class TestRunner {
  public:
    TestRunner() = default;
    ~TestRunner() = default;

    void run_test(Test* test);

  private:
    void thread_fun() {
        _test->run();
        _ev.signal();
    }

    std::unique_ptr<Test> _test;
    std::unique_ptr<Thread> _thread;
    SyncEvent _ev;

    DISALLOW_COPY_AND_ASSIGN(TestRunner);
};

struct Tests {
    static Tests* instance() {
        static Tests tests;
        return &tests;
    }

    void add_test(Test* test) {
        _tests.push_back(test);
    }

    void run_tests();

    void set_args(const std::vector<std::string>& args) {
        _args = args;
    }

    const std::vector<std::string>& get_args() const {
        return _args;
    }

  private:
    TestRunner _runner;
    std::vector<Test*> _tests;
    std::vector<std::string> _args;

    Tests() = default;
    ~Tests() = default;
    DISALLOW_COPY_AND_ASSIGN(Tests);
};

struct TestSaver {
    TestSaver(Test* test) {
        cctest::Tests::instance()->add_test(test);
    }
};

} // namespace cctest

#define DEF_test(_name_) \
    DEF_bool(_name_, false, "enable " #_name_ " test if true"); \
    \
    struct Test_##_name_ : public ::cctest::Test { \
        Test_##_name_() : _name(#_name_) {} \
        virtual ~Test_##_name_() = default; \
        \
        virtual void run(); \
        \
        virtual bool on() { \
            return FLG_##_name_; \
        } \
        \
        virtual const std::string& name() { \
            return _name; \
        } \
        \
      private: \
        std::string _name; \
    }; \
    \
    static ::cctest::TestSaver SaveTest_##_name_(new Test_##_name_()); \
    \
    void Test_##_name_::run()
