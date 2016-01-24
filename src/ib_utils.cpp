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
#include "ib_singleton.h"

// DEBUG {{{
#ifdef DEBUG 
#ifdef IB_OS_WIN
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
void operator delete(void *p) noexcept { __my_free(p); }
#define malloc(size) __my_malloc(size)
#define free(ptr) __my_free(ptr)
#endif
#endif
// }}}

long ib::utils::malloc_count() { // {{{
#ifdef DEBUG 
#ifdef IB_OS_WIN
  return __malloc_count;
#endif
#endif
  return 0;
} // }}}

void ib::utils::exit_application(const int code) { // {{{
  auto config = ib::Singleton<ib::Config>::getInstance();
  auto history = ib::Singleton<ib::History>::getInstance();
  auto icon_manager = ib::Singleton<ib::IconManager>::getInstance();

  if(code == 0) { 
    if(history != 0) history->dump();
    if(config->getEnableIcons() && icon_manager != 0){
      icon_manager->dump();
    }
  }

  ib::SingletonFinalizer::finalize();
  ib::platform::finalize_system();
  exit(code);
} // }}}

void ib::utils::reboot_application() { // {{{
  std::vector<ib::unique_string_ptr> params;
  params.push_back(ib::unique_string_ptr(new std::string("-p")));
  char options[32];
  snprintf(options, 32, "%d", ib::platform::get_pid());
  params.push_back(ib::unique_string_ptr(new std::string(options)));

  const auto cfg = ib::Singleton<ib::Config>::getInstance();
  auto server = ib::Singleton<ib::Server>::getInstance();
  ib::Error error;
  server->shutdown();
  if(ib::platform::shell_execute(cfg->getSelfPath(), params, cfg->getInitialWorkdir(), "auto", error) != 0) {
    server->start(error);
    fl_alert("%s", error.getMessage().c_str());
  }else{
    ib::utils::exit_application(0);
  }
} // }}}

// ib::utils::scan_search_path() { // {{{
static void scan_search_path_awaker1(void *p){ 
  const auto category = reinterpret_cast<const char*>(p);
  auto input = ib::Singleton<ib::MainWindow>::getInstance()->getInput();
  std::string message = "Scanning search paths";
  message +="(category: ";
  message += category;
  message += ")...";
  ib::Singleton<ib::Controller>::getInstance()->showApplication();
  input->value(message.c_str());
  input->adjustSize();
  input->readonly(1);
}
static void scan_search_path_awaker2(void *p){ ib::utils::reboot_application(); }
static ib::threadret scan_search_path_thread(void *p){
  const auto category = reinterpret_cast<const char*>(p);
  ib::platform::on_thread_start();
  sleep(1);
  Fl::awake(scan_search_path_awaker1, p);
  ib::Singleton<ib::Controller>::getInstance()->cacheCommandsInSearchPath(category);
  Fl::awake(scan_search_path_awaker2, 0);
  ib::platform::exit_thread(0);
  return (ib::threadret)0;
}

void ib::utils::scan_search_path(const char *category) {
  const auto cfg = ib::Singleton<ib::Config>::getInstance();
  ib::oschar oscache_path[IB_MAX_PATH];
  ib::platform::utf82oschar_b(oscache_path, IB_MAX_PATH, cfg->getCommandCachePath().c_str());
  if(ib::platform::file_exists(oscache_path)){
    ib::Error error;
    if(ib::platform::remove_file(oscache_path, error) != 0){
      fl_alert("%s", error.getMessage().c_str());
      return;
    }
  }
  ib::thread thread;
  ib::platform::create_thread(&thread, scan_search_path_thread, (void*)category);
} // }}}

