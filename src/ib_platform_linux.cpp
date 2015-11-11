#include "ib_platform.h"
#include "ib_resource.h"
#include "ib_utils.h"
#include "ib_ui.h"
#include "ib_controller.h"
#include "ib_config.h"
#include "ib_regex.h"
#include "ib_comp_value.h"
#include "ib_icon_manager.h"

static char ib_g_lang[32];
static int  ib_g_hotkey;

// utilities {{{

// small functions {{{
char* strncpy_s(char *dest, const char *src, const std::size_t size) {
  strncpy(dest, src, size-1);
  dest[size-1] = '\0';
  return dest;
}

static bool string_startswith(const char *str, const char *pre) {
  return strncmp(pre, str, strlen(pre)) == 0;
}

static bool string_endswith(const char *str, const char *pre) {
  const char *dot = strrchr(str, '.');
  return (dot && !strcmp(dot, pre));
}

static void parse_desktop_entry_value(std::string &result, std::string &value) {
  ib::Regex re("(\\\\s|\\\\n|\\\\t|\\\\r|\\\\\\\\)", ib::Regex::NONE);
  re.init();
  re.gsub(result, value, [](const ib::Regex &reg, std::string *res, void *userdata) {
      std::string escape;
      reg._1(escape);
      if(escape == "\\s") *res += " ";
      else if(escape == "\\n") *res += "\n";
      else if(escape == "\\t") *res += "\t";
      else if(escape == "\\r") *res += "\r";
      else if(escape == "\\\\") *res += "\\";
  }, 0);
}

static void normalize_desktop_entry_value(std::string &result, std::string &value) {
  ib::Regex re("(\\n|\\r|\\t)", ib::Regex::NONE);
  re.init();
  re.gsub(result, value, [](const ib::Regex &reg, std::string *res, void *userdata) {
      std::string escape;
      reg._1(escape);
      if(escape == "\t") *res += "    ";
  }, 0);
}

static int read_fd_all(int fd, std::string &out) {
  char buf[1024];
  do{
    ssize_t ret = read(fd, buf, 1023);
    if(ret < 0) {
      return -1;
    } else {
      buf[ret] = '\0';
      out += buf;
      if(ret == 0) break;
    }
  }while(1);
  return 0;
}

static int parse_cmdline(std::vector<ib::unique_char_ptr> &result, const char *cmdline) {
  ib::Regex space_r("\\s+", ib::Regex::I);
  ib::Regex string_r("\"((((?<=\\\\)\")|[^\"])*)((?<!\\\\)\")", ib::Regex::I);
  ib::Regex value_r("([^\\(\\[\\)\\]\\s\"]+)", ib::Regex::I);
  space_r.init();
  string_r.init();
  value_r.init();
  std::size_t pos = 0;
  std::size_t len = strlen(cmdline);
  while(pos < len) {
    std::string cap;
    if(space_r.match(cmdline, pos) == 0) {
      pos = space_r.getEndpos(0) + 1;
    } else if(string_r.match(cmdline, pos) == 0) {
      string_r._1(cap);
      char *buf = new char[cap.length()+1];
      strcpy(buf, cap.c_str());
      result.push_back(ib::unique_char_ptr(buf));
      pos = string_r.getEndpos(0) + 1;
    } else if(value_r.match(cmdline, pos) == 0) {
      value_r._1(cap);
      char *buf = new char[cap.length()+1];
      strcpy(buf, cap.c_str());
      result.push_back(ib::unique_char_ptr(buf));
      pos = value_r.getEndpos(0) + 1;
    } else {
      return -1;
    }
  }
  return 0;
}

static void set_errno(ib::Error &error) {
  char buf[1024];
  error.setMessage(strerror_r(errno, buf, 1024));
  error.setCode(errno);
}

static bool xdg_mime_filetype(std::string &result, const char *path, ib::Error &e) {
  std::string out, err;
  char quoted[IB_MAX_PATH];
  ib::platform::quote_string(quoted, path);
  if(ib::platform::command_output(out, err, ("xdg-mime query filetype " + std::string(quoted)).c_str(), e) == 0) {
    ib::Regex re("([^;]+);.*", ib::Regex::NONE);
    re.init();
    if(re.match(out) == 0) {
      re._1(result);
      return true;
    }
  }
  return false;
}


static bool xdg_mime_default_app(std::string &result, const char *mime, ib::Error &e) {
  std::string out, err;
  if(ib::platform::command_output(out, err, ("xdg-mime query default " + std::string(mime)).c_str(), e) == 0) {
    result = out;
    return true;
  }
  return false;
}

// }}}

class FreeDesktopKVFile : private ib::NonCopyable<FreeDesktopKVFile> { // {{{
  public:
    explicit FreeDesktopKVFile(const char *path) : path_(), lang_(""), country_(""), modifier_(""), map_() {
      strncpy_s(path_, path, IB_MAX_PATH);
    };
    int parse() {
      std::ifstream ifs(path_);
      std::string   line;
      if (ifs.fail()) {
        return -1;
      }
      ib::Regex kv_reg("^([^#][^=\\s]+)\\s*=\\s*(.*)$", ib::Regex::I);
      kv_reg.init();
      ib::Regex section_reg("^\\s*\\[([^\\]]+)]", ib::Regex::I);
      section_reg.init();

      std::string current_section;
      while(std::getline(ifs, line)) {
        if(section_reg.match(line.c_str()) == 0) {
          section_reg._1(current_section);
        } if(kv_reg.match(line.c_str()) == 0) {
          if(current_section.empty()) {
            return -1;
          }
          std::string   key;
          std::string   raw_value;
          std::string   value;
          kv_reg._1(key);
          kv_reg._2(raw_value);
          parse_desktop_entry_value(value, raw_value);
          map_[std::make_tuple(current_section, key)] = value;
        }
      }
      const char *locale = getenv("LC_MESSAGES");
      if(locale == 0) locale = getenv("LANGUAGE");
      if(locale == 0) locale = getenv("LANG");
      if(locale != 0) {
        ib::Regex locale_reg("(([^\\._@]+)((\\.)([^_]*))?)(_(\\w+))?(@(\\w+))?", ib::Regex::I);
        locale_reg.init();
        if(locale_reg.match(locale) == 0){
          locale_reg._2(lang_);
          locale_reg._7(country_);
          locale_reg._9(modifier_);
        }
      }
      return 0;
    }
    ~FreeDesktopKVFile() { }

    std::string get(const char *section, const char *key, bool l18n = true) {
      std::string skey(key);
      if(!l18n) {
        auto value = map_.find(std::make_tuple(section, skey));
        if(value != map_.end()) return (*value).second;
        return "";
      }

      auto value = map_.find(std::make_tuple(section, skey+"["+lang_+"_"+country_+"@"+modifier_+"]"));
      if(value != map_.end()) return (*value).second;

      value = map_.find(std::make_tuple(section, skey+"["+lang_+"_"+country_+"]"));
      if(value != map_.end()) return (*value).second;

      value = map_.find(std::make_tuple(section, skey+"["+lang_+"@"+modifier_+"]"));
      if(value != map_.end()) return (*value).second;

      value = map_.find(std::make_tuple(section, skey+"["+lang_+"]"));
      if(value != map_.end()) return (*value).second;

      value = map_.find(std::make_tuple(section, skey));
      if(value != map_.end()) return (*value).second;
      return "";
    }

