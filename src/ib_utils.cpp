#include "ib_utils.h"
#include "ib_ui.h"
#include "ib_lua.h"
#include "ib_platform.h"
#include "ib_lexer.h"
#include "ib_icon_manager.h"
#include "ib_config.h"
#include "ib_regex.h"
#include "ib_controller.h"
#include "ib_history.h"
#include "ib_migemo.h"
#include "ib_server.h"

// DEBUG {{{
#ifdef DEBUG 
#undef malloc
#undef free
static long __malloc_count = 0;
void *__my_malloc(size_t size) {
  //Fl::lock();
  void *ptr;
  ptr = malloc(size);
  if(ptr)
    __malloc_count++;
  //Fl::unlock();
  return ptr;
}
void __my_free(void *ptr) {
  //Fl::lock();
  free(ptr);
  __malloc_count--;
  //Fl::unlock();
} 
void *operator new(size_t size) { return __my_malloc(size); }
void operator delete(void *p) { __my_free(p); }
#define malloc(size) __my_malloc(size)
#define free(ptr) __my_free(ptr)
#endif
// }}}

long ib::utils::malloc_count() { // {{{
#ifdef DEBUG 
  return __malloc_count;
#else
  return 0;
#endif
} // }}}

void ib::utils::exit_application(const int code) { // {{{
  if(code == 0) { 
    ib::History::inst().dump();
    if(ib::Config::inst().getEnableIcons()){
      ib::IconManager::inst()->dump();
    }
  }

  if(ib::ListWindow::inst() != 0){
    ib::ListWindow::inst()->hide();
    delete ib::ListWindow::inst();
  }
  if(ib::MainWindow::inst() != 0){
    auto input = ib::MainWindow::inst()->getInput();
    if(input != 0 && ib::Config::inst().getKeyEventThreshold() > 0){
      input->stopKeyEventThread();
    }
    ib::MainWindow::inst()->close();
    delete ib::MainWindow::inst();
  }
  if(ib::IconManager::inst() != 0){
    ib::IconManager::inst()->deleteCachedIcons();
    ib::IconManager::inst()->stopLoaderThread();
    delete ib::IconManager::inst();
  }
  ib::Migemo::inst().destroy();
  ib::Server::inst().shutdown();

  ib::platform::finalize_system();
  exit(code);
} // }}}

void ib::utils::reboot_application() { // {{{
  std::vector<ib::unique_string_ptr> params;
  params.push_back(ib::unique_string_ptr(new std::string("-p")));
  char options[32];
  snprintf(options, 32, "%d", ib::platform::get_pid());
  params.push_back(ib::unique_string_ptr(new std::string(options)));

  auto &cfg = ib::Config::inst();
  ib::Error error;
  if(ib::platform::shell_execute(cfg.getSelfPath(), params, cfg.getInitialWorkdir(), error) != 0) {
    fl_alert(error.getMessage().c_str());
  }else{
    ib::utils::exit_application(0);
  }
} // }}}

// ib::utils::scan_search_path() { // {{{
static void scan_search_path_awaker1(void *p){ 
  const char *category = (const char*)p;
  std::string message = "Scanning search paths";
  message +="(category: ";
  message += category;
  message += ")...";
  ib::Controller::inst().showApplication();
  ib::MainWindow::inst()->getInput()->value(message.c_str());
  ib::MainWindow::inst()->getInput()->adjustSize();
  ib::MainWindow::inst()->getInput()->readonly(1);
}
static void scan_search_path_awaker2(void *p){ ib::utils::reboot_application(); }
static void scan_search_path_thread(void *p){
  const char *category = (const char*)p;
  ib::platform::on_thread_start();
  sleep(1);
  Fl::awake(scan_search_path_awaker1, p);
  ib::Controller::inst().cacheCommandsInSearchPath(category);
  Fl::awake(scan_search_path_awaker2, 0);
  ib::platform::exit_thread(0);
}

