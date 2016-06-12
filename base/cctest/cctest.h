#pragma once

#include <string>
#include <vector>
#include <memory>
#include <iostream>

#include "../data_types.h"
#include "../cclog/cclog.h"
#include "../string_util.h"
#include "../thread_util.h"

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
        std::string current_case() { \
            auto x = 12 - _current_case.size() % 12; \
            if (x < 2) x = 4; \
            return _current_case + std::string(x, ' '); \
        } \
        \
      private: \
        std::string _name; \
        std::string _current_case; \
    }; \
    \
    static ::cctest::TestSaver SaveTest_##_name_(new Test_##_name_()); \
    \
    void Test_##_name_::run()

#define DEF_case(_case_) _current_case = #_case_;

inline void set_green() {
    std::cout << "\033[0;32m"; \
    std::cout.flush();
}

inline void set_red() {
    std::cout << "\033[0;31m";
    std::cout.flush();
}

inline void set_lightblue() {
    std::cout << "\033[1;34m";
    std::cout.flush();
}

inline void reset_color() {
    std::cout << "\033[0m";
    std::cout.flush();
}

#define EXPECT(_cond_) \
    if (_cond_) {\
        set_green(); \
        std::cout << "test " << name() << ": " << current_case() \
            << "EXPECT(" << #_cond_ << ") ok" << std::endl; \
        reset_color(); \
    } else { \
        set_red(); \
        std::cout << "test " << name() << ": " << current_case() \
            << "EXPECT(" << #_cond_ << ") failed" << std::endl; \
        reset_color(); \
    }

#define EXPECT_OP(_x_, _y_, _op_, _op_name_) \
{\
    auto __x = _x_; auto __y = _y_; \
    if (__x _op_ __y) { \
        set_green(); \
        std::cout << "test " << name() << ": " << current_case() \
            << "EXPECT_" << _op_name_ << "(" << #_x_ << ", " << #_y_ \
            << ") ok" << std::endl; \
        reset_color(); \
        \
    } else { \
        set_red(); \
        std::cout << "test " << name() << ": " << current_case() \
            << "EXPECT_" << _op_name_ << "(" << #_x_ << ", " << #_y_ \
            << ") failed: " << __x << " vs " << __y << std::endl; \
        reset_color(); \
    } \
}

#define EXPECT_EQ(_x_, _y_) EXPECT_OP(_x_, _y_, ==, "EQ")
#define EXPECT_GE(_x_, _y_) EXPECT_OP(_x_, _y_, >=, "GE")
#define EXPECT_LE(_x_, _y_) EXPECT_OP(_x_, _y_, <=, "LE")
#define EXPECT_GT(_x_, _y_) EXPECT_OP(_x_, _y_, >, "GT")
#define EXPECT_LT(_x_, _y_) EXPECT_OP(_x_, _y_, <, "LT")
