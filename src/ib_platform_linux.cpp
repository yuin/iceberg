#include "ib_constants.h"
#include "ib_platform.h"
#include "ib_resource.h"
#include "ib_utils.h"
#include "ib_ui.h"
#include "ib_controller.h"
#include "ib_config.h"
#include "ib_regex.h"
#include "ib_comp_value.h"
#include "ib_icon_manager.h"
#include "ib_server.h"
#include "ib_singleton.h"

#define SYSTEM_TRAY_REQUEST_DOCK 0

static char ib_g_lang[32];
static int  ib_g_hotkey;
static const int XERROR_MSG_SIZE = 1024;
static char ib_g_xerror_msg[XERROR_MSG_SIZE];
class TrayIcon;
typedef struct atoms_ {
  Atom xembed_info;
  Atom xembed;
  Atom net_wm_window_opacity;
  Atom net_wm_desktop;
  Atom net_system_tray_opecode;
  Atom net_active_window;
} atoms;
static atoms ib_g_atoms;

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
  const auto dot = strrchr(str, '.');
  return (dot && !strcmp(dot, pre));
}

static std::string parse_desktop_entry_value(const std::string &value) {
  ib::Regex re("(\\\\s|\\\\n|\\\\t|\\\\r|\\\\\\\\)", ib::Regex::NONE);
  re.init();
  auto result = re.gsub(value, [](const ib::Regex &reg, std::string *res, void *userdata) {
      auto escape = reg._1();
      if(escape == "\\s") *res += " ";
      else if(escape == "\\n") *res += "\n";
      else if(escape == "\\t") *res += "\t";
      else if(escape == "\\r") *res += "\r";
      else if(escape == "\\\\") *res += "\\";
  }, nullptr);
  return result;
}

static std::string normalize_desktop_entry_value(const std::string &value) {
  ib::Regex re("(\\n|\\r|\\t)", ib::Regex::NONE);
  re.init();
  auto result = re.gsub(value, [](const ib::Regex &reg, std::string *res, void *userdata) {
      auto escape = reg._1();
      if(escape == "\t") *res += "    ";
  }, nullptr);
  return result;
}