    const char* getPath() const { return path_; }

  protected:
    char           path_[IB_MAX_PATH];
    std::string    lang_;
    std::string    country_;
    std::string    modifier_;
    std::map<std::tuple<std::string, std::string>, std::string> map_;
}; // }}}

class FreeDesktopMime: private ib::NonCopyable<FreeDesktopMime> { // {{{
  public:
    static FreeDesktopMime *instance_;
    static FreeDesktopMime* inst() { return instance_; }
    static void init() { instance_ = new FreeDesktopMime(); instance_->build();}
    
    FreeDesktopMime() : globs_() {};
    void build();
    bool findByPath(std::string &typ, std::string &subtyp, const char *path);
  protected:
    std::vector<std::tuple<int, std::string, std::string, std::string> > globs_;
}; 

FreeDesktopMime* FreeDesktopMime::instance_ = 0;

void FreeDesktopMime::build() {
  const char *files[] = {"/usr/share/mime/globs2", "/usr/share/mime/globs"};
  for(int i = 0; i < 2; i++) {
    const char *file = files[i];
    std::ifstream ifs(file);
    std::string   line;
    if (ifs.fail()) {
      continue;
    }
    ib::Regex reg("\\s*(?:([0-9]+):)?([^:/]+)/([^:/]+):(.*)", ib::Regex::I);
    reg.init();

    while(std::getline(ifs, line)) {
      if(reg.match(line) == 0) {
        std::string scorestr, typ, subtyp, glob;
        reg._1(scorestr); reg._2(typ); reg._3(subtyp); reg._4(glob);
        int score = 0;
        if(!scorestr.empty()) score = std::atoi(scorestr.c_str());
        globs_.push_back(std::make_tuple(score, typ, subtyp, glob));
      }
    }
    break;
  }
}

bool FreeDesktopMime::findByPath(std::string &typ, std::string &subtyp, const char *path) {
  char name[IB_MAX_PATH];
  ib::platform::basename(name, path);
  if(ib::platform::directory_exists(path)) {
    typ = "inode";
    subtyp = "directory";
    return true;
  }
  for(auto it = globs_.begin(), last = globs_.end(); it != last; ++it) {
    auto tup  = (*it);
    auto &glob = std::get<3>(tup);
    if(fl_filename_match(name, glob.c_str())) {
      typ = std::get<1>(tup);
      subtyp = std::get<2>(tup);
      return true;
    }
  }
  return false;
}

//}}}

class FreeDesktopThemeRepos : private ib::NonCopyable<FreeDesktopThemeRepos> { // {{{
  public:
    static FreeDesktopThemeRepos *instance_;
    static FreeDesktopThemeRepos* inst() { return instance_; }
    static void init() { instance_ = new FreeDesktopThemeRepos(); instance_->build();}

    FreeDesktopThemeRepos() : indicies_() {};
    void build();
    FreeDesktopKVFile* getTheme(const char *name) const;
    void findIcon(std::string &result, const char *theme, const char *name, int size);

  protected:
    std::map<std::string, std::unique_ptr<FreeDesktopKVFile> > indicies_;
    void buildHelper(const char *basepath);

};

class FreeDesktopIconFinder : private ib::NonCopyable<FreeDesktopIconFinder> { // {{{
  public:
    FreeDesktopIconFinder(const char *name, const int size) : name_(name), size_(size) {};

    void find(std::string &result);
  protected:
    const char *name_;
    const int   size_;

    void findHelper(std::string &result, const char *theme);
    void lookup(std::string &result, const char *theme);
    bool directoryMatchSize(const char *dir, const char *theme);
    int directorySizeDistance(const char *dir, const char *theme);
    std::tuple<std::string, int,int,int,int> getDirectorySizes(const char *dir, const char *theme);
};

void FreeDesktopIconFinder::find(std::string &result) {
  const char *theme = ib::Config::inst().getIconTheme().c_str();
  findHelper(result, theme);
  if(!result.empty()) return;
  findHelper(result, "Hicolor");
  if(!result.empty()) return;

  const char *pixmaps = "/usr/share/pixmaps";
  char icon_file[IB_MAX_PATH];
  const char *exts[] = {"png", "svg", "xpm"};
  for(int i = 0; i < 3; i++) {
    const char *ext = exts[i];
    snprintf(icon_file, IB_MAX_PATH, "%s/%s.%s", pixmaps, name_, ext);
    if(ib::platform::path_exists(icon_file)) {
      result = icon_file;
      return;
    }
  }
}

void FreeDesktopIconFinder::findHelper(std::string &result, const char *theme) {
  lookup(result, theme);
  if(!result.empty()) return;
  auto kvf = FreeDesktopThemeRepos::inst()->getTheme(theme);
  if(kvf == NULL) return;
  auto parents = kvf->get("Icon Theme", "Inherits", false);
  if(parents.empty() && strcmp(theme, "Hicolor") != 0) parents = "Hicolor";
  std::istringstream stream(parents);
  std::string parent;
  while (std::getline(stream, parent, ',')) {
    findHelper(result, parent.c_str());
    if(!result.empty()) return;
  }
}

void FreeDesktopIconFinder::lookup(std::string &result, const char *theme) {
  auto kvf = FreeDesktopThemeRepos::inst()->getTheme(theme);
  if(kvf == NULL) return;
  auto dirs = kvf->get("Icon Theme", "Directories", false);
  char theme_dir[IB_MAX_PATH];
  ib::platform::dirname(theme_dir, kvf->getPath());
  std::istringstream stream(dirs);
  std::string dir;
  char icon_file[IB_MAX_PATH];
  while (std::getline(stream, dir, ',')) {
    if(directoryMatchSize(dir.c_str(), theme)) {
      const char *exts[] = {"png", "svg", "xpm"};
      for(int i = 0; i < 3; i++) {
        const char *ext = exts[i];
        snprintf(icon_file, IB_MAX_PATH, "%s/%s/%s.%s", theme_dir, dir.c_str(), name_, ext);
        if(ib::platform::path_exists(icon_file)) {
          result = icon_file;
          return;
        }
      }
    }
  }

  int minsize = INT_MAX;
  std::istringstream stream2(dirs);
  while (std::getline(stream2, dir, ',')) {
    const char *exts[] = {"png", "svg", "xpm"};
    for(int i = 0; i < 3; i++) {
      const char *ext = exts[i];
      snprintf(icon_file, IB_MAX_PATH, "%s/%s/%s.%s", theme_dir, dir.c_str(), name_, ext);
      if(ib::platform::path_exists(icon_file)){
        auto distance = directorySizeDistance(dir.c_str(), theme);
        if(distance < minsize) {
          result = icon_file;
          minsize = distance;
        }
      }
    }
  }

}