void ib::utils::scan_search_path(const char *category) {
  auto &cfg = ib::Config::inst();
  ib::oschar oscache_path[IB_MAX_PATH];
  ib::platform::utf82oschar_b(oscache_path, IB_MAX_PATH, cfg.getCommandCachePath().c_str());
  if(ib::platform::file_exists(oscache_path)){
    ib::Error error;
    if(ib::platform::remove_file(oscache_path, error) != 0){
      fl_alert(error.getMessage().c_str());
      return;
    }
  }
  ib::thread thread;
  ib::platform::create_thread(&thread, scan_search_path_thread, (void*)category);
} // }}}

void ib::utils::alert_lua_stack(lua_State *L) { // {{{
  int num, i, type;
  if(L == 0) {
    L = IB_LUA;
  }
  
  num = lua_gettop(L);
  if (num == 0) {
    fl_alert("No stack.");
    return;
  }

  std::string str;
  char tmpstr[128];

  for(i = num; i >= 1; i--) {
    snprintf(tmpstr, 127, "%03d(%04d): ", i, -num + i - 1);
    str += tmpstr;

    type = lua_type(L, i);
    switch(type) {
    case LUA_TNIL:
      str += "NIL\n";
      break;
    case LUA_TBOOLEAN:
      str += "BOOLEAN ";
      str += (lua_toboolean(L, i) ? "true\n" : "false\n");
      break;
    case LUA_TLIGHTUSERDATA:
      str += "LIGHTUSERDATA\n";
      break;
    case LUA_TNUMBER:
      snprintf(tmpstr, 127, "NUMBER %lf\n", lua_tonumber(L, i));
      str += tmpstr;
      break;
    case LUA_TSTRING:
      str += "STRING ";
      str += lua_tostring(L, i);
      str += "\n";
      break;
    case LUA_TTABLE:
      str += "TABLE\n";
      break;
    case LUA_TFUNCTION:
      str += "FUNCTION\n";
      break;
    case LUA_TUSERDATA:
      str += "USERDATA\n";
      break;
    case LUA_TTHREAD:
      str += "TTHREAD\n";
      break;
    }
  }
  fl_alert(str.c_str());
} // }}}

void ib::utils::expand_vars(std::string &ret, const std::string &tpl, const ib::string_map &values) { // {{{
  ib::SimpleTemplateLexer lexer;
  lexer.parse(ret, tpl, values);
} // }}}

