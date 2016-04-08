#include "thread_util.h"

// return false if timeout
bool SyncEvent::timed_wait(uint32 ms) {
    MutexGuard g(_mutex);

    if (!_signaled) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);

        ts.tv_sec += ms / 1000;
        ts.tv_nsec += ms % 1000 * 1000000;

        if (ts.tv_nsec > 999999999) {
            ts.tv_nsec -= 1000000000;
            ++ts.tv_sec;
        }

        int ret = pthread_cond_timedwait(&_cond, _mutex.mutex(), &ts);
        if (ret == ETIMEDOUT) return false;

        CHECK_EQ(ret, 0);
    }

    if (!_manual_reset) _signaled = false;
    return true;
}