std::tuple<std::string, int,int,int,int> FreeDesktopIconFinder::getDirectorySizes(const char *dir, const char *theme) {
  std::string typ;
  int size, min, max, th = 0;
  auto kvf = FreeDesktopThemeRepos::inst()->getTheme(theme);
  if(kvf != NULL) {
    typ = kvf->get(dir, "Type", false);
    if(typ.empty()) typ = "Threshold";
    size = std::atoi(kvf->get(dir, "Size", false).c_str());
    min = size;
    max = size;
    auto minstr = kvf->get(dir, "MinSize", false);
    auto maxstr = kvf->get(dir, "MaxSize", false);
    if(!minstr.empty()) min = std::atoi(minstr.c_str());
    if(!maxstr.empty()) max = std::atoi(maxstr.c_str());
    th = 2;
    auto thstr = kvf->get(dir, "Threshold", false);
    if(!thstr.empty()) th = std::atoi(thstr.c_str());
  }

  return std::make_tuple(typ, size, min, max, th);

}

bool FreeDesktopIconFinder::directoryMatchSize(const char *dir, const char *theme) {
  auto sizedata = getDirectorySizes(dir, theme);
  auto typ = std::get<0>(sizedata);
  auto size = std::get<1>(sizedata);
  auto min = std::get<2>(sizedata);
  auto max = std::get<3>(sizedata);
  auto th  = std::get<4>(sizedata);

  if(typ == "Fixed") {
    return size == size_;
  } else if(typ == "Scalable") {
    return min <= size_ && size_ <= max;
  } else if(typ == "Threshold"){
    return (size - th) <= size_ && size_ <= (size + th);
  }
  return false;
}

int FreeDesktopIconFinder::directorySizeDistance(const char *dir, const char *theme) {
  auto sizedata = getDirectorySizes(dir, theme);
  auto &typ = std::get<0>(sizedata);
  auto size = std::get<1>(sizedata);
  auto min = std::get<2>(sizedata);
  auto max = std::get<3>(sizedata);
  auto th  = std::get<4>(sizedata);
  if(typ == "Fixed") {
    return std::abs(size - size_);
  } else if(typ == "Scalable") {
    if(size_ < min) return min- size_;
    if(size_ > max) return size_ - max;
    return 0;
  } else if(typ == "Threshold"){
    if(size_ < (size - th)) return (size - th) - size_;
    if(size_ > (size + th)) return size_ - (size + th);
    return 0;
  }
  return 0;
}
// }}}

FreeDesktopThemeRepos *FreeDesktopThemeRepos::instance_ = 0;

void FreeDesktopThemeRepos::build() {
  char path[IB_MAX_PATH];
  snprintf(path, IB_MAX_PATH, "%s/.icons", getenv("HOME"));
  buildHelper(path);

  if(getenv("XDG_DATA_DIRS") == 0) {
    snprintf(path, IB_MAX_PATH, "/usr/share/icons");
  }else{
    snprintf(path, IB_MAX_PATH, "%s/icons", getenv("XDG_DATA_DIRS"));
  }
  buildHelper(path);

  snprintf(path, IB_MAX_PATH, "/usr/share/pixmaps");
  buildHelper(path);
}

void FreeDesktopThemeRepos::buildHelper(const char *basepath) {
  char path[IB_MAX_PATH];
  char index_path[IB_MAX_PATH];
  ib::Error error;
  std::vector<ib::unique_oschar_ptr> files;

  if(ib::platform::walk_dir(files, basepath, error, false) != 0) {
    return; //ignore errors
  }
  for(auto it = files.begin(), last = files.end(); it != last; ++it){
    snprintf(path, IB_MAX_PATH, "%s/%s", basepath, (const char*)((*it).get()));
    if(ib::platform::is_directory(path)) {
      snprintf(index_path, IB_MAX_PATH, "%s/%s", path, "index.theme");
      if(ib::platform::file_exists(index_path)) {
        FreeDesktopKVFile *kvf = new FreeDesktopKVFile(index_path);
        if(kvf->parse() < 0) { delete kvf; continue; /* ignore errors */ }
        auto name = kvf->get("Icon Theme", "Name", false);
        if(name.empty()) {
          delete kvf; 
          continue;
          // TODO How we should handle an inheritance?
        }
        indicies_.insert(std::make_pair(name, std::unique_ptr<FreeDesktopKVFile>(kvf)));
      }
    }
  }
}

FreeDesktopKVFile* FreeDesktopThemeRepos::getTheme(const char *name) const {
  auto themeptr = indicies_.find(name);
  if(themeptr == indicies_.end()) return 0;
  return themeptr->second.get();
}


void FreeDesktopThemeRepos::findIcon(std::string &result, const char *theme, const char *name, int size) {
  FreeDesktopIconFinder ficon(name, size);
  ficon.find(result);
}

// }}}
// }}}

//////////////////////////////////////////////////
// X11 functions {{{
//////////////////////////////////////////////////

static int xerror_handler(Display* d, XErrorEvent* e) {
  char buf1[128], buf2[128];
  sprintf(buf1, "XRequest.%d", e->request_code);
  XGetErrorDatabaseText(d,"",buf1,buf1,buf2,128);
  XGetErrorText(d, e->error_code, buf1, 128);
  printf("xerror: %s(%d): %s 0x%lx\n", buf2, e->error_code, buf1, e->resourceid);
  return 0;
}

static int xevent_handler(int e){
  if (!e) {
    //Window window = fl_xevent->xany.window;
    switch(fl_xevent->type){
      case KeyPress: // hotkey
        ib::Controller::inst().showApplication();
        return 1;
    }
  } 
  return 0;
}

static int xregister_hotkey() {
  Window root = RootWindow(fl_display, fl_screen);
  const int* hot_key = ib::Config::inst().getHotKey();
  for(int i=0 ;hot_key[i] != 0; ++i){
    ib_g_hotkey += hot_key[i];
  }
  const int keycode = XKeysymToKeycode(fl_display, ib_g_hotkey & 0xFFFF);
  if(!keycode) {
    return -1;
  }
  XGrabKey(fl_display, keycode, ib_g_hotkey >>16,     root, 1, GrabModeAsync, GrabModeAsync);
  XGrabKey(fl_display, keycode, (ib_g_hotkey>>16)|2,  root, 1, GrabModeAsync, GrabModeAsync); // CapsLock
  XGrabKey(fl_display, keycode, (ib_g_hotkey>>16)|16, root, 1, GrabModeAsync, GrabModeAsync); // NumLock
  XGrabKey(fl_display, keycode, (ib_g_hotkey>>16)|18, root, 1, GrabModeAsync, GrabModeAsync); // both
  return 0;
}

static void signal_handler(int s){
  ib::utils::exit_application(0);
}

static void* xget_property(Window w, const char *prop, Atom typ) {
  Atom aret;
  int f = 0;
  unsigned long n, b;
  long *ptr = 0;

  if (XGetWindowProperty(fl_display, w, XInternAtom(fl_display, prop, False), 0, 1, False, typ, &aret, &f, &n, &b, (unsigned char**)&ptr) != Success) {
      return 0;
  }
  if (aret != typ || n == 0) {
    return 0;
  }
  return ptr;
}

