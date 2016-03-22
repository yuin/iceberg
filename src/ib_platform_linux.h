#ifndef __IB_PLATFORM_LINUX_H__
#define __IB_PLATFORM_LINUX_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <signal.h>
#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/shape.h>

#define INVALID_SOCKET -1
#define SOCKET_ERROR   -1

namespace ib{
  typedef socklen_t       socklen;

  typedef pthread_t       thread;
  typedef pthread_mutex_t mutex;
  typedef pthread_mutex_t cmutex;
  typedef pthread_cond_t  condition;
  typedef void*           threadret;
  typedef void* (*threadfunc)(void*);

  typedef Window          whandle;
  typedef void*           module;

  namespace platform {
    void move_to_current_desktop(Fl_Window *w);
  }
}

#endif
