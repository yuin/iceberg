#include "ib_server.h"
#include "ib_config.h"
#include "ib_utils.h"
#include "ib_platform.h"
#include "ib_controller.h"

void ib::Server::accept(FL_SOCKET fd, void *data) { // {{{
  FL_SOCKET client_socket;
  sockaddr_in client;
  int s = sizeof(client);
  client_socket = ::accept(fd, (sockaddr*)&client, (ib::socklen*)&s);
  if(client_socket < 0){
    return;
  }

  Fl::add_fd(client_socket, FL_READ, ib::Server::respond, 0);
} // }}}

void ib::Server::respond(FL_SOCKET fd, void *data) { // {{{
  char length_buf[4];
  if(ib::utils::read_socket(fd, length_buf, 4) == 4) {
    ib::u32 length = ib::utils::bebytes2u32int(length_buf);
    char *buf = new char[length+1];
    if(ib::utils::read_socket(fd, buf, length) == length){
      buf[length] = '\0';
      ib::Controller::inst().handleIpcMessage(buf);
    }
    delete[] buf;
  }
  Fl::remove_fd(fd, FL_READ);
  ib::platform::close_socket(fd);
} // }}}

int ib::Server::start(ib::Error &error){ // {{{
  const unsigned int port = ib::Config::inst().getServerPort();
  if(port == 0) return 0;
  if(started_) return 0;

  struct sockaddr_in addr;
  char yes = 1;

  socket_ = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_ == INVALID_SOCKET) {
    error.setMessage("Failed to create a server socket.");
    return 1;
  }

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
#ifdef IB_OS_WIN
  addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
#else
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
#endif
  setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(yes));
#ifdef SO_REUSEPORT
  setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, (const char *)&yes, sizeof(yes));
#endif

  if (bind(socket_, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
    error.setMessage("Failed to bind a server socket");
    return 1;
  }

  if(listen(socket_, 1) < 0){
    error.setMessage("Failed to listen a server socket");
    return 1;
  }

  Fl::add_fd(socket_, FL_READ, ib::Server::accept);
  started_ = true;
  return 0;
} // }}}

void ib::Server::shutdown() { // {{{
  if(started_){
    Fl::remove_fd(socket_, FL_READ);
    ib::platform::close_socket(socket_);
    started_ = false;
  }
} // }}}