static void xsend_message_l(Window w, const char *msg, long d0, long d1, long d2, long d3, long d4) {
  XClientMessageEvent ev = {0};
  ev.type = ClientMessage;
  ev.window = w;
  ev.message_type = XInternAtom(fl_display, msg, False);
  ev.format = 32;
  ev.data.l[0] = d0;
  ev.data.l[1] = d1;
  ev.data.l[2] = d2;
  ev.data.l[3] = d3;
  ev.data.l[4] = d4;
  XSendEvent(fl_display, DefaultRootWindow(fl_display), 0,
    SubstructureRedirectMask |SubstructureNotifyMask, (XEvent*)&ev);
}

static int xcurrent_desktop() {
  long *desktop = (long*)xget_property(DefaultRootWindow(fl_display), "_NET_CURRENT_DESKTOP", XA_CARDINAL);
  if(desktop == 0) {
    return -1;
  }
  int ret = (int)desktop[0];
  XFree(desktop);
  return (int)ret;
}

//////////////////////////////////////////////////
// }}}
//////////////////////////////////////////////////

int ib::platform::startup_system() { // {{{
  XSetErrorHandler(xerror_handler);
  signal( SIGINT, signal_handler );
  signal( SIGTERM, signal_handler );
  // TODO get user LANG from ENV.
  strncpy_s(ib_g_lang, "UTF-8", 32);
  FreeDesktopThemeRepos::init();
  FreeDesktopMime::init();
  return 0;
} // }}}

int ib::platform::init_system() { // {{{
  ib::ListWindow::inst()->set_menu_window();
  XAllowEvents(fl_display, AsyncKeyboard, CurrentTime);
  XkbSetDetectableAutoRepeat(fl_display, true, NULL);
  if(xregister_hotkey() < 0 ) {
    fl_alert("%s", "Failed to register hotkeys.");
    ib::utils::exit_application(1);
  }

  Fl::add_handler(xevent_handler);
  XFlush(fl_display);

  return 0;
} // }}}

void ib::platform::finalize_system(){ // {{{ 
  Window root = XRootWindow(fl_display, fl_screen);
  const int keycode = XKeysymToKeycode(fl_display, ib_g_hotkey & 0xFFFF);
  XUngrabKey(fl_display, keycode, ib_g_hotkey >>16,     root);
  XUngrabKey(fl_display, keycode, (ib_g_hotkey>>16)|2,  root); // CapsLock
  XUngrabKey(fl_display, keycode, (ib_g_hotkey>>16)|16, root); // NumLock
  XUngrabKey(fl_display, keycode, (ib_g_hotkey>>16)|18, root); // both
  
  delete FreeDesktopThemeRepos::inst();
  delete FreeDesktopMime::inst();
} // }}}

void ib::platform::get_runtime_platform(char *ret){ // {{{ 
} // }}}

ib::oschar* ib::platform::utf82oschar(const char *src) { // {{{
  char *buff = new char[strlen(src)+1];
  strcpy(buff, src);
  return buff;
} // }}}

void ib::platform::utf82oschar_b(ib::oschar *buf, const unsigned int bufsize, const char *src) { // {{{
  strncpy_s(buf, src, bufsize);
} // }}}

char* ib::platform::oschar2utf8(const ib::oschar *src) { // {{{
  char *buff = new char[strlen(src)+1];
  strcpy(buff, src);
  return buff;
} // }}}

void ib::platform::oschar2utf8_b(char *buf, const unsigned int bufsize, const ib::oschar *src) { // {{{
  strncpy_s(buf, src, bufsize);
} // }}}

char* ib::platform::oschar2local(const ib::oschar *src) { // {{{
  char *buff = new char[strlen(src)+1];
  strcpy(buff, src);
  return buff;
} // }}}

void ib::platform::oschar2local_b(char *buf, const unsigned int bufsize, const ib::oschar *src) { // {{{
  strncpy_s(buf, src, bufsize);
} // }}}

char* ib::platform::utf82local(const char *src) { // {{{
  char *buff = new char[strlen(src)+1];
  strcpy(buff, src);
  return buff;
} // }}}

char* ib::platform::local2utf8(const char *src) { // {{{
  char *buff = new char[strlen(src)+1];
  strcpy(buff, src);
  return buff;
} // }}}

ib::oschar* ib::platform::quote_string(ib::oschar *result, const ib::oschar *str) { // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  bool need_quote = false;
  for(const ib::oschar *tmp = str; *tmp != '\0'; tmp++) {
    if(isspace(*tmp)) { need_quote = true; break; }
  }
  if((strlen(str) != 0 && str[0] == '"') || !need_quote) {
    strncpy_s(result, str, IB_MAX_PATH);
    return result;
  }

  ib::oschar *cur = result + 1;
  result[0] = '"';

  for(std::size_t i =0 ;*str != '\0' && i != IB_MAX_PATH-2; cur++, str++, i++){
    if(*str == '"'){
      *cur = '\\';
      cur++;
    }
    *cur = *str;
  }
  *cur = '"';

  return result;
} // }}}

ib::oschar* ib::platform::unquote_string(ib::oschar *result, const ib::oschar *str) { // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  std::size_t len = strlen(str);
  if(len == 0) { result[0] = '\0'; return result; }
  if(len == 1) { strcpy(result, str); return result; }
  if(*str == '"') str++;
  strncpy_s(result, str, IB_MAX_PATH);
  len = strlen(result);
  if(result[len-1] == '"') {
    result[len-1] = '\0';
  }
  return result;
} // }}}

void ib::platform::hide_window(Fl_Window *window){ // {{{
  XIconifyWindow(fl_display, fl_xid(window), fl_screen);
  XFlush(fl_display);
} // }}}

void ib::platform::show_window(Fl_Window *window){ // {{{
  Atom wm_states[3];
  Atom wm_state;
  wm_state = XInternAtom(fl_display, "_NET_WM_STATE", False);
  wm_states[1] = XInternAtom(fl_display, "_NET_WM_STATE_SKIP_TASKBAR", False);
  XChangeProperty(fl_display, fl_xid(window), wm_state, XA_ATOM, 32, PropModeReplace,(unsigned char *) wm_states, 3);

  xsend_message_l(fl_xid(window), "_NET_ACTIVE_WINDOW", 1, CurrentTime, 0,0,0);

  XFlush(fl_display);
} // }}}

void ib::platform::set_window_alpha(Fl_Window *window, int alpha){ // {{{
  int visible = window->visible();
  if(!visible) window->show();
  double  a = alpha/255.0;
  uint32_t cardinal_alpha = (uint32_t) (a * (uint32_t)-1) ;
  XChangeProperty(fl_display, fl_xid(window), 
                 XInternAtom(fl_display, "_NET_WM_WINDOW_OPACITY", 0),
                 XA_CARDINAL, 32, PropModeReplace, (uint8_t*)&cardinal_alpha, 1) ;
  if(!visible) window->hide();
} // }}}