bool ib::utils::event_key_is_control_key() { // {{{
  int key = Fl::event_key();
  if(key == (FL_Button)) return true;
  if(key == (FL_Tab)) return true;
  //if(key == (FL_Enter)) return true;
  if(key == (FL_Pause)) return true;
  if(key == (FL_Scroll_Lock)) return true;
  if(key == (FL_Escape)) return true;
  if(key == (FL_Home)) return true;
  if(key == (FL_Left)) return true;
  if(key == (FL_Up)) return true;
  if(key == (FL_Right)) return true;
  if(key == (FL_Down)) return true;
  if(key == (FL_Page_Up)) return true;
  if(key == (FL_Page_Down)) return true;
  if(key == (FL_End)) return true;
  if(key == (FL_Print)) return true;
  if(key == (FL_Insert)) return true;
  if(key == (FL_Menu)) return true;
  if(key == (FL_Help)) return true;
  if(key == (FL_Num_Lock)) return true;
  if(key == (FL_KP)) return true;
  //if(key == (FL_KP_Enter)) return true;
  if(key == (FL_KP_Last)) return true;
  if(key == (FL_F+1)) return true;
  if(key == (FL_F+2)) return true;
  if(key == (FL_F+3)) return true;
  if(key == (FL_F+4)) return true;
  if(key == (FL_F+5)) return true;
  if(key == (FL_F+6)) return true;
  if(key == (FL_F+7)) return true;
  if(key == (FL_F+8)) return true;
  if(key == (FL_F+9)) return true;
  if(key == (FL_F+10)) return true;
  if(key == (FL_F+11)) return true;
  if(key == (FL_F+12)) return true;
  if(key == (FL_F_Last)) return true;
  //if(key == (FL_Shift_L)) return true;
  //if(key == (FL_Shift_R)) return true;
  if(key == (FL_Control_L)) return true;
  if(key == (FL_Control_R)) return true;
  if(key == (FL_Caps_Lock)) return true;
  if(key == (FL_Meta_L)) return true;
  if(key == (FL_Meta_R)) return true;
  if(key == (FL_Alt_L)) return true;
  if(key == (FL_Alt_R)) return true;
  if(key == (FL_Volume_Down)) return true;
  if(key == (FL_Volume_Mute)) return true;
  if(key == (FL_Volume_Up)) return true;
  if(key == (FL_Media_Play)) return true;
  if(key == (FL_Media_Stop)) return true;
  if(key == (FL_Media_Prev)) return true;
  if(key == (FL_Media_Next)) return true;
  if(key == (FL_Home_Page)) return true;
  if(key == (FL_Mail)) return true;
  if(key == (FL_Search)) return true;
  if(key == (FL_Back)) return true;
  if(key == (FL_Forward)) return true;
  if(key == (FL_Stop)) return true;
  if(key == (FL_Refresh)) return true;
  if(key == (FL_Sleep)) return true;
  if(key == (FL_Favorites)) return true;
  return false;
} // }}}