static int read_fd_all(int fd, std::string &out) {
  char buf[1024];
  do{
    auto ret = read(fd, buf, 1023);
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

static int parse_cmdline(std::vector<std::unique_ptr<char[]>> &result, const char *cmdline) {
  ib::Regex space_r("\\s+", ib::Regex::I);
  ib::Regex string_r("\"((((?<=\\\\)\")|[^\"])*)((?<!\\\\)\")", ib::Regex::I);
  ib::Regex value_r("([^\\(\\[\\)\\]\\s\"]+)", ib::Regex::I);
  space_r.init();
  string_r.init();
  value_r.init();
  std::size_t pos = 0;
  auto len = strlen(cmdline);
  while(pos < len) {
    if(space_r.match(cmdline, pos) == 0) {
      pos = space_r.getEndpos(0) + 1;
    } else if(string_r.match(cmdline, pos) == 0) {
      auto cap = string_r._1();
      auto buf = new char[cap.length()+1];
      strcpy(buf, cap.c_str());
      result.push_back(std::unique_ptr<char[]>(buf));
      pos = string_r.getEndpos(0) + 1;
    } else if(value_r.match(cmdline, pos) == 0) {
      auto cap = value_r._1();
      auto buf = new char[cap.length()+1];
      strcpy(buf, cap.c_str());
      result.push_back(std::unique_ptr<char[]>(buf));
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
    ib::Regex re("([^;]+)(;.*)?", ib::Regex::NONE);
    re.init();
    if(re.match(out) == 0) {
      result = re._1();
      return true;
    }
  }
  result.clear();
  return false;
}


static bool xdg_mime_default_app(std::string &result, const char *mime, ib::Error &e) {
  std::string out, err;
  if(ib::platform::command_output(out, err, ("xdg-mime query default " + std::string(mime)).c_str(), e) == 0) {
    result = out;
    return true;
  }
  result.clear();
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
          current_section = section_reg._1();
        } if(kv_reg.match(line.c_str()) == 0) {
          if(current_section.empty()) {
            return -1;
          }
          auto key = kv_reg._1();
          auto raw_value = kv_reg._2();
          auto value = parse_desktop_entry_value(raw_value);
          map_[std::make_tuple(current_section, key)] = value;
        }
      }
      auto locale = getenv("LC_MESSAGES");
      if(locale == nullptr) locale = getenv("LANGUAGE");
      if(locale == nullptr) locale = getenv("LANG");
      if(locale != nullptr) {
        ib::Regex locale_reg("(([^\\._@]+)((\\.)([^_]*))?)(_(\\w+))?(@(\\w+))?", ib::Regex::I);
        locale_reg.init();
        if(locale_reg.match(locale) == 0){
          lang_ = locale_reg._2();
          country_ = locale_reg._7();
          modifier_ = locale_reg._9();
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

FreeDesktopMime* FreeDesktopMime::instance_ = nullptr;

void FreeDesktopMime::build() {
  const char *files[] = {"/usr/share/mime/globs2", "/usr/share/mime/globs"};
  for(int i = 0; i < 2; i++) {
    const auto file = files[i];
    std::ifstream ifs(file);
    std::string   line;
    if (ifs.fail()) {
      continue;
    }
    ib::Regex reg("\\s*(?:([0-9]+):)?([^:/]+)/([^:/]+):(.*)", ib::Regex::I);
    reg.init();

    while(std::getline(ifs, line)) {
      if(reg.match(line) == 0) {
        auto scorestr = reg._1();
        auto typ      = reg._2();
        auto subtyp   = reg._3();
        auto glob     = reg._4();
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
  for(const auto &tup : globs_){
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
  const auto theme = ib::Singleton<ib::Config>::getInstance()->getIconTheme().c_str();
  findHelper(result, theme);
  if(!result.empty()) return;
  findHelper(result, "default");
  if(!result.empty()) return;
  findHelper(result, "Hicolor");
  if(!result.empty()) return;

  const char *pixmaps = "/usr/share/pixmaps";
  char icon_file[IB_MAX_PATH];
  const char *exts[] = {"png", "svg", "xpm"};
  for(int i = 0; i < 3; i++) {
    const auto ext = exts[i];
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
  if(kvf == nullptr) return;
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
  if(kvf == nullptr) return;
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
        const auto ext = exts[i];
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
      const auto ext = exts[i];
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
  const auto kvf = FreeDesktopThemeRepos::inst()->getTheme(theme);
  if(kvf != nullptr) {
    typ = kvf->get(dir, "Type", false);
    if(typ.empty()) typ = "Threshold";
    size = std::atoi(kvf->get(dir, "Size", false).c_str());
    min = size;
    max = size;
    const auto minstr = kvf->get(dir, "MinSize", false);
    const auto maxstr = kvf->get(dir, "MaxSize", false);
    if(!minstr.empty()) min = std::atoi(minstr.c_str());
    if(!maxstr.empty()) max = std::atoi(maxstr.c_str());
    th = 2;
    const auto thstr = kvf->get(dir, "Threshold", false);
    if(!thstr.empty()) th = std::atoi(thstr.c_str());
  }

  return std::make_tuple(typ, size, min, max, th);

}

bool FreeDesktopIconFinder::directoryMatchSize(const char *dir, const char *theme) {
  const auto sizedata = getDirectorySizes(dir, theme);
  const auto typ = std::get<0>(sizedata);
  const auto size = std::get<1>(sizedata);
  const auto min = std::get<2>(sizedata);
  const auto max = std::get<3>(sizedata);
  const auto th  = std::get<4>(sizedata);

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
  const auto sizedata = getDirectorySizes(dir, theme);
  const auto &typ = std::get<0>(sizedata);
  const auto size = std::get<1>(sizedata);
  const auto min = std::get<2>(sizedata);
  const auto max = std::get<3>(sizedata);
  const auto th  = std::get<4>(sizedata);
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

FreeDesktopThemeRepos *FreeDesktopThemeRepos::instance_ = nullptr;

void FreeDesktopThemeRepos::build() {
  char path[IB_MAX_PATH];
  snprintf(path, IB_MAX_PATH, "%s/.icons", getenv("HOME"));
  buildHelper(path);

  snprintf(path, IB_MAX_PATH, "/usr/share/icons");
  buildHelper(path);

  if(getenv("XDG_DATA_DIRS") != nullptr) {
    ib::Regex sep(":", ib::Regex::I);
    sep.init();
    auto parts = sep.split(getenv("XDG_DATA_DIRS"));
    for(const auto &part : parts) {
      snprintf(path, IB_MAX_PATH, "%s/icons", part.c_str());
      buildHelper(path);
    }
  }

  snprintf(path, IB_MAX_PATH, "/usr/share/pixmaps");
  buildHelper(path);
}

void FreeDesktopThemeRepos::buildHelper(const char *basepath) {
  char path[IB_MAX_PATH];
  char index_path[IB_MAX_PATH];
  ib::Error error;
  std::vector<std::unique_ptr<ib::oschar[]>> files;

  if(ib::platform::walk_dir(files, basepath, error, false) != 0) {
    return; //ignore errors
  }
  for(const auto &f : files) {
    snprintf(path, IB_MAX_PATH, "%s/%s", basepath, (const char*)(f.get()));
    if(ib::platform::is_directory(path)) {
      snprintf(index_path, IB_MAX_PATH, "%s/%s", path, "index.theme");
      if(ib::platform::file_exists(index_path)) {
        auto kvf = new FreeDesktopKVFile(index_path);
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
  if(themeptr == indicies_.end()) return nullptr;
  return themeptr->second.get();
}


void FreeDesktopThemeRepos::findIcon(std::string &result, const char *theme, const char *name, int size) {

  std::string sname = std::string(name);
  const char *exts[] = {"png", "svg", "xpm"};
  for(int i = 0; i < 3; i++) {
    std::string ext = ".";
    ext += exts[i];
    if(ext.size() > sname.size()) continue;
    if(std::equal(ext.rbegin(), ext.rend(), sname.rbegin())) {
      result = name;
      return;
    }
  }

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
  snprintf(ib_g_xerror_msg, XERROR_MSG_SIZE, "xerror: %s(%d): %s 0x%lx\n", buf2, e->error_code, buf1, e->resourceid);
  return 0;
}

static void xclear_error() { ib_g_xerror_msg[0] = '\0'; }

static bool xhas_error() { return ib_g_xerror_msg[0] != '\0'; }

static void xsend_message_la(Window w, Atom a, long d0, long d1, long d2, long d3, long d4) {
  XEvent event = {0};
  long mask = SubstructureRedirectMask | SubstructureNotifyMask;
  event.xclient.type = ClientMessage;
  event.xclient.serial = 0;
  event.xclient.send_event = True;
  event.xclient.window = w;
  event.xclient.message_type = a;
  event.xclient.format = 32;
  event.xclient.data.l[0] = d0;
  event.xclient.data.l[1] = d1;
  event.xclient.data.l[2] = d2;
  event.xclient.data.l[3] = d3;
  event.xclient.data.l[4] = d4;
  xclear_error();
  XSendEvent(fl_display, DefaultRootWindow(fl_display), False, mask, &event);
  if(xhas_error()) {
    fprintf(stdout, "%s\n", ib_g_xerror_msg); fflush(stdout);
  }
  XSync(fl_display, False);
}

class TrayIcon : public Fl_Double_Window {
  private:
    uint32_t parent_xid_;
    Fl_Box*  box_;

    void replaceXid() {
      Colormap colormap = fl_colormap;
      XSetWindowAttributes attr;
      attr.border_pixel = None;
      attr.background_pixel = None;
      attr.colormap = colormap;
      attr.bit_gravity = 0;
      attr.backing_store = Always;
      attr.override_redirect = 1;
      attr.event_mask = ExposureMask | StructureNotifyMask | KeyPressMask
        | KeyReleaseMask | KeymapStateMask | FocusChangeMask | ButtonPressMask
        | ButtonReleaseMask | EnterWindowMask | LeaveWindowMask | PointerMotionMask;

      Fl_X::set_xid(this,
        XCreateWindow(fl_display, parent_xid_, 0, 0, 1, 1 ,
          0, fl_visual->depth, InputOutput, fl_visual->visual,
          CWBorderPixel|CWBackPixel|CWColormap|CWEventMask|CWBitGravity|CWBackingStore|CWOverrideRedirect , &attr));
    };

    void setXembedInfo(unsigned long flags) {
     unsigned long buffer[2] = {1, flags};
     XChangeProperty (fl_display, fl_xid(this),
       ib_g_atoms.xembed_info, ib_g_atoms.xembed_info, 32, PropModeReplace,
       (unsigned char *)buffer, 2);
    };

  public:
    TrayIcon() : Fl_Double_Window(64, 64), parent_xid_(0), box_(nullptr) {}
    void init(Window xid) {
      parent_xid_ = xid;
      end();
      replaceXid();
      setXembedInfo(1);
      copy_tooltip("iceberg - click to restore");
      begin();
        box_ = new Fl_Box(0,0,1,1);
      end();
    };
    ~TrayIcon() {
      if(box_) {
        if(box_->image()) delete box_->image();
        delete box_;
      }
    }
    int handle(int e) {
      if(e == FL_PUSH) {
       ib::Singleton<ib::Controller>::getInstance()->showApplication();
       return 1;
      }
      return Fl_Window::handle(e);
    }
    uint32_t getParentXid() const { return parent_xid_; }
    Fl_Box*  getBox() const { return box_; }
};

static int event_handler(int e, Fl_Window* w) {
  if(fl_xevent->type == ClientMessage) {
    if (fl_xevent->xclient.message_type == ib_g_atoms.xembed) {
      long message = fl_xevent->xclient.data.l[1];
      auto icon = ib::Singleton<TrayIcon>::getInstance();
      if(message == 0) { // XEMBED_EMBEDDED_NOTIFY
         auto xid = fl_xevent->xclient.data.l[3];
         XWindowAttributes attr;
         XGetWindowAttributes(fl_display, xid, &attr);
         auto size = std::max(attr.width, attr.height);
         auto box = icon->getBox();
         auto iconmanager = ib::Singleton<ib::IconManager>::getInstance();
         icon->resize(0,0,size, size);
         box->resize(0,0,size,size);
         box->box(FL_NO_BOX);
         icon->shape(iconmanager->getIcebergIcon(size));
         if(box->image()) delete box->image();
         box->image(iconmanager->getIcebergIcon(size));
      } else if(message == 1) { //XEMBED_WINDOW_ACTIVATE
        if (w) {
          w->resize(0,0, w->w(), w->h());
        }
      }
    }
  }
  return Fl::handle_(e, w);
};

static int xevent_handler(int e){
  //Window window = fl_xevent->xany.window;
  switch(fl_xevent->type){
    case KeyPress: // hotkey
      ib::Singleton<ib::Controller>::getInstance()->showApplication();
      return 1;
  }
  return 0;
}

static int xregister_hotkey() {
  auto root = RootWindow(fl_display, fl_screen);
  const auto hot_key = ib::Singleton<ib::Config>::getInstance()->getHotKey();
  for(int i=0 ;hot_key[i] != 0; ++i){
    ib_g_hotkey += hot_key[i];
  }
  const auto keycode = XKeysymToKeycode(fl_display, ib_g_hotkey & 0xFFFF);
  if(!keycode) {
    return -1;
  }
  XGrabKey(fl_display, keycode, ib_g_hotkey >>16,     root, 1, GrabModeAsync, GrabModeAsync);
  XGrabKey(fl_display, keycode, (ib_g_hotkey>>16)|2,  root, 1, GrabModeAsync, GrabModeAsync); // CapsLock
  XGrabKey(fl_display, keycode, (ib_g_hotkey>>16)|16, root, 1, GrabModeAsync, GrabModeAsync); // NumLock
  XGrabKey(fl_display, keycode, (ib_g_hotkey>>16)|18, root, 1, GrabModeAsync, GrabModeAsync); // both
  return 0;
}

static int xregister_systray_icon() {
  char atom_tray_name[128];
  sprintf(atom_tray_name, "_NET_SYSTEM_TRAY_S%i", fl_screen);
  Atom sys_tray_atom = XInternAtom(fl_display, atom_tray_name, False);
  Window dock = XGetSelectionOwner(fl_display, sys_tray_atom);
  // if(!dock) {
  //   // no system tray found.
  //   return 0;
  // }
  auto mainwin = ib::Singleton<ib::MainWindow>::getInstance();
  auto trayicon = ib::Singleton<TrayIcon>::initInstance();
  trayicon->init(fl_xid(mainwin));
  trayicon->show();
  trayicon->wait_for_expose();
  xsend_message_la(dock, ib_g_atoms.net_system_tray_opecode, fl_event_time, SYSTEM_TRAY_REQUEST_DOCK, fl_xid(trayicon), 0, 0);
  XSync(fl_display, False);
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
      return nullptr;
  }
  if (aret != typ || n == 0) {
    return nullptr;
  }
  return ptr;
}

static int xcurrent_desktop() {
  auto desktop = reinterpret_cast<long*>(xget_property(DefaultRootWindow(fl_display), "_NET_CURRENT_DESKTOP", XA_CARDINAL));
  if(desktop == nullptr) {
    desktop = reinterpret_cast<long*>(xget_property(DefaultRootWindow(fl_display), "_WIN_WORKSPACE", XA_CARDINAL));
    if(desktop == nullptr) {
      return -1;
    }
  }
  auto ret = reinterpret_cast<int&>(desktop[0]);
  XFree(desktop);
  return ret;
}

static bool xis_gui_app(const char *path) {
  std::string out, err;
  ib::Error e;
  char quoted[IB_MAX_PATH];
  ib::platform::quote_string(quoted, path);
  if(ib::platform::command_output(out, err, ("ldd " + std::string(quoted)).c_str(), e) == 0) {
      std::istringstream stream(out);
      std::string   line;
      while(std::getline(stream, line)) {
        if(line.find("libX11") != std::string::npos) {
          return true;
        }
      }
  }
  return false;
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
  ib::Singleton<ib::ListWindow>::getInstance()->set_menu_window();
  ib::Singleton<ib::MessageBox>::getInstance()->set_menu_window();
  auto intern_atom = [](const char *msg) -> Atom { return XInternAtom(fl_display, msg, False); };
  ib_g_atoms.xembed = intern_atom("_XEMBED");
  ib_g_atoms.xembed_info = intern_atom("_XEMBED_INFO");
  ib_g_atoms.net_wm_window_opacity = intern_atom("_NET_WM_WINDOW_OPACITY");
  ib_g_atoms.net_wm_desktop = intern_atom("_NET_WM_DESKTOP");
  ib_g_atoms.net_system_tray_opecode = intern_atom("_NET_SYSTEM_TRAY_OPCODE");
  ib_g_atoms.net_active_window = intern_atom("_NET_ACTIVE_WINDOW");
  XAllowEvents(fl_display, AsyncKeyboard, CurrentTime);
  XkbSetDetectableAutoRepeat(fl_display, true, nullptr);

  Fl::event_dispatch(event_handler);
  Fl::add_handler(xevent_handler);

  if(xregister_hotkey() < 0 ) {
    ib::utils::message_box("%s", "Failed to register hotkeys.");
    ib::utils::exit_application(1);
  }

  if(xregister_systray_icon() < 0 ) {
    ib::utils::message_box("%s", "Failed to register a system tray icon.");
    ib::utils::exit_application(1);
  }

  // first call of get_clipboard sometime fails for unknown reasons.
  std::string tmp;
  ib::utils::get_clipboard(tmp);
  Fl::add_clipboard_notify([](int source, void *data){
    std::string tmp;
    ib::utils::get_clipboard(tmp);
  }, nullptr);

  XFlush(fl_display);

  ib::Error e;
  ib::Singleton<ib::Controller>::getInstance()->setCwd(getenv("HOME"), e);

  return 0;
} // }}}

void ib::platform::finalize_system(){ // {{{
  if(fl_display != 0) {
    auto root = XRootWindow(fl_display, fl_screen);
    const auto keycode = XKeysymToKeycode(fl_display, ib_g_hotkey & 0xFFFF);
    XUngrabKey(fl_display, keycode, ib_g_hotkey >>16,     root);
    XUngrabKey(fl_display, keycode, (ib_g_hotkey>>16)|2,  root); // CapsLock
    XUngrabKey(fl_display, keycode, (ib_g_hotkey>>16)|16, root); // NumLock
    XUngrabKey(fl_display, keycode, (ib_g_hotkey>>16)|18, root); // both
  }

  delete FreeDesktopThemeRepos::inst();
  delete FreeDesktopMime::inst();
} // }}}

void ib::platform::get_runtime_platform(char *ret){ // {{{
} // }}}

std::unique_ptr<ib::oschar[]> ib::platform::utf82oschar(const char *src) { // {{{
  auto buff = new char[strlen(src)+1];
  strcpy(buff, src);
  return std::unique_ptr<ib::oschar[]>(buff);
} // }}}

void ib::platform::utf82oschar_b(ib::oschar *buf, const unsigned int bufsize, const char *src) { // {{{
  strncpy_s(buf, src, bufsize);
} // }}}

std::unique_ptr<char[]> ib::platform::oschar2utf8(const ib::oschar *src) { // {{{
  auto buff = new char[strlen(src)+1];
  strcpy(buff, src);
  return std::unique_ptr<char[]>(buff);
} // }}}

void ib::platform::oschar2utf8_b(char *buf, const unsigned int bufsize, const ib::oschar *src) { // {{{
  strncpy_s(buf, src, bufsize);
} // }}}

std::unique_ptr<char[]> ib::platform::oschar2local(const ib::oschar *src) { // {{{
  auto buff = new char[strlen(src)+1];
  strcpy(buff, src);
  return std::unique_ptr<char[]>(buff);
} // }}}

void ib::platform::oschar2local_b(char *buf, const unsigned int bufsize, const ib::oschar *src) { // {{{
  strncpy_s(buf, src, bufsize);
} // }}}

std::unique_ptr<char[]> ib::platform::utf82local(const char *src) { // {{{
  auto buff = new char[strlen(src)+1];
  strcpy(buff, src);
  return std::unique_ptr<char[]>(buff);
} // }}}

std::unique_ptr<char[]> ib::platform::local2utf8(const char *src) { // {{{
  auto buff = new char[strlen(src)+1];
  strcpy(buff, src);
  return std::unique_ptr<char[]>(buff);
} // }}}

std::unique_ptr<ib::oschar[]> ib::platform::quote_string(ib::oschar *result, const ib::oschar *str) { // {{{
  bool ret_unique = false;
  if(result == nullptr){ result = new ib::oschar[IB_MAX_PATH]; ret_unique = true; }
  bool need_quote = false;
  bool all_space = true;

  for(const ib::oschar *tmp = str; *tmp != '\0'; tmp++) {
    if(!isspace(*tmp)) { all_space = false; break; }
  }

  if(!all_space) {
    for(const ib::oschar *tmp = str; *tmp != '\0'; tmp++) {
      if(isspace(*tmp)) { need_quote = true; break; }
    }
  }

  if((strlen(str) != 0 && str[0] == '"') || !need_quote || all_space) {
    strncpy_s(result, str, IB_MAX_PATH);
    return std::unique_ptr<ib::oschar[]>(ret_unique ? result: nullptr);
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
  cur++;
  *cur = '\0';

  return std::unique_ptr<ib::oschar[]>(ret_unique ? result: nullptr);
} // }}}

std::unique_ptr<ib::oschar[]> ib::platform::unquote_string(ib::oschar *result, const ib::oschar *str) { // {{{
  bool ret_unique = false;
  if(result == nullptr){ result = new ib::oschar[IB_MAX_PATH]; ret_unique = true;}
  auto len = strlen(str);
  if(len == 0) { result[0] = '\0'; return std::unique_ptr<ib::oschar[]>(ret_unique ? result: nullptr); }
  if(len == 1) { strcpy(result, str); return std::unique_ptr<ib::oschar[]>(ret_unique ? result: nullptr); }
  if(*str == '"') str++;
  strncpy_s(result, str, IB_MAX_PATH);
  len = strlen(result);
  if(result[len-1] == '"') {
    result[len-1] = '\0';
  }
  return std::unique_ptr<ib::oschar[]>(ret_unique ? result: nullptr);
} // }}}

void ib::platform::hide_window(Fl_Window *window){ // {{{
  window->clear_visible();
  XUnmapWindow(fl_display, fl_xid(window));
  // XIconifyWindow(fl_display, fl_xid(window), fl_screen);
  XFlush(fl_display);
} // }}}

static void ib_platform_remove_taskbar_icon(Fl_Window *window) { // {{{
    Window xid = fl_xid(window); 
    Display *display = fl_display;
    Atom wmState = XInternAtom(display, "_NET_WM_STATE", False);
    Atom skipTaskbar = XInternAtom(display, "_NET_WM_STATE_SKIP_TASKBAR", False);
    XChangeProperty(display, xid, wmState, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)&skipTaskbar, 1);
} // }}}

void ib::platform::activate_window(Fl_Window *window){ // {{{
  window->set_visible_focus();
  xsend_message_la(fl_xid(window), ib_g_atoms.net_active_window, 1, CurrentTime, 0,0,0);
  ib_platform_remove_taskbar_icon(window);
  XFlush(fl_display);
} // }}}

void ib::platform::raise_window(Fl_Window *window){ // {{{
  window->set_visible();
  // XRaiseWindow(fl_display, fl_xid(window));
  Atom wmHints = XInternAtom(fl_display, "WM_HINTS", True);
  if (wmHints != None) {
      XWMHints hints;
      hints.flags = InputHint;
      hints.input = False;
      XSetWMHints(fl_display, fl_xid(window), &hints);
  }
  ib_platform_remove_taskbar_icon(window);
  XMapRaised(fl_display, fl_xid(window));
} // }}}
  
void ib::platform::set_window_alpha(Fl_Window *window, int alpha){ // {{{
  auto visible = window->visible();
  if(!visible) window->show();
  double  a = alpha/255.0;
  uint32_t cardinal_alpha = (uint32_t) (a * (uint32_t)-1) ;
  XChangeProperty(fl_display, fl_xid(window), ib_g_atoms.net_wm_window_opacity,
                 XA_CARDINAL, 32, PropModeReplace, (uint8_t*)&cardinal_alpha, 1) ;
  if(!visible) window->hide();
} // }}}

static int ib_platform_shell_execute(const std::string &path, const std::string &strparams, const std::string &cwd, const std::string &terminal, bool sudo, ib::Error &error) { // {{{
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

  const auto server = ib::Singleton<ib::Server>::getInstance();

  // shutdown the server to prevent socket fd passing.
  server->shutdown();
  ib::Regex proto_reg("^(\\w+)://.*", ib::Regex::I);
  proto_reg.init();
  ret = 0;
  if(proto_reg.match(path) == 0){
    if(sudo) { cmd += "gksudo "; }
    cmd += "xdg-open ";
    cmd += path;
    cmd += " ";
    cmd += strparams;
  } else {
    std::string rpath;
    if(-1 == access(path.c_str(), R_OK)) {
      ib::oschar os_path[IB_MAX_PATH];
      ib::platform::utf82oschar_b(os_path, IB_MAX_PATH, path.c_str());
      ib::oschar tmp[IB_MAX_PATH];
      memcpy(tmp, os_path, sizeof(ib::oschar)*IB_MAX_PATH);
      memset(os_path, 0, sizeof(ib::oschar)*IB_MAX_PATH);
      if(!ib::platform::which(os_path, tmp)) {
        memcpy(os_path, tmp, sizeof(ib::oschar)*IB_MAX_PATH);
      }
      char p[IB_MAX_PATH_BYTE];
      ib::platform::oschar2utf8_b(p, IB_MAX_PATH_BYTE, os_path);
      rpath += p;
    } else {
      rpath += path.c_str();
    }
    if(-1 == access(rpath.c_str(), R_OK)) {
      set_errno(error);
      ret = -1;
      goto finally;
    }
    if(!strparams.empty() || access(rpath.c_str(), X_OK) == 0) {
      bool isterm = terminal == "yes" || (terminal == "auto" && !xis_gui_app(rpath.c_str()));
      if(sudo) { cmd += isterm ? "sudo " : "gksudo "; }
      cmd += quoted_path;
      if(!strparams.empty()){
        cmd += " ";
        cmd += strparams;
      }
      if(isterm) {
        ib::string_map values;
        cmd += ";";
        cmd += getenv("SHELL");
        values.insert(ib::string_pair("1", ("'" + cmd + "'")));
        cmd = ib::utils::expand_vars(ib::Singleton<ib::Config>::getInstance()->getTerminal(), values);
      }
    } else {
      std::string mime, app;
      xdg_mime_filetype(mime, quoted_path, error);
      if(!mime.empty()) {
        xdg_mime_default_app(app, mime.c_str(), error);
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
      if(sudo) { cmd += "gksudo "; }
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
  if(server->start(error) != 0) ret = -1;
  ib::platform::set_current_workdir(wd, error);
  return ret;
} // }}}

int ib::platform::shell_execute(const std::string &path, const std::vector<std::unique_ptr<std::string>> &params, const std::string &cwd, const std::string &terminal, bool sudo, ib::Error &error) { // {{{
  std::string strparams;
  for(const auto &p : params) {
    ib::oschar qparam[IB_MAX_PATH];
    strparams += " ";
    ib::platform::quote_string(qparam, p.get()->c_str());
    strparams += qparam;
  }
  return ib_platform_shell_execute(path, strparams, cwd, terminal, sudo, error);
} /* }}} */

int ib::platform::shell_execute(const std::string &path, const std::vector<std::string*> &params, const std::string &cwd, const std::string &terminal, bool sudo, ib::Error &error) { // {{{
  std::string strparams;
  for(const auto &p : params) {
    ib::oschar qparam[IB_MAX_PATH];
    strparams += " ";
    ib::platform::quote_string(qparam, p->c_str());
    strparams += qparam;
  }
  return ib_platform_shell_execute(path, strparams, cwd, terminal, sudo, error);
} /* }}} */

int ib::platform::command_output(std::string &sstdout, std::string &sstderr, const char *cmd, ib::Error &error) { // {{{
  int outfd[2];
  int efd[2];
  int infd[2];
  pid_t pid;

  std::vector<std::unique_ptr<char[]>> argv;
  if(parse_cmdline(argv, cmd) != 0) {
    error.setMessage("Invalid command line.");
    error.setCode(1);
    return -1;
  }
  std::unique_ptr<char*[]> cargv(new char*[argv.size()+1]);
  for(std::size_t i = 0; i < argv.size(); i++) {
    cargv.get()[i] = argv.at(i).get();
  }
  cargv[argv.size()] = nullptr;

  if(pipe(outfd) != 0 || pipe(infd) != 0 || pipe(efd) != 0) {
    error.setMessage("Failed to create pipes.");
    error.setCode(1);
    return -1;
  }

  pid = fork();
  if(pid < 0) {
    set_errno(error);
    return -1;
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
    auto ret = waitpid(pid, &status, WUNTRACED);
    if (ret < 0 || !WIFEXITED(status)) {
      close(infd[1]);
      close(outfd[0]);
      close(efd[0]);
      set_errno(error);
      return -1;
    }
    close(infd[1]);
    close(outfd[0]);
    close(efd[0]);
    return WEXITSTATUS(status);
  }
  return -1;
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
    auto name = ib::utils::to_command_name(prop_name);

    auto prop_comment = kvf.get(SECTION_KEY, "Comment");
    if(!prop_comment.empty()){
      auto normalized = normalize_desktop_entry_value(prop_comment);
      cmd->setDescription(normalized);
    }

    auto prop_icon = kvf.get(SECTION_KEY, "Icon");
    if(!prop_icon.empty()){
      std::string path;
      const auto theme = ib::Singleton<ib::Config>::getInstance()->getIconTheme().c_str();
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
        std::vector<std::unique_ptr<ib::oschar[]>> cmdline;
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
            const auto *v = it->get();
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
      auto prop_terminal = kvf.get(SECTION_KEY, "Terminal");
      if(!prop_terminal.empty()){
        if(prop_terminal == "true") {
          cmd->setTerminal("yes");
        } else if(prop_terminal == "false") {
          cmd->setTerminal("no");
        } else {
          cmd->setTerminal("auto");
        }
      }
    } else if(prop_type == "Directory") {
      // TODO
    } else if(prop_type == "Link") {
      // TODO
    }
} // }}}

void ib::platform::on_command_init(ib::Command *cmd) { // {{{
  const auto path = cmd->getCommandPath().c_str();

  if(string_endswith(path, ".desktop")){
    desktop_entry2command(cmd, path);
  }
} // }}}

ib::oschar* ib::platform::default_config_path(ib::oschar *result) { // {{{
  if(result == nullptr){ result = new ib::oschar[IB_MAX_PATH]; }
  if(getenv("XDG_CONFIG_HOME") != nullptr) {
    snprintf(result, IB_MAX_PATH, "%s/iceberg", getenv("XDG_CONFIG_HOME"));
    return result;
  }

  snprintf(result, IB_MAX_PATH, "%s/.config", getenv("HOME"));
  if(ib::platform::directory_exists(result)) {
    snprintf(result, IB_MAX_PATH, "%s/.config/iceberg", getenv("HOME"));
    return result;
  }

  snprintf(result, IB_MAX_PATH, "%s/%s", getenv("HOME"), ".iceberg");
  return result;
} // }}}

ib::oschar* ib::platform::resolve_icon(ib::oschar *result, ib::oschar *file, int size){ // {{{
  if(result == nullptr){ result = new ib::oschar[IB_MAX_PATH]; }
  if(ib::platform::file_exists(file)) {
    strncpy_s(result, file, IB_MAX_PATH);
  } else {
    auto repos = FreeDesktopThemeRepos::inst();
    const auto theme = ib::Singleton<ib::Config>::getInstance()->getIconTheme().c_str();
    std::string iconpath;
    repos->findIcon(iconpath, theme, file, size);
    strncpy_s(result, iconpath.c_str(), IB_MAX_PATH);
  }
  return result;
} // }}}

//////////////////////////////////////////////////
// path functions {{{
//////////////////////////////////////////////////
Fl_Image* ib::platform::get_associated_icon_image(const ib::oschar *path, const int size){ // {{{
  auto repos = FreeDesktopThemeRepos::inst();
  std::string iconpath;
  bool isdir = ib::platform::directory_exists(path);
  const auto theme = ib::Singleton<ib::Config>::getInstance()->getIconTheme().c_str();

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
    return ib::Singleton<ib::IconManager>::getInstance()->readFileIcon(iconpath.c_str(), size);
  }
  return nullptr;
} /* }}} */

ib::oschar* ib::platform::join_path(ib::oschar *result, const ib::oschar *parent, const ib::oschar *child) { // {{{
  if(result == nullptr){
    result = new ib::oschar[IB_MAX_PATH];
  }
  const auto length = strlen(parent);
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
  if(result == nullptr){ result = new ib::oschar[IB_MAX_PATH]; }
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
  auto parts = sep.split(tmp);
  std::vector<std::string>  st;
  for(const auto &part : parts) {
    if(part == ".." && st.size() > 0){
      st.pop_back();
    } else {
      st.emplace_back(part);
    }
  }
  for(const auto &part : st) {
    strcat(result, part.c_str());
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
  return result;
} // }}}

ib::oschar* ib::platform::normalize_join_path(ib::oschar *result, const ib::oschar *parent, const ib::oschar *child){ // {{{
  if(result == nullptr){ result = new ib::oschar[IB_MAX_PATH]; }
  ib::oschar tmp[IB_MAX_PATH];
  ib::platform::join_path(tmp, parent, child);
  ib::platform::normalize_path(result, tmp);
  return result;
} // }}}

ib::oschar* ib::platform::dirname(ib::oschar *result, const ib::oschar *path){ // {{{
  if(result == nullptr){ result = new ib::oschar[IB_MAX_PATH]; }
  strncpy_s(result, path, IB_MAX_PATH);
  const auto len = strlen(path);
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
  if(result == nullptr){ result = new ib::oschar[IB_MAX_PATH]; }
  ib::oschar tmp[IB_MAX_PATH];
  strncpy_s(tmp, path, IB_MAX_PATH);
  const auto basename = fl_filename_name(tmp);
  strncpy_s(result, basename, IB_MAX_PATH);
  return result;
} // }}}

ib::oschar* ib::platform::to_absolute_path(ib::oschar *result, const ib::oschar *dir, const ib::oschar *path) { // {{{
  if(result == nullptr){ result = new ib::oschar[IB_MAX_PATH]; }
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
  const auto length = strlen(path);
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

int ib::platform::walk_dir(std::vector<std::unique_ptr<ib::oschar[]>> &result, const ib::oschar *dir, ib::Error &error, bool recursive) { // {{{
    auto d = opendir(dir);
    if (!d) {
      set_errno(error);
      return 1;
    }
    while (1) {
        auto entry = readdir(d);
        if (!entry) break;
        const auto d_name = entry->d_name;
        if (entry->d_type & DT_DIR) {
            if (strcmp (d_name, "..") != 0 &&
                strcmp (d_name, ".") != 0) {
                auto path = new ib::oschar[IB_MAX_PATH];
                if(recursive){
                  snprintf(path, IB_MAX_PATH, "%s/%s", dir, d_name);
                } else {
                  snprintf(path, IB_MAX_PATH, "%s", d_name);
                }
                result.push_back(std::unique_ptr<ib::oschar[]>(path));
                if(recursive) {
                  ib::platform::walk_dir(result, path, error, recursive);
                }
            }
        } else {
          auto path = new ib::oschar[IB_MAX_PATH];
          if(recursive){
            snprintf(path, IB_MAX_PATH, "%s/%s", dir, d_name);
          } else {
            snprintf(path, IB_MAX_PATH, "%s", d_name);
          }
          result.push_back(std::unique_ptr<ib::oschar[]>(path));
        }
    }
    if(closedir(d)){
      // ignore errors...
    }
    std::sort(result.begin(), result.end(), [](const std::unique_ptr<ib::oschar[]> &left, const std::unique_ptr<ib::oschar[]> &right) {
        return strcmp(left.get(), right.get()) < 0;
    });

    return 0;
} // }}}

ib::oschar* ib::platform::get_self_path(ib::oschar *result){ // {{{
  if(result == nullptr){ result = new ib::oschar[IB_MAX_PATH]; }
  char buf[64];
  snprintf(buf, 64, "/proc/%d/exe", getpid());
  const auto ret = readlink(buf, result, IB_MAX_PATH-1);
  if(ret < 0) {
    ib::utils::message_box("Failed to get self path.");
  }
  result[ret] = '\0';
  return result;
} // }}}

ib::oschar* ib::platform::get_current_workdir(ib::oschar *result){ // {{{
  if(result == nullptr){ result = new ib::oschar[IB_MAX_PATH]; }
  if(nullptr == getcwd(result, IB_MAX_PATH-1)) {
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
  if(result == nullptr){ result = new ib::oschar[IB_MAX_PATH]; }
  char *filename = result;
  const char* program = name;
  std::size_t filesize = IB_MAX_PATH;

  // code from fl_open_uri.cxx
  const char *path;
  char       *ptr,
             *end;
  if ((path = getenv("PATH")) == nullptr) path = "/bin:/usr/bin";
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
  if(result == nullptr){ result = new ib::oschar[IB_MAX_PATH]; }
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
  if(result == nullptr){ result = new ib::oschar[IB_MAX_PATH]; }
  memset(result, 0, IB_MAX_PATH);
  const auto dot = strrchr(path, '.');
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
  pthread_create(t, nullptr, f, p);
}
/* }}} */

void ib::platform::on_thread_start(){ /* {{{ */
} /* }}} */

void ib::platform::join_thread(ib::thread *t){ /* {{{ */
  pthread_join(*t, nullptr);
} /* }}} */

void ib::platform::exit_thread(int exit_code) { // {{{
  // nothing to do
} // }}}

void ib::platform::create_mutex(ib::mutex *m) { /* {{{ */
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
  const auto ret = pthread_mutex_init(m, &attr);
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
  pthread_cond_init(c, nullptr);
} /* }}} */

void ib::platform::destroy_condition(ib::condition *c) { /* {{{ */
  pthread_cond_destroy(c);
} /* }}} */

int ib::platform::wait_condition(ib::condition *c, ib::cmutex *m, int ms) { /* {{{ */
  if(ms == 0) return pthread_cond_wait(c, m);
  struct timespec tv;
  tv.tv_sec  = time(nullptr) + ms/1000;
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
//////////////////////////////////////////////////
// system functions }}}
//////////////////////////////////////////////////

//////////////////////////////////////////////////
// platform specific functions {{{
//////////////////////////////////////////////////
void ib::platform::move_to_current_desktop(Fl_Window *w) {
  const auto desktop = xcurrent_desktop();
  if(desktop >= 0) {
    xsend_message_la(fl_xid(w), ib_g_atoms.net_wm_desktop, desktop,0,0,0,0);
  }
}
//////////////////////////////////////////////////
// platform specific functions }}}
//////////////////////////////////////////////////
