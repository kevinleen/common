#pragma once

#include "data_types.h"
#include "cclog/cclog.h"
#include <time.h>
#include <sys/time.h>
#include <string>

namespace sys {
namespace xx {
/*
 * univeral time(UTC)
 *
 *   sec():  seconds since EPOCH
 *    ms():  milliseconds since EPOCH
 *    us():  microseconds since EPOCH
 */
struct utc {
    static inline int64 us() {
      struct timeval now;
      ::gettimeofday(&now, NULL);
      return static_cast<int64>(now.tv_sec) * 1000000 + now.tv_usec;
    }

    static inline int64 ms() {
      return utc::us() / 1000;
    }

    static inline int64 sec() {
      return utc::us() / 1000000;
    }

    static inline int64 hour() {
        return utc::sec() / 3600;
    }

    static inline int64 day() {
        return utc::sec() / 86400;
    }

    static inline bool is_today(int64 sec) {
        return utc::day() == sec / 86400;
    }
};

/*
 * local time
 */
struct local_time {
    static inline int64 us() {
      struct timeval now;
      struct timezone tz;
      ::gettimeofday(&now, &tz);

      return now.tv_usec
          + (static_cast<int64>(now.tv_sec) - tz.tz_minuteswest * 60) * 1000000;
    }

    static inline int64 ms() {
      return local_time::us() / 1000;
    }

    static inline int64 sec() {
      return local_time::us() / 1000000;
    }

    static inline int64 hour() {
        return local_time::sec() / 3600;
    }

    static inline int64 day() {
        return local_time::sec() / 86400;
    }

    static inline bool is_today(int64 sec) {
        return local_time::day() == sec / 86400;
    }

    static std::string to_string(int64 sec, const char* format =
                                     "%Y-%m-%d %H:%M:%S");

    static inline std::string to_string() {
      return local_time::to_string(local_time::sec());
    }

    /*
     * to_string("%Y-%m-%d %H:%M:%S") ==> 2015-12-25 01:23:45
     */
    static inline std::string to_string(const char* format) {
      return local_time::to_string(local_time::sec(), format);
    }
};
}  // namespace xx

extern xx::utc utc;
extern xx::local_time local_time;

/*
 * sleep for @ms milliseconds
 */
void msleep(uint32 ms);

/*
 * sleep for @us microseconds
 */
void usleep(uint32 us);

/*
 * sleep for @sec seconds
 */
inline void sleep(uint32 sec) {
  sys::msleep(sec * 1000);
}

class timer {
  public:
    timer() {
      _start = sys::utc.us();
    }
    ~timer() = default;

    void restart() {
      _start = sys::utc.us();
    }

    int64 us() const {
      return sys::utc.us() - _start;
    }

    int64 ms() const {
      return this->us() / 1000;
    }

    int64 sec() const {
      return this->us() / 1000000;
    }

  private:
    int64 _start;

    DISALLOW_COPY_AND_ASSIGN(timer);
};

class auto_timer {
  public:
    auto_timer(const std::string& msg = std::string(), uint32 ms = 50)
        : _msg(msg), _ms(ms) {
    }

    ~auto_timer() {
      auto ms = _timer.ms();
      TLOG_IF("auto_timer", ms > _ms) << _msg << " timedout: " << _ms;
    }

  private:
    sys::timer _timer;
    std::string _msg;
    uint32 _ms;
};

#define AUTO_TIMER(_name_, _msg_, _ms_) \
    sys::auto_timer _name_( \
        std::string(__FILE__) + ":" + util::to_string(__LINE__) + "> " + _msg_, _ms_)

}  // namespace sys

#define kSecondsPerDay (86400)

class TimeStamp {
  public:
    explicit TimeStamp(uint64 micro_secs = 0)
        : _ms(micro_secs) {
    }
    TimeStamp(const TimeStamp& rhs)
        : _ms(rhs._ms) {
    }

    const static uint64 kMilliSecsPerSecond = 1000ULL;
    const static uint64 kMicroSecsPerMilliSecond = 1000ULL;
    const static uint64 kMicroSecsPerSecond = kMicroSecsPerMilliSecond
        * kMilliSecsPerSecond;
    const static uint64 kNanoSecsPerMicroSecond = 1000ULL;
    const static uint64 kNanoSecsPerSecond = kMicroSecsPerSecond
        * kNanoSecsPerMicroSecond;

    static TimeStamp now();
    static TimeStamp afterSeconds(uint64 secs);
    static TimeStamp afterMicroSeconds(uint64 micro_secs);

    uint64 microSecs() const {
      return _ms;
    }
    struct timeval toTimeVal() const;
    struct timespec toTimeSpec() const;
    const std::string toTimeRFC1123() const;

    bool operator <(const TimeStamp& t) const {
      return _ms < t._ms;
    }
    bool operator >(const TimeStamp& t) const {
      return !operator <(t);
    }

    TimeStamp& operator +(uint64 micro_sec) {
      _ms += micro_sec;
      return *this;
    }
    TimeStamp& operator +(const TimeStamp& rhs) {
      _ms += rhs._ms;
      return *this;
    }
    TimeStamp& operator -(const TimeStamp& rhs) {
      _ms -= rhs._ms;
      return *this;
    }
    TimeStamp& operator -(uint64 micro_sec) {
      _ms -= micro_sec;
      return *this;
    }

    TimeStamp& operator +=(uint64 micro_sec) {
      _ms += micro_sec;
      return *this;
    }
    TimeStamp& operator -=(uint64 micro_sec) {
      _ms -= micro_sec;
      return *this;
    }
    TimeStamp& operator +=(const TimeStamp& rhs) {
      _ms += rhs._ms;
      return *this;
    }
    TimeStamp& operator -=(const TimeStamp& rhs) {
      _ms -= rhs._ms;
      return *this;
    }
    const TimeStamp& operator =(const TimeStamp& rhs) {
      _ms = rhs._ms;
      return *this;
    }

  private:
    uint64 _ms;
};