static int ib_platform_shell_execute(const std::string &path, const std::string &strparams, const std::string &cwd, ib::Error &error) { // {{{
  std::string cmd;
  ib::oschar quoted_path[IB_MAX_PATH];
  ib::platform::quote_string(quoted_path, path.c_str());
  int ret;

  if(ib::platform::directory_exists(path.c_str())) {
    if(ib::utils::open_directory(path, error) != 0) {
      return -1;
    }
    return 0;
  }

  ib::oschar wd[IB_MAX_PATH];
  ib::platform::get_current_workdir(wd);
  if(ib::platform::set_current_workdir(cwd.c_str(), error) != 0) {
    return -1;
  };

  ib::Regex proto_reg("^(\\w+)://.*", ib::Regex::I);
  proto_reg.init();
  ret = 0;
  if(proto_reg.match(path) == 0){
    cmd += "xdg-open ";
    cmd += path;
    cmd += " ";
    cmd += strparams;
  } else {
    if(-1 == access(path.c_str(), R_OK)) {
      set_errno(error);
      ret = -1;
      goto finally;
    }
    if(!strparams.empty() || access(path.c_str(), X_OK) == 0) {
      cmd += quoted_path;
      cmd += " ";
      cmd += strparams;
    } else {
      std::string mime, app;
      if(!xdg_mime_filetype(mime, quoted_path, error)) {
        ret = -1;
        goto finally;
      }
      if(!mime.empty()) {
        if(!xdg_mime_default_app(app, mime.c_str(), error)) {
            ret = -1;
            goto finally;
        }
        if(app.empty()){
          std::ostringstream message;
          message << "No associated applications found for " << quoted_path << " (mimetype: " << mime << ").";
          error.setCode(1);
          error.setMessage(message.str().c_str());
          ret = -1;
          goto finally;
        }
      } else {
        std::ostringstream message;
        message << "No mime types found for "<<quoted_path;
        error.setCode(1);
        error.setMessage(message.str().c_str());
        ret = -1;
        goto finally;
      }
      cmd += "xdg-open ";
      cmd += quoted_path;
    }
  }

  ret = system((cmd + " &").c_str());
  if(ret != 0) {
    std::ostringstream message;
    message << "Failed to start " << cmd;
    error.setCode(1);
    error.setMessage(message.str().c_str());
    goto finally;
  }

finally:
  
  ib::platform::set_current_workdir(wd, error);
  return ret;
} // }}}

int ib::platform::shell_execute(const std::string &path, const std::vector<ib::unique_string_ptr> &params, const std::string &cwd, ib::Error &error) { // {{{
  std::string strparams;
  for(auto it = params.begin(), last = params.end(); it != last; ++it){
    ib::oschar qparam[IB_MAX_PATH];
    strparams += " ";
    ib::platform::quote_string(qparam, (*it).get()->c_str());
    strparams += qparam;
  }
  return ib_platform_shell_execute(path, strparams, cwd, error);
} /* }}} */

int ib::platform::shell_execute(const std::string &path, const std::vector<std::string*> &params, const std::string &cwd, ib::Error &error) { // {{{
  std::string strparams;
  for(auto it = params.begin(), last = params.end(); it != last; ++it){
    ib::oschar qparam[IB_MAX_PATH];
    strparams += " ";
    ib::platform::quote_string(qparam, (*it)->c_str());
    strparams += qparam;
  }
  return ib_platform_shell_execute(path, strparams, cwd, error);
} /* }}} */

int ib::platform::command_output(std::string &sstdout, std::string &sstderr, const char *cmd, ib::Error &error) { // {{{
  int outfd[2];
  int efd[2];
  int infd[2];
  pid_t pid;

  std::vector<ib::unique_char_ptr> argv;
  if(parse_cmdline(argv, cmd) != 0) {
    error.setMessage("Invalid command line.");
    error.setCode(1);
    return 1;
  }
  std::unique_ptr<char*[]> cargv(new char*[argv.size()+1]);
  for(std::size_t i = 0; i < argv.size(); i++) {
    cargv.get()[i] = argv.at(i).get();
  }
  cargv[argv.size()] = NULL;

  if(pipe(outfd) != 0 || pipe(infd) != 0 || pipe(efd) != 0) {
    error.setMessage("Failed to create pipes.");
    error.setCode(1);
    return 1;
  }

  pid = fork();
  if(pid < 0) {
    set_errno(error);
    return 1;
  } else if(pid == 0) {
    if(infd[0] != STDIN_FILENO) {
      dup2(infd[0], STDIN_FILENO);
      close(infd[0]);
    }
    if(outfd[1] != STDOUT_FILENO) {
      dup2(outfd[1], STDOUT_FILENO);
      close(outfd[1]);
    }
    if(efd[1] != STDERR_FILENO) {
      dup2(efd[1], STDERR_FILENO);
      close(efd[1]);
    }
    close(infd[1]);
    close(outfd[0]);
    close(efd[0]);
    execvp(cargv.get()[0],cargv.get());
    exit(127);
  } else {
    close(infd[0]);
    close(outfd[1]);
    close(efd[1]);

    read_fd_all(outfd[0], sstdout); // ignore errors
    read_fd_all(efd[0], sstderr); // ignore errors
    int status;
    pid_t ret = waitpid(pid, &status, WUNTRACED|WNOHANG);
    if (ret < 0) {
      close(infd[1]);
      close(outfd[0]);
      close(efd[0]);
      set_errno(error);
      return 1;
    }
    close(infd[1]);
    close(outfd[0]);
    close(efd[0]);
    return WEXITSTATUS(status);
  }
  return 1;
} // }}}

int ib::platform::show_context_menu(ib::oschar *path){ // {{{
  // TODO
  return 0;
} // }}}

