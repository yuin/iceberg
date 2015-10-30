#include "ib_event.h"

// class CancelableEvent {{{
static ib::threadret thread_func(void *self_) { // {{{
  ib::CancelableEvent *self = (ib::CancelableEvent*)self_;
  while(1) {
    ib::platform::lock_cmutex(&(self->trigger_cmutex_));
    if(self->last_event_ == 0) {
      ib::platform::wait_condition(&(self->trigger_cond_), &(self->trigger_cmutex_), 0);
    }
    if(!self->running_) {
      ib::platform::unlock_cmutex(&(self->trigger_cmutex_));
      goto exit_thread;
    }
    {
      int timeout = ib::platform::wait_condition(&(self->trigger_cond_), &(self->trigger_cmutex_), self->ms_);
      if(!self->running_) {
        ib::platform::unlock_cmutex(&(self->trigger_cmutex_));
        goto exit_thread;
      }
      if(timeout && self->last_event_ != 0) {
        self->f_(self->last_event_);
        self->last_event_ = 0;
      }
    }
    ib::platform::unlock_cmutex(&(self->trigger_cmutex_));
  }
exit_thread:
  ib::platform::exit_thread(0);
  return (ib::threadret)0;
} // }}}

void ib::CancelableEvent::startThread(){ // {{{
  if(running_ == 0) {
    ib::platform::create_cmutex(&trigger_cmutex_);
    ib::platform::create_condition(&trigger_cond_);
    running_ = 1;
    ib::platform::create_thread(&thread_, &thread_func, this);
  }
} // }}}

void ib::CancelableEvent::stopThread(){ // {{{
  bool isrunning = false;
  {
    ib::platform::lock_cmutex(&trigger_cmutex_);
    if(running_ != 0) isrunning=true;
    running_ = 0;
    ib::platform::unlock_cmutex(&trigger_cmutex_);
  }
  if(!isrunning){
    ib::platform::notify_condition(&trigger_cond_);
    ib::platform::join_thread(&thread_);
    ib::platform::destroy_cmutex(&trigger_cmutex_);
    ib::platform::destroy_condition(&trigger_cond_);
  }
} // }}}

void ib::CancelableEvent::queueEvent(void *ev){ // {{{
    ib::platform::lock_cmutex(&trigger_cmutex_);
    ib::platform::notify_condition(&trigger_cond_);
    last_event_ = ev;
    ib::platform::unlock_cmutex(&trigger_cmutex_);
} // }}}

void ib::CancelableEvent::cancelEvent(){ // {{{
    ib::platform::lock_cmutex(&trigger_cmutex_);
    last_event_ = 0;
    ib::platform::unlock_cmutex(&trigger_cmutex_);
} // }}}

// }}}
