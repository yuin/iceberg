#ifndef __IB_EVENT_H__
#define __IB_EVENT_H__

#include "ib_constants.h"
#include "ib_platform.h"
#include "ib_utils.h"

typedef void (*eventf)(void *); 

namespace ib {
  class CancelableEvent : private NonCopyable<CancelableEvent> { // {{{
    public:
      explicit CancelableEvent(int ms, void (*f)(void*)) : ms_(ms), f_(f), last_event_(0), state_(0), running_(0), thread_(), trigger_cmutex_(), trigger_cond_() {}
      virtual ~CancelableEvent() {}

      void startThread();
      void stopThread();
      void queueEvent(void *ev);
      void cancelEvent();
      void setMs(int ms) { ms_ = ms; }

      int ms_;
      eventf f_;
      void *last_event_;
      int state_;
      int running_;

      ib::thread thread_;

      ib::cmutex trigger_cmutex_;
      ib::condition trigger_cond_;
  }; //.}}}
}

#endif
