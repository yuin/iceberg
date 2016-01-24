#ifndef __IB_SINGLETON_H__
#define __IB_SINGLETON_H__

#include "ib_utils.h"
#include "ib_platform.h"

namespace ib {
  class SingletonFinalizer {
    public:
      typedef void(*FinalizerFunc)();

      static void addFinalizer(FinalizerFunc func);
      static void finalize();
      static int count_;
      static const int MAX_FUNCS_ = 256;
      static FinalizerFunc funcs_[MAX_FUNCS_];
  };

  template <typename T>
  class Singleton final {
    public:
      static T* initInstance() {
        instance_ = new T;
        SingletonFinalizer::addFinalizer(&ib::Singleton<T>::destroy);
        return instance_;
      }

      static T* getInstance() {
        assert(instance_);
        return instance_;
      }

    protected:
      static void destroy() {
        delete instance_;
        instance_ = nullptr;
      }
      static T* instance_;
  };

}

template <typename T> T* ib::Singleton<T>::instance_ = nullptr;

#endif