void ib::utils::alert_lua_stack(lua_State *L) { // {{{
  int num, i, type;
  if(L == nullptr) {
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
  fl_alert("%s", str.c_str());
} // }}}

std::string ib::utils::expand_vars(const std::string &tpl, const ib::string_map &values) { // {{{
  ib::SimpleTemplateLexer lexer;
  std::string ret;
  lexer.parse(ret, tpl, values);
  return ret;
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

static int ib_utils_parse_single_keyname(const char *string) { // {{{
  if(strcmp(string, "space") == 0){
    return (int)' ';
  }else if(strcmp(string, "backspace") == 0){
    return 0xff08;
  }else if(strcmp(string, "tab") == 0){
    return 0xff09;
  }else if(strcmp(string, "iso_key") == 0){
    return 0xff0c;
  }else if(strcmp(string, "enter") == 0){
    return 0xff0d;
  }else if(strcmp(string, "pause") == 0){
    return 0xff13;
  }else if(strcmp(string, "scroll_lock") == 0){
    return 0xff14;
  }else if(strcmp(string, "escape") == 0){
    return 0xff1b;
  }else if(strcmp(string, "home") == 0){
    return 0xff50;
  }else if(strcmp(string, "left") == 0){
    return 0xff51;
  }else if(strcmp(string, "up") == 0){
    return 0xff52;
  }else if(strcmp(string, "right") == 0){
    return 0xff53;
  }else if(strcmp(string, "down") == 0){
    return 0xff54;
  }else if(strcmp(string, "page_up") == 0){
    return 0xff55;
  }else if(strcmp(string, "page_down") == 0){
    return 0xff56;
  }else if(strcmp(string, "end") == 0){
    return 0xff57;
  }else if(strcmp(string, "print") == 0){
    return 0xff61;
  }else if(strcmp(string, "insert") == 0){
    return 0xff63;
  }else if(strcmp(string, "menu") == 0){
    return 0xff67;
  }else if(strcmp(string, "help") == 0){
    return 0xff68;
  }else if(strcmp(string, "num_lock") == 0){
    return 0xff7f;
  }else if(strcmp(string, "kp") == 0){
    return 0xff80;
  }else if(strcmp(string, "kp0") == 0){
    return 0xff80;
  }else if(strcmp(string, "kp1") == 0){
    return FL_KP + 1;
  }else if(strcmp(string, "kp2") == 0){
    return FL_KP + 2;
  }else if(strcmp(string, "kp3") == 0){
    return FL_KP + 3;
  }else if(strcmp(string, "kp4") == 0){
    return FL_KP + 4;
  }else if(strcmp(string, "kp5") == 0){
    return FL_KP + 5;
  }else if(strcmp(string, "kp6") == 0){
    return FL_KP + 6;
  }else if(strcmp(string, "kp7") == 0){
    return FL_KP + 7;
  }else if(strcmp(string, "kp8") == 0){
    return FL_KP + 8;
  }else if(strcmp(string, "kp9") == 0){
    return FL_KP + 9;
  }else if(strcmp(string, "kp_enter") == 0){
    return 0xff8d;
  }else if(strcmp(string, "kp_last") == 0){
    return 0xffbd;
  }else if(strcmp(string, "f1") == 0){
    return FL_F + 1;
  }else if(strcmp(string, "f2") == 0){
    return FL_F + 2;
  }else if(strcmp(string, "f3") == 0){
    return FL_F + 3;
  }else if(strcmp(string, "f4") == 0){
    return FL_F + 4;
  }else if(strcmp(string, "f5") == 0){
    return FL_F + 5;
  }else if(strcmp(string, "f6") == 0){
    return FL_F + 6;
  }else if(strcmp(string, "f7") == 0){
    return FL_F + 7;
  }else if(strcmp(string, "f8") == 0){
    return FL_F + 8;
  }else if(strcmp(string, "f9") == 0){
    return FL_F + 9;
  }else if(strcmp(string, "f10") == 0){
    return FL_F + 10;
  }else if(strcmp(string, "f11") == 0){
    return FL_F + 11;
  }else if(strcmp(string, "f12") == 0){
    return FL_F + 12;
  }else if(strcmp(string, "f_last") == 0){
    return 0xffe0;
  }else if(strcmp(string, "shift_l") == 0){
    return 0xffe1;
  }else if(strcmp(string, "shift_r") == 0){
    return 0xffe2;
  }else if(strcmp(string, "control_l") == 0){
    return 0xffe3;
  }else if(strcmp(string, "control_r") == 0){
    return 0xffe4;
  }else if(strcmp(string, "caps_lock") == 0){
    return 0xffe5;
  }else if(strcmp(string, "meta_l") == 0){
    return 0xffe7;
  }else if(strcmp(string, "meta_r") == 0){
    return 0xffe8;
  }else if(strcmp(string, "alt_l") == 0){
    return 0xffe9;
  }else if(strcmp(string, "alt_r") == 0){
    return 0xffea;
  }else if(strcmp(string, "delete") == 0){
    return 0xffff;
  }else if(strcmp(string, "volume_down") == 0){
    return 0xef11;
  }else if(strcmp(string, "volume_mute") == 0){
    return 0xef12;
  }else if(strcmp(string, "volume_up") == 0){
    return 0xef13;
  }else if(strcmp(string, "media_play") == 0){
    return 0xef14;
  }else if(strcmp(string, "media_stop") == 0){
    return 0xef15;
  }else if(strcmp(string, "media_prev") == 0){
    return 0xef16;
  }else if(strcmp(string, "media_next") == 0){
    return 0xef17;
  }else if(strcmp(string, "home_page") == 0){
    return 0xef18;
  }else if(strcmp(string, "mail") == 0){
    return 0xef19;
  }else if(strcmp(string, "search") == 0){
    return 0xef1b;
  }else if(strcmp(string, "back") == 0){
    return 0xef26;
  }else if(strcmp(string, "forward") == 0){
    return 0xef27;
  }else if(strcmp(string, "stop") == 0){
    return 0xef28;
  }else if(strcmp(string, "refresh") == 0){
    return 0xef29;
  }else if(strcmp(string, "sleep") == 0){
    return 0xef2f;
  }else if(strcmp(string, "favorites") == 0){
    return 0xef30;
  }else{
    ib::Regex re("0x[0-9a-fA-F]+", ib::Regex::NONE);
    re.init();
    if(re.match(string) == 0) {
      int num = 0;
      sscanf(string, "%x", &num);
      return num;
    }
  }
  return -1;
} // }}}

void ib::utils::parse_key_bind(int *result, const char *string) { // {{{
  auto ret = ib_utils_parse_single_keyname(string);
  if(ret > -1) {
    result[0] = ret;
    return;
  }
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
    ret = ib_utils_parse_single_keyname(string+start);
    if(ret > -1) {
      result[pos] = ret;
    }
  }

} // }}}

void ib::utils::get_clipboard(std::string &ret) { // {{{
  auto clipboard = ib::Singleton<ib::MainWindow>::getInstance()->_getClipboard();
  Fl::paste(*clipboard, 1);
  ret = clipboard->value();
  clipboard->value("");
} // }}}

void ib::utils::set_clipboard(const std::string &text) { // {{{
  ib::utils::set_clipboard(text.c_str());
} // }}}

void ib::utils::set_clipboard(const char *text) { // {{{
  Fl::copy(text, (int)strlen(text), 1);
#ifndef IB_OS_WIN
  std::string tmp;
  ib::utils::get_clipboard(tmp);
#endif
} // }}}

std::string ib::utils::to_command_name(const std::string &string){ // {{{
  //TODO needs more efficiency
  ib::Regex re("\\s+", ib::Regex::NONE);
  re.init();
  auto tmp = re.gsub(string.c_str(), "_");
  ib::Regex re1("(.*)\\.(\\w+)", ib::Regex::NONE);
  re1.init();
  auto ret = re1.gsub(tmp.c_str(), "\\1");
  return ret;
} // }}}

int ib::utils::open_directory(const std::string &path, ib::Error &error) { // {{{
  const auto cfg = ib::Singleton<ib::Config>::getInstance();
  ib::Command cmd;
  cmd.setName("tmp");
  cmd.setPath(cfg->getFileBrowser());
  cmd.init();
  std::vector<std::string*> params;
  params.push_back(new std::string(path));
  auto ret = cmd.execute(params, nullptr, error);
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
  const auto length = strlen(message);
  if(length > 2147483646) return 1;
  auto ulength = static_cast<ib::u32>(length);

  struct sockaddr_in server;
  FL_SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock == INVALID_SOCKET) { return 1; }

  server.sin_family = AF_INET;
  server.sin_port = htons(ib::Singleton<ib::Config>::getInstance()->getServerPort());
#ifdef IB_OS_WIN
  server.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
#else
  server.sin_addr.s_addr = inet_addr("127.0.0.1");
#endif
  if(connect(sock, (struct sockaddr *)&server, sizeof(server)) != 0) {
    return 2;
  }

  char length_buf[4];
  ib::utils::u32int2bebytes(length_buf, ulength);
  auto n = send(sock, length_buf, 4, 0);
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
} // }}}

