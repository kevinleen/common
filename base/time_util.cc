#include "time_util.h"
#include <errno.h>

namespace sys {
namespace xx {
std::string local_time::to_string(int64 sec, const char* format) {
    struct tm tm;
    ::gmtime_r(&sec, &tm);

    std::string s(32, '\0');
    size_t r = ::strftime(&s[0], 32, format, &tm);
    s.resize(r);

    return s;
}
} // namespace xx

void msleep(uint32 ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = ms % 1000 * 1000000;

    while (nanosleep(&ts, &ts) == -1 && errno == EINTR);
}

void usleep(uint32 us) {
    struct timespec ts;
    ts.tv_sec = us / 1000000;
    ts.tv_nsec = us % 1000000 * 1000;

    while (nanosleep(&ts, &ts) == -1 && errno == EINTR);
}
} // namespace sys

struct timeval TimeStamp::toTimeVal() const {
  struct timeval tv = { 0 };
  tv.tv_sec = _ms / kMicroSecsPerSecond;
  tv.tv_usec = _ms % kMicroSecsPerSecond;
  return tv;
}

struct timespec TimeStamp::toTimeSpec() const {
  timespec ts = { 0 };
  ts.tv_sec = _ms / kMicroSecsPerSecond;
  ts.tv_nsec = _ms % kMicroSecsPerSecond * kNanoSecsPerMicroSecond;
  return ts;
}

TimeStamp TimeStamp::now() {
  timeval tv;
  ::gettimeofday(&tv, NULL);
  return TimeStamp(tv.tv_sec * TimeStamp::kMicroSecsPerSecond + tv.tv_usec);
}

TimeStamp TimeStamp::afterMicroSeconds(uint64 micro_secs) {
  return TimeStamp(now().microSecs() + micro_secs);
}

TimeStamp TimeStamp::afterSeconds(uint64 secs) {
  return TimeStamp(now().microSecs() + secs * 1000);
}

const std::string TimeStamp::toTimeRFC1123() const {
  static const char* Days[] =
      { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
  static const char* Months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
  char buff[128];
  time_t t = _ms / TimeStamp::kMicroSecsPerSecond;
  //  time_t t = time(NULL);
  tm* broken_t = gmtime(&t);

  sprintf(buff, "%s, %d %s %d %d:%d:%d GMT", Days[broken_t->tm_wday],
          broken_t->tm_mday, Months[broken_t->tm_mon], broken_t->tm_year + 1900,
          broken_t->tm_hour, broken_t->tm_min, broken_t->tm_sec);
  return std::string(buff);
}
