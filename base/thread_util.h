#pragma once


#include "data_types.h"
#include "cclog/cclog.h"

#include <time.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <functional>

class Mutex {
  public:
    Mutex() {
        CHECK(pthread_mutex_init(&_mutex, NULL) == 0);
    }
    ~Mutex() {
        CHECK(pthread_mutex_destroy(&_mutex) == 0);
    }

    void lock() {
        int err = pthread_mutex_lock(&_mutex);
        CHECK_EQ(err, 0)<< ::strerror(err);
    }

    void unlock() {
        int err = pthread_mutex_unlock(&_mutex);
        CHECK_EQ(err, 0) << ::strerror(err);
    }

    bool try_lock() {
        return pthread_mutex_trylock(&_mutex) == 0;
    }

    pthread_mutex_t* mutex() {
        return &_mutex;
    }

private:
    pthread_mutex_t _mutex;

    DISALLOW_COPY_AND_ASSIGN(Mutex);
};

class RwLock {
  public:
    RwLock() {
        CHECK(pthread_rwlock_init(&_lock, NULL) == 0);
    }
    ~RwLock() {
        CHECK(pthread_rwlock_destroy(&_lock) == 0);
    }

    void read_lock() {
        int err = pthread_rwlock_rdlock(&_lock);
        CHECK_EQ(err, 0)<< ::strerror(err);
    }

    void write_lock() {
        int err = pthread_rwlock_wrlock(&_lock);
        CHECK_EQ(err, 0) << ::strerror(err);
    }

    void unlock() {
        int err = pthread_rwlock_unlock(&_lock);
        CHECK_EQ(err, 0) << ::strerror(err);
    }

    bool try_read_lock() {
        return pthread_rwlock_tryrdlock(&_lock) == 0;
    }

    bool try_write_lock() {
        return pthread_rwlock_trywrlock(&_lock) == 0;
    }

private:
    pthread_rwlock_t _lock;

    DISALLOW_COPY_AND_ASSIGN(RwLock);
};

class MutexGuard {
  public:
    explicit MutexGuard(Mutex& mutex)
        : _mutex(mutex) {
        _mutex.lock();
    }

    ~MutexGuard() {
        _mutex.unlock();
    }

  private:
    Mutex& _mutex;

    DISALLOW_COPY_AND_ASSIGN(MutexGuard);
};

class ReadLockGuard {
  public:
    explicit ReadLockGuard(RwLock& lock)
        : _lock(lock) {
        _lock.read_lock();
    }

    ~ReadLockGuard() {
        _lock.unlock();
    }

  private:
    RwLock& _lock;

    DISALLOW_COPY_AND_ASSIGN(ReadLockGuard);
};

class WriteLockGuard {
  public:
    explicit WriteLockGuard(RwLock& lock)
        : _lock(lock) {
        _lock.write_lock();
    }

    ~WriteLockGuard() {
        _lock.unlock();
    }

  private:
    RwLock& _lock;

    DISALLOW_COPY_AND_ASSIGN(WriteLockGuard);
};

#define atomic_swap __sync_lock_test_and_set
#define atomic_release __sync_lock_release
#define atomic_compare_swap __sync_val_compare_and_swap

class SpinLock {
  public:
    SpinLock() : _locked(false) {
    }

    ~SpinLock() = default;

    bool try_lock() {
        return atomic_swap(&_locked, true) == false;
    }

    void lock() {
        while (!this->try_lock());
    }

    void unlock() {
        atomic_release(&_locked);
    }

  private:
    bool _locked;

    DISALLOW_COPY_AND_ASSIGN(SpinLock);
};

class SpinLockGuard {
  public:
    explicit SpinLockGuard(SpinLock& lock)
        : _lock(lock) {
        _lock.lock();
    }

    ~SpinLockGuard() {
        _lock.unlock();
    }

  private:
    SpinLock& _lock;

    DISALLOW_COPY_AND_ASSIGN(SpinLockGuard);
};

