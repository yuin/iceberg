#ifndef __IB_SERVER_H__
#define __IB_SERVER_H__

#include "ib_constants.h"
#include "ib_utils.h"
#include "ib_platform.h"
#include "ib_singleton.h"

namespace ib {

  class Server : private NonCopyable<Server>{ // {{{
    friend class ib::Singleton<ib::Server>;
    public:
      ~Server();

      static void accept(FL_SOCKET fd, void *data);
      static void respond(FL_SOCKET fd, void *data);
      static void action(FL_SOCKET fd, void *data);
      int start(ib::Error &error);
      void shutdown();

    protected:
      Server() : socket_(0), started_(false) {}

      FL_SOCKET socket_;
      bool started_;

  }; // }}}

}

#endif 