void ib::utils::parse_key_bind(int *result, const char *string) { // {{{
  int start = 0, ptr=0, ret_ptr=0;
  for(; string[ptr] != '\0' && ret_ptr != 3; ptr++){
    if(string[ptr] == '-'){
      if(memcmp(string+start, "shift", ptr - start) == 0){
        result[ret_ptr++] = 0x00010000;
        start = ptr+1;
      }else if(memcmp(string+start, "caps_lock", ptr - start) == 0){
        result[ret_ptr++] = 0x0002000;
        start = ptr+1;
      }else if(memcmp(string+start, "ctrl", ptr - start) == 0){
        result[ret_ptr++] = 0x00040000;
        start = ptr+1;
      }else if(memcmp(string+start, "alt", ptr - start) == 0){
        result[ret_ptr++] = 0x00080000;
        start = ptr+1;
      }else if(memcmp(string+start, "num_lock", ptr - start) == 0){
        result[ret_ptr++] = 0x00100000;
        start = ptr+1;
      }else if(memcmp(string+start, "meta", ptr - start) == 0){
        result[ret_ptr++] = 0x00400000;
        start = ptr+1;
      }else if(memcmp(string+start, "scroll_lock", ptr - start) == 0){
        result[ret_ptr++] = 0x00800000;
        start = ptr+1;
      }else if(memcmp(string+start, "button1", ptr - start) == 0){
        result[ret_ptr++] = 0x01000000;
        start = ptr+1;
      }else if(memcmp(string+start, "button2", ptr - start) == 0){
        result[ret_ptr++] = 0x02000000;
        start = ptr+1;
      }else if(memcmp(string+start, "button3", ptr - start) == 0){
        result[ret_ptr++] = 0x04000000;
        start = ptr+1;
      }else if(memcmp(string+start, "button5", ptr - start) == 0){
        result[ret_ptr++] = 0x7f000000;
        start = ptr+1;
      }
    }
  }
  int pos = std::min<int>(2, ret_ptr);
  if((ptr - start) == 1){
    result[pos] = (int)tolower(string[start]);
  }else if((ptr - start) > 0){
    if(memcmp(string+start, "space", ptr-start) == 0){
      result[pos] = (int)' ';
    }else if(memcmp(string+start, "backspace", ptr-start) == 0){
      result[pos] = 0xff08;
    }else if(memcmp(string+start, "tab", ptr-start) == 0){
      result[pos] = 0xff09;
    }else if(memcmp(string+start, "iso_key", ptr-start) == 0){
      result[pos] = 0xff0c;
    }else if(memcmp(string+start, "enter", ptr-start) == 0){
      result[pos] = 0xff0d;
    }else if(memcmp(string+start, "pause", ptr-start) == 0){
      result[pos] = 0xff13;
    }else if(memcmp(string+start, "scroll_lock", ptr-start) == 0){
      result[pos] = 0xff14;
    }else if(memcmp(string+start, "escape", ptr-start) == 0){
      result[pos] = 0xff1b;
    }else if(memcmp(string+start, "home", ptr-start) == 0){
      result[pos] = 0xff50;
    }else if(memcmp(string+start, "left", ptr-start) == 0){
      result[pos] = 0xff51;
    }else if(memcmp(string+start, "up", ptr-start) == 0){
      result[pos] = 0xff52;
    }else if(memcmp(string+start, "right", ptr-start) == 0){
      result[pos] = 0xff53;
    }else if(memcmp(string+start, "down", ptr-start) == 0){
      result[pos] = 0xff54;
    }else if(memcmp(string+start, "page_up", ptr-start) == 0){
      result[pos] = 0xff55;
    }else if(memcmp(string+start, "page_down", ptr-start) == 0){
      result[pos] = 0xff56;
    }else if(memcmp(string+start, "end", ptr-start) == 0){
      result[pos] = 0xff57;
    }else if(memcmp(string+start, "print", ptr-start) == 0){
      result[pos] = 0xff61;
    }else if(memcmp(string+start, "insert", ptr-start) == 0){
      result[pos] = 0xff63;
    }else if(memcmp(string+start, "menu", ptr-start) == 0){
      result[pos] = 0xff67;
    }else if(memcmp(string+start, "help", ptr-start) == 0){
      result[pos] = 0xff68;
    }else if(memcmp(string+start, "num_lock", ptr-start) == 0){
      result[pos] = 0xff7f;
    }else if(memcmp(string+start, "kp", ptr-start) == 0){
      result[pos] = 0xff80;
    }else if(memcmp(string+start, "kp0", ptr-start) == 0){
      result[pos] = 0xff80;
    }else if(memcmp(string+start, "kp1", ptr-start) == 0){
      result[pos] = FL_KP + 1;
    }else if(memcmp(string+start, "kp2", ptr-start) == 0){
      result[pos] = FL_KP + 2;
    }else if(memcmp(string+start, "kp3", ptr-start) == 0){
      result[pos] = FL_KP + 3;
    }else if(memcmp(string+start, "kp4", ptr-start) == 0){
      result[pos] = FL_KP + 4;
    }else if(memcmp(string+start, "kp5", ptr-start) == 0){
      result[pos] = FL_KP + 5;
    }else if(memcmp(string+start, "kp6", ptr-start) == 0){
      result[pos] = FL_KP + 6;
    }else if(memcmp(string+start, "kp7", ptr-start) == 0){
      result[pos] = FL_KP + 7;
    }else if(memcmp(string+start, "kp8", ptr-start) == 0){
      result[pos] = FL_KP + 8;
    }else if(memcmp(string+start, "kp9", ptr-start) == 0){
      result[pos] = FL_KP + 9;
    }else if(memcmp(string+start, "kp_enter", ptr-start) == 0){
      result[pos] = 0xff8d;
    }else if(memcmp(string+start, "kp_last", ptr-start) == 0){
      result[pos] = 0xffbd;
    }else if(memcmp(string+start, "f1", ptr-start) == 0){
      result[pos] = FL_F + 1;
    }else if(memcmp(string+start, "f2", ptr-start) == 0){
      result[pos] = FL_F + 2;
    }else if(memcmp(string+start, "f3", ptr-start) == 0){
      result[pos] = FL_F + 3;
    }else if(memcmp(string+start, "f4", ptr-start) == 0){
      result[pos] = FL_F + 4;
    }else if(memcmp(string+start, "f5", ptr-start) == 0){
      result[pos] = FL_F + 5;
    }else if(memcmp(string+start, "f6", ptr-start) == 0){
      result[pos] = FL_F + 6;
    }else if(memcmp(string+start, "f7", ptr-start) == 0){
      result[pos] = FL_F + 7;
    }else if(memcmp(string+start, "f8", ptr-start) == 0){
      result[pos] = FL_F + 8;
    }else if(memcmp(string+start, "f9", ptr-start) == 0){
      result[pos] = FL_F + 9;
    }else if(memcmp(string+start, "f10", ptr-start) == 0){
      result[pos] = FL_F + 10;
    }else if(memcmp(string+start, "f11", ptr-start) == 0){
      result[pos] = FL_F + 11;
    }else if(memcmp(string+start, "f12", ptr-start) == 0){
      result[pos] = FL_F + 12;
    }else if(memcmp(string+start, "f_last", ptr-start) == 0){
      result[pos] = 0xffe0;
    }else if(memcmp(string+start, "shift_l", ptr-start) == 0){
      result[pos] = 0xffe1;
    }else if(memcmp(string+start, "shift_r", ptr-start) == 0){
      result[pos] = 0xffe2;
    }else if(memcmp(string+start, "control_l", ptr-start) == 0){
      result[pos] = 0xffe3;
    }else if(memcmp(string+start, "control_r", ptr-start) == 0){
      result[pos] = 0xffe4;
    }else if(memcmp(string+start, "caps_lock", ptr-start) == 0){
      result[pos] = 0xffe5;
    }else if(memcmp(string+start, "meta_l", ptr-start) == 0){
      result[pos] = 0xffe7;
    }else if(memcmp(string+start, "meta_r", ptr-start) == 0){
      result[pos] = 0xffe8;
    }else if(memcmp(string+start, "alt_l", ptr-start) == 0){
      result[pos] = 0xffe9;
    }else if(memcmp(string+start, "alt_r", ptr-start) == 0){
      result[pos] = 0xffea;
    }else if(memcmp(string+start, "delete", ptr-start) == 0){
      result[pos] = 0xffff;
    }else if(memcmp(string+start, "volume_down", ptr-start) == 0){
      result[pos] = 0xef11;
    }else if(memcmp(string+start, "volume_mute", ptr-start) == 0){
      result[pos] = 0xef12;
    }else if(memcmp(string+start, "volume_up", ptr-start) == 0){
      result[pos] = 0xef13;
    }else if(memcmp(string+start, "media_play", ptr-start) == 0){
      result[pos] = 0xef14;
    }else if(memcmp(string+start, "media_stop", ptr-start) == 0){
      result[pos] = 0xef15;
    }else if(memcmp(string+start, "media_prev", ptr-start) == 0){
      result[pos] = 0xef16;
    }else if(memcmp(string+start, "media_next", ptr-start) == 0){
      result[pos] = 0xef17;
    }else if(memcmp(string+start, "home_page", ptr-start) == 0){
      result[pos] = 0xef18;
    }else if(memcmp(string+start, "mail", ptr-start) == 0){
      result[pos] = 0xef19;
    }else if(memcmp(string+start, "search", ptr-start) == 0){
      result[pos] = 0xef1b;
    }else if(memcmp(string+start, "back", ptr-start) == 0){
      result[pos] = 0xef26;
    }else if(memcmp(string+start, "forward", ptr-start) == 0){
      result[pos] = 0xef27;
    }else if(memcmp(string+start, "stop", ptr-start) == 0){
      result[pos] = 0xef28;
    }else if(memcmp(string+start, "refresh", ptr-start) == 0){
      result[pos] = 0xef29;
    }else if(memcmp(string+start, "sleep", ptr-start) == 0){
      result[pos] = 0xef2f;
    }else if(memcmp(string+start, "favorites", ptr-start) == 0){
      result[pos] = 0xef30;
    }
  }

} // }}}