void desktop_entry2command(ib::Command *cmd, const char *path) { // {{{
  static const char *SECTION_KEY = "Desktop Entry";
  if(!string_endswith(path, ".desktop")) return;
    FreeDesktopKVFile kvf(path);
    if(kvf.parse() < 0) return; // ignore errors;

    auto prop_type = kvf.get(SECTION_KEY, "Type");
    if(prop_type.empty()) return; // Type is a required value. ignore errors;

    auto prop_hidden = kvf.get(SECTION_KEY, "Hidden");
    if(prop_hidden == "true") {
      cmd->setName("/");
      return; // ignore this command. '/' is treated as a path, thus this command will never be shown in the completion lists.
    }

    auto prop_name = kvf.get(SECTION_KEY, "Name");
    if(prop_name.empty()) return; // Name is a required value. ignore errors;
    std::string name;
    ib::utils::to_command_name(name, prop_name);
    cmd->setName(name);

    auto prop_comment = kvf.get(SECTION_KEY, "Comment");
    if(!prop_comment.empty()){
      std::string normalized;
      normalize_desktop_entry_value(normalized, prop_comment);
      cmd->setDescription(normalized);
    }

    auto prop_icon = kvf.get(SECTION_KEY, "Icon");
    if(!prop_icon.empty()){
      std::string path;
      const char *theme = ib::Config::inst().getIconTheme().c_str();
      FreeDesktopThemeRepos::inst()->findIcon(path, theme, prop_icon.c_str(), 32);
      if(!path.empty()) {
        cmd->setIconFile(path);
      }
    }

    if(prop_type == "Application") {
      auto prop_path = kvf.get(SECTION_KEY, "Path");
      if(!prop_path.empty()){
        cmd->setWorkdir(prop_path);
      }
      auto prop_exec = kvf.get(SECTION_KEY, "Exec");
      if(!prop_exec.empty()) {
        std::vector<ib::unique_oschar_ptr> cmdline;
        std::string command;
        parse_cmdline(cmdline, prop_exec.c_str());
        if(cmdline.size() != 0) {
          if(!string_startswith(cmdline.at(0).get(), "/")) {
              ib::oschar abs[IB_MAX_PATH];
              if(ib::platform::which(abs, cmdline.at(0).get())) {
                cmd->setCommandPath(abs);
              }
          } else {
            cmd->setCommandPath(cmdline.at(0).get());
          }
          ib::oschar quoted[IB_MAX_PATH];
          auto it = cmdline.begin();
          it++;
          int argc = 0;
          for(auto last = cmdline.end(); it != last; ++it){
            command += " ";
            const ib::oschar *v = it->get();
            if(strcmp(v, "%f") == 0 || strcmp(v, "%F") == 0 || strcmp(v, "%u") == 0 ||
               strcmp(v, "%U") == 0){
              snprintf(quoted, IB_MAX_PATH, "${%d}",  ++argc);
            } else if(strcmp(v, "%i") == 0 && !prop_icon.empty()) {
              ib::platform::quote_string(quoted, prop_icon.c_str());
            } else if(strcmp(v, "%c") == 0) {
              ib::platform::quote_string(quoted, prop_name.c_str());
            } else if(strcmp(v, "%k") == 0) {
              ib::platform::quote_string(quoted, path);
            }else{
              ib::platform::quote_string(quoted, v);
            }
            command += quoted;
          }
        }
        cmd->setPath(command);
      }
    } else if(prop_type == "Directory") {
      // TODO
    } else if(prop_type == "Link") {
      // TODO
    }
} // }}}

void ib::platform::on_command_init(ib::Command *cmd) { // {{{
  const char *path = cmd->getCommandPath().c_str();

  if(string_endswith(path, ".desktop")){
    desktop_entry2command(cmd, path);
  }
} // }}}

ib::oschar* ib::platform::default_config_path(ib::oschar *result) { // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  snprintf(result, IB_MAX_PATH, "%s/%s", getenv("HOME"), ".iceberg");
  return result;
} // }}}

//////////////////////////////////////////////////
// path functions {{{
//////////////////////////////////////////////////
Fl_RGB_Image* ib::platform::get_associated_icon_image(const ib::oschar *path, const int size){ // {{{
  auto repos = FreeDesktopThemeRepos::inst();
  std::string iconpath;
  bool isdir = ib::platform::directory_exists(path);
  const char *theme = ib::Config::inst().getIconTheme().c_str();

  if(isdir) {
    repos->findIcon(iconpath, theme, "folder", size);
  }

  if(iconpath.empty() && string_endswith(path, ".desktop")){
    FreeDesktopKVFile kvf(path);
    if(kvf.parse() == 0) {
      auto prop_icon = kvf.get("Desktop Entry", "Icon");
      repos->findIcon(iconpath, theme, prop_icon.c_str(), size);
    }
  }

  if(iconpath.empty()){
    std::string mtype, msubtype;
    if(FreeDesktopMime::inst()->findByPath(mtype, msubtype, path)) {
      repos->findIcon(iconpath, theme, msubtype.c_str(), size);
      if(iconpath.empty()) {
        repos->findIcon(iconpath, theme, (mtype + "-" + msubtype).c_str(), size);
        if(iconpath.empty()) {
          repos->findIcon(iconpath, theme, ("gnome-mime-" + mtype + "-" + msubtype).c_str(), size);
        }
      }
    }
  }

  if(iconpath.empty() && !isdir){
    repos->findIcon(iconpath, theme, "misc", size);
  }

  if(!iconpath.empty() && ib::platform::file_exists(iconpath.c_str())) {
    return ib::IconManager::inst()->readFileIcon(iconpath.c_str(), size);
  }
  return 0;
} /* }}} */

ib::oschar* ib::platform::join_path(ib::oschar *result, const ib::oschar *parent, const ib::oschar *child) { // {{{
  if(result == 0){
    result = new ib::oschar[IB_MAX_PATH];
  }
  std::size_t length = strlen(parent);
  const ib::oschar *sep;
  if(parent[length-1] != '/') {
    sep = "/";
  }else{
    sep = "";
  }
  const ib::oschar *c = child;
  if(string_startswith(child, "./")) {
    c += 2;
  }
  snprintf(result, IB_MAX_PATH, "%s%s%s", parent, sep, c);
  return result;
} // }}}

ib::oschar* ib::platform::normalize_path(ib::oschar *result, const ib::oschar *path){ // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  result[0] = '\0';
  ib::oschar tmp[IB_MAX_PATH];
  bool is_dot_path = string_startswith(path, "./") || strcmp(path, ".") == 0;
  bool is_dot_dot_path = string_startswith(path, "../") || strcmp(path, "..") == 0;
  strncpy_s(tmp, path, IB_MAX_PATH);
  if(is_dot_path){
    tmp[0] = '_';
  }else if(is_dot_dot_path){
    tmp[0] = '_'; tmp[1] = '_';
  }
  ib::Regex sep("/", ib::Regex::I);
  sep.init();
  std::vector<std::string*> parts;
  std::vector<std::string*>  st;
  sep.split(parts, tmp);
  for(auto part : parts) {
    if(*part == ".." && st.size() > 0){
      st.pop_back();
    } else {
      st.push_back(part);
    }
  }
  for(auto part : st) {
    strcat(result, part->c_str());
    strcat(result, "/");
  }
  if(!string_endswith(path, "/")) {
    result[strlen(result)-1] = '\0';
  }
  if(is_dot_path){
    result[0] = '.';
  }else if(is_dot_dot_path){
    result[0] = '.'; result[1] = '.';
  }
  ib::utils::delete_pointer_vectors(parts);
  return result;
} // }}}

ib::oschar* ib::platform::normalize_join_path(ib::oschar *result, const ib::oschar *parent, const ib::oschar *child){ // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  ib::oschar tmp[IB_MAX_PATH];
  ib::platform::join_path(tmp, parent, child);
  ib::platform::normalize_path(result, tmp);
  return result;
} // }}}