// for single producer-consumer
class SyncEvent {
  public:
    explicit SyncEvent(bool manual_reset = false, bool signaled = false)
        : _manual_reset(manual_reset), _signaled(signaled) {
        pthread_condattr_t attr;
        CHECK(pthread_condattr_init(&attr) == 0);
        CHECK(pthread_condattr_setclock(&attr, CLOCK_MONOTONIC) == 0);
        CHECK(pthread_cond_init(&_cond, &attr) == 0);
        CHECK(pthread_condattr_destroy(&attr) == 0);
    }

    ~SyncEvent() {
        CHECK(pthread_cond_destroy(&_cond) == 0);
    }

    void signal() {
        MutexGuard m(_mutex);
        if (!_signaled) {
            _signaled = true;
            pthread_cond_broadcast(&_cond);
        }
    }

    void reset() {
        MutexGuard m(_mutex);
        _signaled = false;
    }

    bool is_signaled() {
        MutexGuard m(_mutex);
        return _signaled;
    }

    void wait() {
        MutexGuard m(_mutex);
        if (!_signaled) {
            pthread_cond_wait(&_cond, _mutex.mutex());
            CHECK(_signaled);
        }
        if (!_manual_reset) _signaled = false;
    }

    // return false if timeout
    bool timed_wait(uint32 ms);

  private:
    pthread_cond_t _cond;
    Mutex _mutex;

    const bool _manual_reset;
    bool _signaled;
};

class Thread {
  public:
    explicit Thread(std::function<void()> fun)
        : _fun(fun), _id(0) {
    }

    ~Thread() = default;

    bool start() {
        if (atomic_compare_swap(&_id, 0, 1) != 0) return true;
        return pthread_create(&_id, NULL, &Thread::thread_fun, &_fun) == 0;
    }

    void join() {
        auto id = atomic_swap(&_id, 0);
        if (id != 0) pthread_join(id, NULL);
    }

    void detach() {
        auto id = atomic_swap(&_id, 0);
        if (id != 0) pthread_detach(id);
    }

    void cancel() {
        auto id = atomic_swap(&_id, 0);
        if (id != 0) {
            pthread_cancel(id);
            pthread_join(id, NULL);
        }
    }

    uint64 id() const {
        return _id;
    }

  private:
    std::function<void ()> _fun;
    pthread_t _id;

    DISALLOW_COPY_AND_ASSIGN(Thread);

    static void* thread_fun(void* p) {
        auto f = *((std::function<void()>*) p);
        f();
        return NULL;
    }
};

class StoppableThread : public Thread {
  public:
    /*
     * run thread_fun every @ms milliseconds
     */
    StoppableThread(std::function<void()> f, uint32 ms)
        : Thread(std::bind(&StoppableThread::thread_fun, this)),
          _f(f), _ms(ms), _stop(false) {
    }

    ~StoppableThread() = default;

    void join() {
        if (atomic_swap(&_stop, true) == false) {
            _ev.signal();
            Thread::join();
        }
    }

    void notify() {
        _ev.signal();
    }

  private:
    SyncEvent _ev;
    std::function<void ()> _f;

    uint32 _ms;
    bool _stop;

    void thread_fun() {
        while (!_stop) {
            _ev.timed_wait(_ms);
            if (!_stop) _f();
        }
    }
};

template<typename ObjectType>
class ThreadStorage {
  public:
    ThreadStorage() {
        ::pthread_key_create(&_key, NULL);
        set(NULL);
    }
    ~ThreadStorage() {
        ::pthread_key_delete(_key);
    }

    ObjectType* get() const {
        return static_cast<ObjectType*>(::pthread_getspecific(_key));
    }
    void set(ObjectType* obj) {
        ::pthread_setspecific(_key, obj);
    }

  private:
    pthread_key_t _key;

    DISALLOW_COPY_AND_ASSIGN(ThreadStorage);
};