void ib::utils::get_clipboard(std::string &ret) { // {{{
  auto clipboard = ib::MainWindow::inst()->_getClipboard();
  Fl::paste(*clipboard, 1);
  ret = clipboard->value();
  clipboard->value("");
} // }}}

void ib::utils::set_clipboard(const std::string &text) { // {{{
  ib::utils::set_clipboard(text.c_str());
} // }}}

void ib::utils::set_clipboard(const char *text) { // {{{
  Fl::copy(text, (int)strlen(text), 1);
} // }}}

void ib::utils::to_command_name(std::string &ret, const std::string &string){ // {{{
  //TODO needs more efficiency
  ib::Regex re("\\s+", ib::Regex::NONE);
  re.init();
  std::string tmp;
  re.gsub(tmp, string.c_str(), "_");
  ib::Regex re1("(.*)\\.(\\w+)", ib::Regex::NONE);
  re1.init();
  re1.gsub(ret, tmp.c_str(), "\\1");
} // }}}

int ib::utils::open_directory(const std::string &path, ib::Error &error) { // {{{
  auto &cfg = ib::Config::inst();
  ib::Command cmd;
  cmd.setName("tmp");
  cmd.setPath(cfg.getFileBrowser());
  cmd.init();
  std::vector<std::string*> params;
  params.push_back(new std::string(path));
  int ret = cmd.execute(params, 0, error);
  ib::utils::delete_pointer_vectors(params);
  return ret;
} // }}}