ib::oschar* ib::platform::dirname(ib::oschar *result, const ib::oschar *path){ // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  strncpy_s(result, path, IB_MAX_PATH);
  const std::size_t len = strlen(path);
  if(len == 0) return result;
  std::size_t i = len -1;
  for(; i > 0; --i){
    if(result[i] == '/'){
      break;
    }
  }
  result[i] = '\0';
  if(i == 0) {
    strncpy_s(result, "/", IB_MAX_PATH);
  }
  return result;
} // }}}

ib::oschar* ib::platform::basename(ib::oschar *result, const ib::oschar *path){ // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  ib::oschar tmp[IB_MAX_PATH];
  strncpy_s(tmp, path, IB_MAX_PATH);
  const char *basename = fl_filename_name(tmp);
  strncpy_s(result, basename, IB_MAX_PATH);
  return result;
} // }}}

ib::oschar* ib::platform::to_absolute_path(ib::oschar *result, const ib::oschar *dir, const ib::oschar *path) { // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  if(ib::platform::is_relative_path(path)){
    ib::platform::normalize_join_path(result, dir, path);
  } else {
    strncpy_s(result, path, IB_MAX_PATH);
  }
  return result;
} // }}}

bool ib::platform::is_directory(const ib::oschar *path) { // {{{
  return (string_endswith(path, "/") || fl_filename_isdir(path) != 0);
} // }}}

bool ib::platform::is_path(const ib::oschar *str){ // {{{
  return string_startswith(str, "/") || string_startswith(str, "./") || string_startswith(str, "../");
} // }}}

bool ib::platform::is_relative_path(const ib::oschar *path) { // {{{
  const std::size_t length = strlen(path);
  bool is_dot_path = length > 1 && path[0] == '.' && path[1] == '/';
  if(!is_dot_path) is_dot_path = length == 1 && path[0] == '.';
  bool is_dot_dot_path = length > 2 && path[0] == '.' && path[1] == '.' && path[2] == '/';
  if(!is_dot_dot_path) is_dot_dot_path = length == 2 && path[0] == '.' && path[1] == '.';
  return is_dot_path || is_dot_dot_path;
} // }}}

bool ib::platform::directory_exists(const ib::oschar *path) { // {{{
  return fl_filename_isdir(path);
} // }}}

bool ib::platform::file_exists(const ib::oschar *path) { // {{{
  return ib::platform::path_exists(path) && !ib::platform::directory_exists(path);
} // }}}

bool ib::platform::path_exists(const ib::oschar *path) { // {{{
  return access(path, F_OK) == 0;
} // }}}

int ib::platform::walk_dir(std::vector<ib::unique_oschar_ptr> &result, const ib::oschar *dir, ib::Error &error, bool recursive) { // {{{
    DIR *d = opendir(dir);
    if (!d) {
      set_errno(error);
      return 1;
    }
    while (1) {
        struct dirent * entry = readdir(d);
        if (!entry) break;
        const char *d_name = entry->d_name;
        if (entry->d_type & DT_DIR) {
            if (strcmp (d_name, "..") != 0 &&
                strcmp (d_name, ".") != 0) {
                ib::oschar *path = new ib::oschar[IB_MAX_PATH];
                if(recursive){
                  snprintf(path, IB_MAX_PATH, "%s/%s", dir, d_name);
                } else {
                  snprintf(path, IB_MAX_PATH, "%s", d_name);
                }
                result.push_back(ib::unique_oschar_ptr(path));
                if(recursive) {
                  ib::platform::walk_dir(result, path, error, recursive);
                }
            }
        } else {
          ib::oschar *path = new ib::oschar[IB_MAX_PATH];
          if(recursive){
            snprintf(path, IB_MAX_PATH, "%s/%s", dir, d_name);
          } else {
            snprintf(path, IB_MAX_PATH, "%s", d_name);
          }
          result.push_back(ib::unique_oschar_ptr(path));
        }
    }
    if(closedir(d)){
      // ignore errors...
    }
    std::sort(result.begin(), result.end(), [](const ib::unique_oschar_ptr &left, const ib::unique_oschar_ptr &right) {
        return strcmp(left.get(), right.get()) < 0;
    });

    return 0;
} // }}}

ib::oschar* ib::platform::get_self_path(ib::oschar *result){ // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  char buf[64];
  snprintf(buf, 64, "/proc/%d/exe", getpid());
  std::size_t ret = readlink(buf, result, IB_MAX_PATH-1);
  if(ret < 0) {
    fl_alert("Failed to get self path.");
  }
  result[ret] = '\0';
  return result;
} // }}}

ib::oschar* ib::platform::get_current_workdir(ib::oschar *result){ // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  if(NULL == getcwd(result, IB_MAX_PATH-1)) {
    // ignore errors;
  }
  return result;
} // }}}

int ib::platform::set_current_workdir(const ib::oschar *dir, ib::Error &error){ // {{{
  if(chdir(dir)!= 0) {
    set_errno(error);
    return -1;
  }
  return 0;
} // }}}

bool ib::platform::which(ib::oschar *result, const ib::oschar *name) { // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  char *filename = result;
  const char* program = name;
  std::size_t filesize = IB_MAX_PATH;

  // code from fl_open_uri.cxx
  const char *path;
  char       *ptr,
             *end;
  if ((path = getenv("PATH")) == NULL) path = "/bin:/usr/bin";
  for (ptr = filename, end = filename + filesize - 1; *path; path ++) {
    if (*path == ':') {
      if (ptr > filename && ptr[-1] != '/' && ptr < end) *ptr++ = '/';
      strncpy_s(ptr, program, end - ptr + 1);
      if (!access(filename, X_OK)) return true;
      ptr = filename;
    } else if (ptr < end) *ptr++ = *path;
  }
  if (ptr > filename) {
    if (ptr[-1] != '/' && ptr < end) *ptr++ = '/';
    strncpy_s(ptr, program, end - ptr + 1);
    if (!access(filename, X_OK)) return true;
  }
  return false;
} // }}}

ib::oschar* ib::platform::icon_cache_key(ib::oschar *result, const ib::oschar *path) { // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  ib::oschar file_type[IB_MAX_PATH];
  ib::platform::file_type(file_type, path);
  if(ib::platform::directory_exists(path)) {
    strncpy_s(result, ":folder:common:", IB_MAX_PATH);
  } else if(string_endswith(path, ".desktop")) {
    strncpy_s(result, path, IB_MAX_PATH);
  } else if(strlen(file_type) > 0) {
    snprintf(result, IB_MAX_PATH, ":filetype:%s", file_type);
  } else if(access(path, X_OK) == 0) {
    snprintf(result, IB_MAX_PATH, ":filetype:executable");
  } else {
    snprintf(result, IB_MAX_PATH, ":filetype:file");
  }
  return result;
} // }}}

//////////////////////////////////////////////////
// path functions }}}
//////////////////////////////////////////////////

//////////////////////////////////////////////////
// filesystem functions {{{
//////////////////////////////////////////////////
int ib::platform::remove_file(const ib::oschar *path, ib::Error &error){ // {{{
  if(remove(path) < 0) {
    set_errno(error);
    return -1;
  }
  return 0;
} // }}}

