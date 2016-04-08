#pragma once

#include <boost/shared_ptr.hpp>

template<class T>
struct shared_ptr : public boost::shared_ptr<T> {
    shared_ptr(T* t)
        : boost::shared_ptr<T>(t) {
    }

    shared_ptr()
        : boost::shared_ptr<T>() {
    }

    shared_ptr(const boost::shared_ptr<T>& r) {
      boost::shared_ptr<T>::operator=(r);
    }

    shared_ptr& operator=(const boost::shared_ptr<T>& r) {
      boost::shared_ptr<T>::operator=(r);
      return *this;
    }
};