size_t ib::utils::read_socket(FL_SOCKET s, char *buf, const size_t bufsize){ // {{{
  size_t received_size = 0;
  int tmp;
  while(received_size<bufsize){
    tmp = recv(s,buf+received_size,bufsize-received_size,0);
    if(tmp==SOCKET_ERROR){
      return received_size;
    }
    if(tmp==0){ return received_size; }
    received_size+=tmp;
  }
  return received_size;
} // }}}

int ib::utils::ipc_message(const char *message) { // {{{
  const size_t length = strlen(message);
  if(length > 2147483646) return 1;
  ib::u32 ulength = (ib::u32)length;

  struct sockaddr_in server;
  FL_SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == INVALID_SOCKET) { return 1; }

  server.sin_family = AF_INET;
  server.sin_port = htons(ib::Config::inst().getServerPort());
  server.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
  if(connect(sock, (struct sockaddr *)&server, sizeof(server)) != 0) {
    return 2;
  }

  char length_buf[4];
  ib::utils::u32int2bebytes(length_buf, ulength);
  int n = send(sock, length_buf, 4, 0);
  if (n < 0) { return 3; }
  n = send(sock, message, length, 0);
  if (n < 0) { return 3; }
  ib::platform::close_socket(sock);
  return 0;
}
int ib::utils::ipc_message(const std::string &message){ return ib::utils::ipc_message(message.c_str()); }
// }}}

void ib::utils::u32int2bebytes(char *result, ib::u32 value){ // {{{
  result[0] = (value >> 24);
  result[1] = (value >> 16) & 0xff;
  result[2] = (value >> 8 ) & 0xff;
  result[3] = value         & 0xff;
} // }}}

ib::u32 ib::utils::bebytes2u32int(const char *bytes){ // {{{
  return (ib::u32)(((bytes[3] | (bytes[2] << 8)) | (bytes[1] << 0x10)) | (bytes[0] << 0x18));
} // }}}

ib::FlScopedLock::~FlScopedLock() { // {{{
  Fl::unlock();
  Fl::awake(ib::MainWindow::inst());
  Fl::awake(ib::ListWindow::inst());
} // }}}