int ib::platform::copy_file(const ib::oschar *source, const ib::oschar *dest, ib::Error &error){ // {{{
  int fdin = -1, fdout = -1, ret = 0;
  off_t ncpy = 0;
  struct stat fileinfo = {0};

  if((fdin = open(source, O_RDONLY)) < 0){
    ret = -1;
    goto finally;
  }
  if(fstat(fdin, &fileinfo) < 0) {
    ret = -1;
    goto finally;
  }

  if((fdout = open(dest,  O_WRONLY|O_TRUNC| O_CREAT, fileinfo.st_mode & 07777)) < 0){
    ret = -1;
    goto finally;
  }
  if(sendfile(fdout, fdin, &ncpy, fileinfo.st_size) < 0) {
    ret = -1;
    goto finally;
  }

finally:
  if(fdin >= 0) close(fdin);
  if(fdout >= 0) close(fdout);
  if(ret < 0){
    set_errno(error);
  }
  return ret;
} // }}}

int ib::platform::file_size(size_t &size, const ib::oschar *path, ib::Error &error){ // {{{
  struct stat st;
  if(stat(path, &st) < 0) {
    set_errno(error);
    return -1;
  }
  size = (size_t)st.st_size;
  return 0;
} // }}}

ib::oschar* ib::platform::file_type(ib::oschar *result, const ib::oschar *path){ // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  const char *dot = strrchr(path, '.');
  if(!dot || dot == path) return result;
  strcpy(result, dot);
  return result;
} // }}}
//////////////////////////////////////////////////
// filesystem functions }}}
//////////////////////////////////////////////////

//////////////////////////////////////////////////
// thread functions {{{
//////////////////////////////////////////////////
void ib::platform::create_thread(ib::thread *t, ib::threadfunc f, void* p) { // {{{
  pthread_create(t, NULL, f, p);
}
/* }}} */

void ib::platform::on_thread_start(){ /* {{{ */
} /* }}} */

void ib::platform::join_thread(ib::thread *t){ /* {{{ */
  pthread_join(*t, NULL);
} /* }}} */

void ib::platform::exit_thread(int exit_code) { // {{{
  // nothing to do
} // }}}

void ib::platform::create_mutex(ib::mutex *m) { /* {{{ */
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr); 
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
  int ret = pthread_mutex_init(m, &attr);
  if(ret == 0) {
    m = 0;
  }
} /* }}} */

void ib::platform::destroy_mutex(ib::mutex *m) { /* {{{ */
  // nothing to do
} /* }}} */

void ib::platform::lock_mutex(ib::mutex *m) { /* {{{ */
  pthread_mutex_lock(m);
} /* }}} */

void ib::platform::unlock_mutex(ib::mutex *m) { /* {{{ */
  pthread_mutex_unlock(m);
} /* }}} */

void ib::platform::create_cmutex(ib::cmutex *m) { /* {{{ */
  *m = PTHREAD_MUTEX_INITIALIZER;
} /* }}} */

void ib::platform::destroy_cmutex(ib::cmutex *m) { /* {{{ */
  // nothing to do
} /* }}} */

void ib::platform::lock_cmutex(ib::cmutex *m) { /* {{{ */
  pthread_mutex_lock(m);
} /* }}} */

void ib::platform::unlock_cmutex(ib::cmutex *m) { /* {{{ */
  pthread_mutex_unlock(m);
} /* }}} */

void ib::platform::create_condition(ib::condition *c) { /* {{{ */
  pthread_cond_init(c, NULL);
} /* }}} */

void ib::platform::destroy_condition(ib::condition *c) { /* {{{ */
  pthread_cond_destroy(c);
} /* }}} */

int ib::platform::wait_condition(ib::condition *c, ib::cmutex *m, int ms) { /* {{{ */
  if(ms == 0) return pthread_cond_wait(c, m);
  struct timespec tv;
  tv.tv_sec  = time(NULL) + ms/1000;
  tv.tv_nsec = ms%1000 * 1000000;
  return pthread_cond_timedwait(c, m, &tv);
} /* }}} */

void ib::platform::notify_condition(ib::condition *c) { /* {{{ */
  pthread_cond_signal(c);
} /* }}} */

//////////////////////////////////////////////////
// thread functions }}}
//////////////////////////////////////////////////

//////////////////////////////////////////////////
// process functions {{{
//////////////////////////////////////////////////
int ib::platform::wait_pid(const int pid) { // {{{
  for(int limit = 0; limit < 10; limit++){
    if(kill(pid, 0) < 0 && errno == ESRCH) {
      return 0;
    }
    struct timespec ts = {0};
    ts.tv_nsec = 500 * 1000000;
    nanosleep(&ts, 0);
  }
  return -1;
} // }}}

int ib::platform::get_pid() { // {{{
  return (int)getpid();
} // }}}

//////////////////////////////////////////////////
// process functions }}}
//////////////////////////////////////////////////

//////////////////////////////////////////////////
// dynamic loading functions {{{
//////////////////////////////////////////////////
int ib::platform::load_library(ib::module &dl, const ib::oschar *name, ib::Error &error) { // {{{
  std::string lib;
  lib += "lib";
  lib += name;
  lib += ".so";
  dl = dlopen(lib.c_str(), RTLD_LAZY);
  if(dl == 0) {
    error.setMessage(dlerror());
    error.setCode(1);
    return 1;
  }
  return 0;
} // }}}

void* ib::platform::get_dynamic_symbol(ib::module dl, const char *name){ //{{{
  return dlsym (dl, name);
} //}}}

void ib::platform::close_library(ib::module dl) { //{{{
  dlclose(dl);
} //}}}

//////////////////////////////////////////////////
// dynamic loading functions }}}
//////////////////////////////////////////////////

//////////////////////////////////////////////////
// socket functions {{{
//////////////////////////////////////////////////
void ib::platform::close_socket(FL_SOCKET s){ // {{{
  close(s);
} // }}}
//////////////////////////////////////////////////
// socket functions }}}
//////////////////////////////////////////////////

//////////////////////////////////////////////////
// system functions {{{
//////////////////////////////////////////////////
int ib::platform::get_num_of_cpu(){ // {{{
  return (int)sysconf(_SC_NPROCESSORS_ONLN);
} // }}}

int ib::platform::convert_keysym(int key){ // {{{
  // end of composition. converts original keysym to a harmless keysym.
  if(key == 0xff0d) return (int)'_';
  return key;
} // }}}

#ifndef IB_OS_WIN
        // end of xim composition. change the keycode to a harmless dummy code.
#endif
//////////////////////////////////////////////////
// system functions }}}
//////////////////////////////////////////////////

//////////////////////////////////////////////////
// platform specific functions {{{
//////////////////////////////////////////////////
void ib::platform::move_to_current_desktop(Fl_Window *w) {
  int desktop = xcurrent_desktop();
  if(desktop >= 0) {
    xsend_message_l(fl_xid(w), "_NET_WM_DESKTOP", desktop,0,0,0,0);
  }
}
//////////////////////////////////////////////////
// platform specific functions }}}
//////////////////////////////////////////////////
