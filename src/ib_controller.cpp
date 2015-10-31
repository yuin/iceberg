#include "ib_controller.h"
#include "ib_config.h"
#include "ib_platform.h"
#include "ib_lua.h"
#include "ib_search_path.h"
#include "ib_lexer.h"
#include "ib_ui.h"
#include "ib_completer.h"
#include "ib_regex.h"
#include "ib_history.h"
#include "ib_icon_manager.h"

void ib::Controller::initFonts(){ // {{{
  auto &cfg = ib::Config::inst();
  Fl::set_font(ib::Fonts::input, cfg.getStyleInputFont().c_str());
  Fl::set_font(ib::Fonts::list, cfg.getStyleListFont().c_str());

  fl_message_font(ib::Fonts::input, 16);
} // }}}

void ib::Controller::initBoxtypes(){ // {{{
  int v;
  v = FL_NO_BOX;
  v = FL_FLAT_BOX;
  v = FL_UP_BOX;
  v = FL_DOWN_BOX;
  v = FL_UP_FRAME;
  v = FL_DOWN_FRAME;
  v = FL_THIN_UP_BOX;
  v = FL_THIN_DOWN_BOX;
  v = FL_THIN_UP_FRAME;
  v = FL_THIN_DOWN_FRAME;
  v = FL_ENGRAVED_BOX;
  v = FL_EMBOSSED_BOX;
  v = FL_ENGRAVED_FRAME;
  v = FL_EMBOSSED_FRAME;
  v = FL_BORDER_BOX;
  v = FL_BORDER_FRAME;
  v = FL_ROUND_UP_BOX;
  v = FL_ROUND_DOWN_BOX;
  v = FL_SHADOW_BOX;
  v = FL_SHADOW_FRAME;
  v = FL_ROUNDED_BOX;
  v = FL_ROUNDED_FRAME;
  v = FL_RFLAT_BOX;
  v = FL_RSHADOW_BOX;
  v = FL_DIAMOND_UP_BOX;
  v = FL_DIAMOND_DOWN_BOX;
  v = FL_OVAL_BOX;
  v = FL_OSHADOW_BOX;
  v = FL_OVAL_FRAME;
  v = FL_OFLAT_BOX;
  v = FL_PLASTIC_UP_BOX;
  v = FL_PLASTIC_DOWN_BOX;
  v = FL_PLASTIC_UP_FRAME;
  v = FL_PLASTIC_DOWN_FRAME;
  v = FL_PLASTIC_THIN_UP_BOX;
  v = FL_PLASTIC_THIN_DOWN_BOX;
  v = FL_PLASTIC_ROUND_UP_BOX;
  v = FL_PLASTIC_ROUND_DOWN_BOX;
  (void)v;
} // }}}

void ib::Controller::executeCommand() { // {{{
  auto input = ib::MainWindow::inst()->getInput();
  auto listbox = ib::ListWindow::inst()->getListbox();

  const bool is_empty = input->isEmpty();

  if(listbox->isAutocompleted()){
    completionInput();
  }
  input->scan();
  const std::string rawvalue(input->value());
  std::string workdir;
  if(input->isUsingCwd()){
    workdir = getCwd();
  }

  const std::string cmd(is_empty ? ":empty" : input->getFirstValue());
  ib::unique_oschar_ptr os_cmd(ib::platform::utf82oschar(cmd.c_str()));
  ib::Error error;
  bool success = false;
  std::string message;
  auto it = commands_.find(cmd);
  if(it != commands_.end()){
    if((*it).second->execute(input->getParamValues(), workdir.empty() ? 0 : &workdir, error) != 0){
      if(!error.getMessage().empty()){
        message = error.getMessage();
      }
    }else{
      success = true;
    }
  }else {
    ib::oschar osabspath[IB_MAX_PATH];
    ib::oschar oscwd[IB_MAX_PATH];
    ib::platform::utf82oschar_b(oscwd, IB_MAX_PATH, getCwd().c_str());
    ib::platform::to_absolute_path(osabspath, oscwd, os_cmd.get());
    char abspath[IB_MAX_PATH_BYTE];
    ib::platform::oschar2utf8_b(abspath, IB_MAX_PATH_BYTE, osabspath);
    if(ib::platform::directory_exists(osabspath)){
      if(ib::utils::open_directory(abspath, error) != 0){
        message = error.getMessage();
      }else{
        success = true;
      }
    }else{
      ib::oschar osdirpath[IB_MAX_PATH];
      ib::platform::dirname(osdirpath, osabspath);
      char dirpath[IB_MAX_PATH_BYTE];
      ib::platform::oschar2utf8_b(dirpath, IB_MAX_PATH_BYTE, osdirpath);
      std::string strdirpath = dirpath;
      if(ib::platform::shell_execute(abspath, input->getParamValues(), strdirpath, error) != 0){
        message = error.getMessage();
      }else{
        success = true;
      }
    }
  }

  afterExecuteCommand(success, message.empty() ? 0 : message.c_str());
  if(success){
    if(it != commands_.end()){
      if((*it).second->isEnabledHistory()){
        ib::History::inst().addBaseCommandHistory(rawvalue, (*it).second);
      }else{
        ib::History::inst().addRawInputHistory((*it).second->getName());
      }
    }else{
      ib::History::inst().addRawInputHistory(rawvalue);
    }
  }

  return;
} // }}}

void ib::Controller::afterExecuteCommand(const bool success, const char *message) { // {{{
  auto input = ib::MainWindow::inst()->getInput();
  auto listbox = ib::ListWindow::inst()->getListbox();
  if(success){
    setHistorySearchMode(false, false);
    listbox->clearAll();
    ib::ListWindow::inst()->hide();
    if(result_text_.empty()) {
      input->clear();
      ib::MainWindow::inst()->hide();
    }else{
      ib::MainWindow::inst()->clearIconbox();
      input->value(result_text_.c_str());
      input->position(0);
      input->mark((int)strlen(result_text_.c_str()));
      result_text_.clear();
      input->adjustSize();
    }
  }
  if(message != 0){
    fl_alert("%s", message);
  }
} // }}}

void ib::Controller::hideApplication() { // {{{
#ifdef IB_OS_WIN
  ib::platform::hide_window(ib::MainWindow::inst());
  if(ib::ListWindow::inst()->visible()) {
    ib::platform::hide_window(ib::ListWindow::inst());
    ib::ListWindow::inst()->set_visible();
  }
#else
  ib::MainWindow::inst()->hide();
  ib::ListWindow::inst()->hide();
#endif
} // }}}

void ib::Controller::showApplication() { // {{{
#ifdef IB_OS_WIN
  if(ib::ListWindow::inst()->getListbox()->isEmpty()) {
    ib::platform::hide_window(ib::ListWindow::inst());
  }else if(ib::ListWindow::inst()->visible()) {
    ib::platform::show_window(ib::ListWindow::inst());
  }
  ib::platform::show_window(ib::MainWindow::inst());
#else
  if(ib::ListWindow::inst()->getListbox()->isEmpty()) {
    ib::ListWindow::inst()->hide();
  }else {
    ib::ListWindow::inst()->show();
    ib::platform::show_window(ib::ListWindow::inst());
  }
  ib::platform::show_window(ib::MainWindow::inst());
#endif
} // }}}

void ib::Controller::loadConfig(const int argc, char* const *argv) { // {{{
  ib::Config &cfg = ib::Config::inst();
  ib::Error error;
  
  const char *usage = "Usage: iceberg.exe [-c CONFIG_FILE] [-m message]";
  for(int i = 0; i < argc; ++i) {
    int error = 0;
    if(strcmp(argv[i], "-r") == 0){
      if(i == argc-1) { error = 1; }
      else{ cfg.setOldPid(atoi(argv[++i]));}
    }else if(strcmp(argv[i], "-c") == 0) {
      if(i == argc-1) { error = 1; }
      else{ cfg.setConfigPath(argv[++i]); }
    }else if(strcmp(argv[i], "-m") == 0) {
      if(i == argc-1) { error = 1; }
      else{ cfg.setIpcMessage(argv[++i]); }
    }

    if(error) {
      fl_alert("%s", usage);
      ib::utils::exit_application(1);
    }
  }

  ib::oschar osbuf[IB_MAX_PATH];
  ib::platform::get_current_workdir(osbuf);
  ib::unique_char_ptr current_workdir(ib::platform::oschar2utf8(osbuf));
  ib::platform::get_self_path(osbuf);
  ib::unique_char_ptr self_path(ib::platform::oschar2utf8(osbuf));
  ib::unique_oschar_ptr osself_dir(ib::platform::dirname(0, osbuf));
  cfg.setSelfPath(self_path.get());
  cfg.setInitialWorkdir(current_workdir.get());
  setCwd(current_workdir.get(), error);
  if(cfg.getConfigPath().size() == 0) {
    ib::unique_oschar_ptr osconfig_name(ib::platform::utf82oschar("config.lua"));
    ib::unique_oschar_ptr oscconfig_path(ib::platform::join_path(0, osself_dir.get(), osconfig_name.get()));
    ib::unique_char_ptr   cconfig_path(ib::platform::oschar2utf8(oscconfig_path.get()));
    cfg.setConfigPath(cconfig_path.get());
  }
  ib::unique_oschar_ptr osconfig_path(ib::platform::utf82oschar(cfg.getConfigPath().c_str()));

  ib::platform::dirname(osbuf, osconfig_path.get());
  ib::unique_oschar_ptr oscmd_cache_name(ib::platform::utf82oschar("commands.cache"));
  ib::unique_oschar_ptr oscmd_cache_path(ib::platform::join_path(0, osbuf, oscmd_cache_name.get()));
  ib::unique_char_ptr   cmd_cache_path(ib::platform::oschar2utf8(oscmd_cache_path.get()));
  cfg.setCommandCachePath(cmd_cache_path.get());

  ib::platform::dirname(osbuf, osconfig_path.get());
  ib::unique_oschar_ptr oshistory_name(ib::platform::utf82oschar("history.txt"));
  ib::unique_oschar_ptr oshistory_path(ib::platform::join_path(0, osbuf, oshistory_name.get()));
  ib::unique_char_ptr   history_path(ib::platform::oschar2utf8(oshistory_path.get()));
  cfg.setHistoryPath(history_path.get());

  ib::platform::dirname(osbuf, osconfig_path.get());
  ib::unique_oschar_ptr osicon_cache_name(ib::platform::utf82oschar("icons.cache"));
  ib::unique_oschar_ptr osicon_cache_path(ib::platform::join_path(0, osbuf, osicon_cache_name.get()));
  ib::unique_char_ptr   icon_cache_path(ib::platform::oschar2utf8(osicon_cache_path.get()));
  cfg.setIconCachePath(icon_cache_path.get());

  ib::platform::dirname(osbuf, osconfig_path.get());
  ib::unique_oschar_ptr osmigemo_dict_name(ib::platform::utf82oschar("dict"));
  ib::unique_oschar_ptr osmigemo_dict_path(ib::platform::normalize_join_path(0, osbuf, osmigemo_dict_name.get()));
  ib::unique_char_ptr   migemo_dict_path(ib::platform::oschar2utf8(osmigemo_dict_path.get()));
  cfg.setMigemoDictPath(migemo_dict_path.get());

  cfg.setPlatform(IB_OS_STRING);

  ib::MainLuaState::inst().init();
  
  ib::SearchPath *search_path = 0;
  lua_Integer luint = 0;
  lua_Number  ludouble = 0.0;
  int         key_buf[3];
  int         number;
  int         i;
#define READ_UNSIGNED_INT(name) \
  luint = lua_tointeger(IB_LUA, -1); \
  if(luint < 0) { fl_alert(#name " must be an unsigned int."); ib::utils::exit_application(1); }\
  if(luint > 4096) { fl_alert(#name "is too large."); ib::utils::exit_application(1); } \
  number = (int)luint;
#define READ_UNSIGNED_INT_M(name, max) \
  luint = lua_tointeger(IB_LUA, -1); \
  if(luint < 0) { fl_alert(#name " must be an unsigned int."); ib::utils::exit_application(1); }\
  if(luint > max) { fl_alert(#name "is too large."); ib::utils::exit_application(1); } \
  number = (int)luint;
#define GET_FIELD(name, type) lua_getfield(IB_LUA, -1, name); if(!lua_isnil(IB_LUA, -1) && !lua_is##type(IB_LUA, -1)) { fl_alert(#name " must be " #type); ib::utils::exit_application(1); } if(!lua_isnil(IB_LUA, -1))
#define GET_LIST(index, type) lua_pushinteger(IB_LUA, index); lua_gettable(IB_LUA, -2); if(lua_isnil(IB_LUA, -1)){lua_pop(IB_LUA, 1);break;}if(!lua_is##type(IB_LUA, -1)) { fl_alert("index" #index " must be " #type); ib::utils::exit_application(1); }
#define ENUMERATE_TABLE lua_pushnil(IB_LUA);while (lua_next(IB_LUA, -2) != 0) 
#define PARSE_KEY_BIND(name) memset(key_buf, 0, sizeof(key_buf));ib::utils::parse_key_bind(key_buf, lua_tostring(IB_LUA, -1));if(key_buf[0] == 0) { fl_alert("Invalid key bind:" #name);ib::utils::exit_application(1);}


  // System {{{
  lua_getglobal(IB_LUA, "system");
    GET_FIELD("default_search_path_depth", number) {
       READ_UNSIGNED_INT("default_search_path_depth");
       cfg.setDefaultSearchPathDepth(number);
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("enable_icons", boolean) {
       cfg.setEnableIcons(lua_toboolean(IB_LUA, -1) != 0);
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("max_cached_icons", number) {
       READ_UNSIGNED_INT_M("max_cached_icons", 1000000000);
       cfg.setMaxCachedIcons(number);
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("key_event_threshold", number) {
       READ_UNSIGNED_INT("key_event_threshold");
       cfg.setKeyEventThreshold(number);
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("max_histories", number) {
       READ_UNSIGNED_INT("max_histories");
       cfg.setMaxHistories(number);
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("max_candidates", number) {
       READ_UNSIGNED_INT("max_candidates");
       cfg.setMaxCandidates(number);
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("max_clipboard_histories", number) {
       READ_UNSIGNED_INT("max_clipboard_histories");
       cfg.setMaxClipboardHistories(number);
    }
    lua_pop(IB_LUA, 1);

    GET_FIELD("history_factor", number) {
       ludouble = lua_tonumber(IB_LUA, -1);
       if(ludouble > 1.0 || ludouble < 0.0) {
         fl_alert("history_factor must be 1.0 > value > 0.0;");
         ib::utils::exit_application(1);
       }
       cfg.setHistoryFactor(ludouble);
    }
    lua_pop(IB_LUA, 1);

    GET_FIELD("file_browser", string) {
      cfg.setFileBrowser(lua_tostring(IB_LUA, -1));
    }
    lua_pop(IB_LUA, 1);

    GET_FIELD("server_port", number) {
       luint = lua_tointeger(IB_LUA, -1);
       cfg.setServerPort(luint);
    }
    lua_pop(IB_LUA, 1);

    GET_FIELD("path_autocomplete", boolean) {
       cfg.setPathAutocomplete(lua_toboolean(IB_LUA, -1) != 0);
    }
    lua_pop(IB_LUA, 1);

    GET_FIELD("option_autocomplete", boolean) {
       cfg.setOptionAutocomplete(lua_toboolean(IB_LUA, -1) != 0);
    }
    lua_pop(IB_LUA, 1);

    GET_FIELD("hot_key", string) {
      PARSE_KEY_BIND("hot_key");
      cfg.setHotKey(key_buf);
    }
    lua_pop(IB_LUA, 1);

    GET_FIELD("escape_key", string) {
      PARSE_KEY_BIND("escape_key");
      cfg.setEscapeKey(key_buf);
    }
    lua_pop(IB_LUA, 1);

    GET_FIELD("list_next_key", string) {
      PARSE_KEY_BIND("list_next_key");
      cfg.setListNextKey(key_buf);
    }
    lua_pop(IB_LUA, 1);

    GET_FIELD("list_prev_key", string) {
      PARSE_KEY_BIND("list_prev_key");
      cfg.setListPrevKey(key_buf);
    }
    lua_pop(IB_LUA, 1);

    GET_FIELD("toggle_mode_key", string) {
      PARSE_KEY_BIND("toggle_mode_key");
      cfg.setToggleModeKey(key_buf);
    }
    lua_pop(IB_LUA, 1);

    GET_FIELD("kill_word_key", string) {
      PARSE_KEY_BIND("kill_word_key");
      cfg.setKillWordKey(key_buf);
    }
    lua_pop(IB_LUA, 1);


    GET_FIELD("search_path", table) {
      for(i = 1;;i++){
        GET_LIST(i, table) {
          GET_FIELD("path", string) {
            search_path = new ib::SearchPath(lua_tostring(IB_LUA, -1));
          }
          lua_pop(IB_LUA, 1);
          GET_FIELD("category", string) {
            search_path->setCategory(lua_tostring(IB_LUA, -1));
          }
          lua_pop(IB_LUA, 1);
          GET_FIELD("depth", number) {
            READ_UNSIGNED_INT("depth");
            search_path->setDepth(number);
          }
          lua_pop(IB_LUA, 1);
          GET_FIELD("pattern", string) {
            search_path->setPattern(lua_tostring(IB_LUA, -1));
          }
          lua_pop(IB_LUA, 1);

          ib::unique_string_ptr error_msg1(new std::string(""));
          ib::Regex re1(search_path->getPattern().c_str(), ib::Regex::I);
          if(re1.init(error_msg1.get()) != 0) {
            fl_alert("search_path(%s, pattern) : %s", search_path->getPath().c_str(), error_msg1.get()->c_str());
            ib::utils::exit_application(1);
          }

          GET_FIELD("exclude_pattern", string) {
            search_path->setExcludePattern(lua_tostring(IB_LUA, -1));
          }
          lua_pop(IB_LUA, 1);

          ib::unique_string_ptr error_msg2(new std::string(""));
          ib::Regex re2(search_path->getExcludePattern().c_str(), ib::Regex::I);
          if(re2.init(error_msg2.get()) != 0) {
            fl_alert("search_path(%s, exclude_pattern) : %s", search_path->getPath().c_str(), error_msg2.get()->c_str());
            ib::utils::exit_application(1);
          }
          
          cfg.addSearchPath(search_path);
        }
        lua_pop(IB_LUA, 1);
      }
    }
    lua_pop(IB_LUA, 1);

    GET_FIELD("completer", table) {
      GET_FIELD("command", number) {
        READ_UNSIGNED_INT("command");
        if(number == ib::CompletionMethod::BEGINS_WITH) {
          ib::Completer::inst().setMethodCommand(new ib::BeginsWithMatchCompletionMethod());
        }else if(number == ib::CompletionMethod::PARTIAL) {
          ib::Completer::inst().setMethodCommand(new ib::PartialMatchCompletionMethod());
        }else if(number == ib::CompletionMethod::ABBR) {
          ib::Completer::inst().setMethodCommand(new ib::AbbrMatchCompletionMethod());
        }else{
          fl_alert("unknown completer.");
          ib::utils::exit_application(1);
        }
      }
      lua_pop(IB_LUA, 1);
      GET_FIELD("path", number) {
        READ_UNSIGNED_INT("path");
        if(number == ib::CompletionMethod::BEGINS_WITH) {
          ib::Completer::inst().setMethodPath(new ib::BeginsWithMatchCompletionMethod());
        }else if(number == ib::CompletionMethod::PARTIAL) {
          ib::Completer::inst().setMethodPath(new ib::PartialMatchCompletionMethod());
        }else if(number == ib::CompletionMethod::ABBR) {
          ib::Completer::inst().setMethodPath(new ib::AbbrMatchCompletionMethod());
        }else{
          fl_alert("unknown completer.");
          ib::utils::exit_application(1);
        }
      }
      lua_pop(IB_LUA, 1);
      GET_FIELD("history", number) {
        READ_UNSIGNED_INT("history");
        if(number == ib::CompletionMethod::BEGINS_WITH) {
          ib::Completer::inst().setMethodHistory(new ib::BeginsWithMatchCompletionMethod());
        }else if(number == ib::CompletionMethod::PARTIAL) {
          ib::Completer::inst().setMethodHistory(new ib::PartialMatchCompletionMethod());
        }else if(number == ib::CompletionMethod::ABBR) {
          ib::Completer::inst().setMethodHistory(new ib::AbbrMatchCompletionMethod());
        }else{
          fl_alert("unknown completer.");
          ib::utils::exit_application(1);
        }
      }
      lua_pop(IB_LUA, 1);
      GET_FIELD("option", number) {
        READ_UNSIGNED_INT("option");
        if(number == ib::CompletionMethod::BEGINS_WITH) {
          ib::Completer::inst().setMethodOption(new ib::BeginsWithMatchCompletionMethod());
        }else if(number == ib::CompletionMethod::PARTIAL) {
          ib::Completer::inst().setMethodOption(new ib::PartialMatchCompletionMethod());
        }else if(number == ib::CompletionMethod::ABBR) {
          ib::Completer::inst().setMethodOption(new ib::AbbrMatchCompletionMethod());
        }else{
          fl_alert("unknown completer.");
          ib::utils::exit_application(1);
        }
      }
      lua_pop(IB_LUA, 1);

      GET_FIELD("option_func", table) {
        ENUMERATE_TABLE {
          ib::Completer::inst().setOptionFuncFlag(luaL_checkstring(IB_LUA, -2));
          lua_pop(IB_LUA, 1);
        }
      }
      lua_pop(IB_LUA, 1);
    }
    lua_pop(IB_LUA, 1);
  lua_pop(IB_LUA, 1);
  assert(lua_gettop(IB_LUA) == 0);
  // }}}

  // Styles {{{
  lua_getglobal(IB_LUA, "styles");
    GET_FIELD("window_boxtype", number) {
       READ_UNSIGNED_INT("window_boxtype");
       cfg.setStyleWindowBoxtype((Fl_Boxtype)number);
    }
    lua_pop(IB_LUA, 1);

    GET_FIELD("window_posx", number) {
       READ_UNSIGNED_INT("window_posx");
       cfg.setStyleWindowPosx(number);
       cfg.setStyleWindowPosxAuto(false);
    }
    lua_pop(IB_LUA, 1);

    lua_getfield(IB_LUA, -1, "window_posx");
    if(lua_isnil(IB_LUA, -1)){
       cfg.setStyleWindowPosxAuto(true);
    }
    lua_pop(IB_LUA, 1);

    GET_FIELD("window_posy", number) {
       READ_UNSIGNED_INT("window_posy");
       cfg.setStyleWindowPosy(number);
       cfg.setStyleWindowPosyAuto(false);
    }
    lua_pop(IB_LUA, 1);

    lua_getfield(IB_LUA, -1, "window_posy");
    if(lua_isnil(IB_LUA, -1)){
       cfg.setStyleWindowPosyAuto(true);
    }
    lua_pop(IB_LUA, 1);


    GET_FIELD("window_width", number) {
       READ_UNSIGNED_INT("window_width");
       cfg.setStyleWindowWidth(number);
    }
    lua_pop(IB_LUA, 1);

    GET_FIELD("window_height", number){
      READ_UNSIGNED_INT("window_height");
      cfg.setStyleWindowHeight(number);
    }
    lua_pop(IB_LUA, 1);

    GET_FIELD("window_padx", number){
      READ_UNSIGNED_INT("window_padx");
      cfg.setStyleWindowPadx(number);
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("window_pady", number){
      READ_UNSIGNED_INT("window_pady");
      cfg.setStyleWindowPady(number);
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("window_bg_color", table){
      cfg.setStyleWindowBgColor(ib::MainLuaState::inst().getColorFromStackTop());
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("taskbar_height", number){
      READ_UNSIGNED_INT("taskbar_height");
      cfg.setStyleTaskbarHeight(number);
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("window_alpha", number){
      READ_UNSIGNED_INT("window_alpha");
      cfg.setStyleWindowAlpha(number);
    }
    lua_pop(IB_LUA, 1);


    GET_FIELD("input_boxtype", number) {
       READ_UNSIGNED_INT("input_boxtype");
       cfg.setStyleInputBoxtype((Fl_Boxtype)number);
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("input_font", string) {
      cfg.setStyleInputFont(lua_tostring(IB_LUA, 2));
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("input_font_size", number) {
      READ_UNSIGNED_INT("input_font_size");
      cfg.setStyleInputFontSize(number);
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("input_font_color", table){
      cfg.setStyleInputFontColor(ib::MainLuaState::inst().getColorFromStackTop());
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("input_bg_color", table){
      cfg.setStyleInputBgColor(ib::MainLuaState::inst().getColorFromStackTop());
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("input_selection_bg_color", table){
      cfg.setStyleInputSelectionBgColor(ib::MainLuaState::inst().getColorFromStackTop());
    }
    lua_pop(IB_LUA, 1);

    GET_FIELD("list_boxtype", number) {
       READ_UNSIGNED_INT("list_boxtype");
       cfg.setStyleListBoxtype((Fl_Boxtype)number);
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("list_padx", number){
      READ_UNSIGNED_INT("list_padx");
      cfg.setStyleListPadx(number);
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("list_pady", number){
      READ_UNSIGNED_INT("list_pady");
      cfg.setStyleListPady(number);
    }
    lua_pop(IB_LUA, 1);

    GET_FIELD("list_font", string){
      cfg.setStyleListFont(lua_tostring(IB_LUA, 2));
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("list_font_size", number){
      READ_UNSIGNED_INT("list_font_size");
      cfg.setStyleListFontSize(number);
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("list_desc_font_size", number){
      READ_UNSIGNED_INT("list_desc_font_size");
      cfg.setStyleListDescFontSize(number);
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("list_font_color", table){
      cfg.setStyleListFontColor(ib::MainLuaState::inst().getColorFromStackTop());
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("list_desc_font_color", table){
      cfg.setStyleListDescFontColor(ib::MainLuaState::inst().getColorFromStackTop());
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("list_selection_font_color", table){
      cfg.setStyleListSelectionFontColor(ib::MainLuaState::inst().getColorFromStackTop());
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("list_selection_desc_font_color", table){
      cfg.setStyleListSelectionDescFontColor(ib::MainLuaState::inst().getColorFromStackTop());
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("list_bg_color1", table){
      cfg.setStyleListBgColor1(ib::MainLuaState::inst().getColorFromStackTop());
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("list_selection_bg_color1", table){
      cfg.setStyleListSelectionBgColor1(ib::MainLuaState::inst().getColorFromStackTop());
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("list_bg_color2", table){
      cfg.setStyleListBgColor2(ib::MainLuaState::inst().getColorFromStackTop());
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("list_selection_bg_color2", table){
      cfg.setStyleListSelectionBgColor2(ib::MainLuaState::inst().getColorFromStackTop());
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("list_border_color", table){
      cfg.setStyleListBorderColor(ib::MainLuaState::inst().getColorFromStackTop());
    }
    lua_pop(IB_LUA, 1);
    GET_FIELD("list_alpha", number){
      READ_UNSIGNED_INT("list_alpha");
      cfg.setStyleListAlpha(number);
    }
    lua_pop(IB_LUA, 1);
  lua_pop(IB_LUA, 1);
  assert(lua_gettop(IB_LUA) == 0);
  // }}}

  // Commands {{{
  ib::BaseCommand *command = 0;
  lua_getglobal(IB_LUA, "commands");
    ENUMERATE_TABLE {
      lua_getfield(IB_LUA, -1, "path");
      if(lua_isstring(IB_LUA, -1)){
        command = new ib::Command();
        command->setPath(lua_tostring(IB_LUA, -1));
      }else{
        command = new ib::LuaFunctionCommand();
        command->setPath("lua_command");
      }
      lua_pop(IB_LUA, 1);

      command->setName(luaL_checkstring(IB_LUA, -2));

      lua_getfield(IB_LUA, -1, "workdir");
      if(!lua_isnil(IB_LUA, -1)) {
        if(lua_isstring(IB_LUA, -1)) {
          command->setWorkdir(luaL_checkstring(IB_LUA, -1));
        }else{
          command->setIsDynamicWorkdir(true);
        }
      }
      lua_pop(IB_LUA, 1);

      GET_FIELD("description", string) {
        command->setDescription(luaL_checkstring(IB_LUA, -1));
      }
      lua_pop(IB_LUA, 1);

      GET_FIELD("icon", string) {
        command->setIconFile(luaL_checkstring(IB_LUA, -1));
      }
      lua_pop(IB_LUA, 1);

      GET_FIELD("history", boolean) {
        command->setEnablesHistory(lua_toboolean(IB_LUA, -1) != 0);
      }
      lua_pop(IB_LUA, 1);

      if(command->getName() == "" || command->getPath() == ""){
        fl_alert("Command must have 'name' and 'path' attibutes.");
        ib::utils::exit_application(1);
      }

      lua_getfield(IB_LUA, -1, "completion"); 
      if(lua_isfunction(IB_LUA, -1)) { 
        lua_pop(IB_LUA, 1);
        lua_getglobal(IB_LUA, "system");
          lua_getfield(IB_LUA, -1, "completer");
            lua_getfield(IB_LUA, -1, "option_func");
              lua_getfield(IB_LUA, -4, "completion");
              lua_setfield(IB_LUA, -2, command->getName().c_str());
              ib::Completer::inst().setOptionFuncFlag(command->getName().c_str());
            lua_pop(IB_LUA, 1);
          lua_pop(IB_LUA, 1);
        lua_pop(IB_LUA, 1);
      }else{
        lua_pop(IB_LUA, 1);
      }

      addCommand(command->getName(), command);
      lua_pop(IB_LUA, 1);
    }
  lua_pop(IB_LUA, 1);
  assert(lua_gettop(IB_LUA) == 0);
  // }}}

#undef PARSE_KEY_BIND
#undef ENUMERATE_TABLE
#undef GET_LIST
#undef GET_FIELD
#undef READ_UNSIGNED_INT

  cfg.setLoaded(true);
} // }}}

// void ib::Controller::cacheCommandsInSearchPath() { // {{{
void _walk_search_path(std::vector<ib::Command*> &commands,
    const char *category, const char *path, int current_depth, 
    int limit_depth,
    const std::string &pattern, ib::Regex *exre){
  if(current_depth > limit_depth) {
    return;
  }
  ib::oschar osfull_path[IB_MAX_PATH];
  ib::oschar tmp_path[IB_MAX_PATH];
  char       full_path[IB_MAX_PATH_BYTE];
  char       file[IB_MAX_PATH_BYTE];
  char       quoted_path[IB_MAX_PATH_BYTE];
  ib::Regex re(pattern.c_str(), ib::Regex::I);
  re.init();

  std::vector<ib::unique_oschar_ptr> files;
  ib::oschar osdir[IB_MAX_PATH];
  ib::platform::utf82oschar_b(osdir, IB_MAX_PATH, path);
  ib::Error error;
  if(ib::platform::walk_dir(files, osdir, error, false) == 0){
    for(auto it = files.begin(), last = files.end(); it != last; ++it){
      ib::platform::join_path(osfull_path, osdir, (*it).get());
      ib::platform::oschar2utf8_b(full_path, IB_MAX_PATH_BYTE, osfull_path);
      if(exre != 0 && exre->match(full_path) == 0) continue;
      if(ib::platform::directory_exists(osfull_path)){
        _walk_search_path(commands, category, full_path, current_depth+1, limit_depth, pattern, exre);
      }else{
        ib::platform::oschar2utf8_b(file, IB_MAX_PATH_BYTE, (*it).get());
        if(re.match(file) == 0){
          ib::Command *command = new ib::Command();
          ib::platform::oschar2utf8_b(file, IB_MAX_PATH_BYTE, (*it).get());
          std::string cmdname;
          ib::utils::to_command_name(cmdname, file);
          command->setName(cmdname);
          command->setCategory(category);
          command->setWorkdir(path);
          ib::platform::quote_string(tmp_path, osfull_path);
          ib::platform::oschar2utf8_b(quoted_path, IB_MAX_PATH_BYTE, tmp_path);
          command->setPath(quoted_path);
          command->init();
          commands.push_back(command);
        }
      }
    }
  }
}

void ib::Controller::cacheCommandsInSearchPath(const char *category) {
  auto &cfg = ib::Config::inst();
  auto &search_paths = cfg.getSearchPath();
  bool is_all = strcmp(category, "all") == 0;
  long prev_index = 0;
  std::vector<ib::Command*> commands;
  if(!is_all){
    auto &prev_commands = ib::Controller::inst().getCommands();
    for(auto it = prev_commands.begin(), last = prev_commands.end(); it != last; ++it){
      if(!(*it).second->getCategory().empty() && (*it).second->getCategory() != category){
        ib::Command *prev_command = dynamic_cast<ib::Command*>((*it).second);
        if(prev_command != 0){
          commands.push_back(prev_command);
          prev_index++;
        }
      }
    }
  }

  for(auto it = search_paths.begin(), last = search_paths.end(); it != last; ++it){
    if(is_all || (*it)->getCategory() == category){
      int depth = (*it)->getDepth();
      if(depth == 0) depth = cfg.getDefaultSearchPathDepth();
      ib::Regex exre((*it)->getExcludePattern().c_str(), ib::Regex::I);
      exre.init();
      _walk_search_path(commands, (*it)->getCategory().c_str(), (*it)->getPath().c_str(), 0, depth,
                        (*it)->getPattern(), strlen(exre.getPattern()) == 0 ? 0 : &exre);
    }
  }

  ib::unique_char_ptr locache_path(ib::platform::utf82local(cfg.getCommandCachePath().c_str()));
  std::ofstream ofs(locache_path.get());
  for(auto it = commands.begin(), last = commands.end(); it != last; ++it){
    ofs << (*it)->getCategory() << std::endl;
    ofs << (*it)->getName() << std::endl;
    ofs << (*it)->getPath() << std::endl;
    ofs << (*it)->getRawWorkdir() << std::endl;
    ofs << (*it)->getDescription() << std::endl;
    ofs << (*it)->getCommandPath() << std::endl;
  }

  for(long i = prev_index, l = commands.size(); i < l; ++i){
    if(commands.at(i) != 0){ delete commands.at(i); }
  }
  commands.clear();
} // }}}

// void ib::Controller::loadCachedCommands() { // {{{
void ib::Controller::loadCachedCommands() {
  auto &cfg = ib::Config::inst();
  auto input = ib::MainWindow::inst()->getInput();
  input->value("Loading commands...");
  ib::unique_char_ptr locache_path(ib::platform::utf82local(cfg.getCommandCachePath().c_str()));
  std::ifstream ifs(locache_path.get());
  std::string buf;

  while(ifs && getline(ifs, buf)) {
    ib::Command *cmd = new ib::Command();
    cmd->setCategory(buf);
    getline(ifs, buf);
    cmd->setName(buf);
    getline(ifs, buf);
    cmd->setPath(buf);
    getline(ifs, buf);
    cmd->setWorkdir(buf);
    getline(ifs, buf);
    cmd->setDescription(buf);
    getline(ifs, buf);
    cmd->setCommandPath(buf);
    cmd->setInitialized(true);
    addCommand(cmd->getName(), cmd);
  }
  input->value("");
} // }}}

void ib::Controller::addCommand(const std::string &name, ib::BaseCommand *command) { // {{{
  command->init();
  if(commands_.find(name) == commands_.end()){
    commands_[name] = command;
  }else{
    delete command;
  }
} // }}}

void ib::Controller::completionInput() { // {{{
  auto listbox = ib::ListWindow::inst()->getListbox();
  auto input= ib::MainWindow::inst()->getInput();
  if(!listbox->hasSelectedIndex()) return;

  std::size_t position = 0;
  std::string buf;
  if(isHistorySearchMode()){
    buf += listbox->selectedValue()->getCompvalue();
    input->value(buf.c_str());
    input->position((int)position);
  }else{
    if(input->isUsingCwd()){
      buf += "!";
    }
    int i = 0;
    for(auto it = input->getTokens().begin(), last = input->getTokens().end(); it != last; ++it, ++i){
      if(i == input->getCursorTokenIndex()){
        const std::string &cursorValue = input->getCursorToken()->getValue();
        if(!input->getCursorToken()->isValueToken()) {
          buf += cursorValue;
        }
        ib::unique_oschar_ptr os_value(ib::platform::utf82oschar(cursorValue.c_str()));
        if(ib::platform::is_path(os_value.get())){
          ib::oschar os_dirname[IB_MAX_PATH];
          ib::platform::dirname(os_dirname, os_value.get());
          ib::oschar os_compvalue[IB_MAX_PATH];
          ib::platform::utf82oschar_b(os_compvalue, IB_MAX_PATH, listbox->selectedValue()->getCompvalue().c_str());
          ib::oschar os_completed_path[IB_MAX_PATH];
          ib::platform::normalize_join_path(os_completed_path, os_dirname, os_compvalue);
          ib::platform::quote_string(os_completed_path, os_completed_path);
          char completed_path[IB_MAX_PATH_BYTE];
          ib::platform::oschar2utf8_b(completed_path, IB_MAX_PATH_BYTE, os_completed_path);
          buf += completed_path;
        }else{
          const std::string &comp_value = listbox->selectedValue()->getCompvalue();
          ib::unique_oschar_ptr oscomp_value(ib::platform::utf82oschar(comp_value.c_str()));
          ib::unique_oschar_ptr osquoted_value(ib::platform::quote_string(0, oscomp_value.get()));
          ib::unique_char_ptr   quoted_value(ib::platform::oschar2utf8(osquoted_value.get()));
          buf += quoted_value.get();
        }
        position = buf.length();
        if(!buf.empty() && buf.at(position-1) == '"') {
          position--;
        }
      }else{
        buf += (*it)->getToken();
      }

    }
    input->value(buf.c_str());
    input->position((int)position);
  }
  input->adjustSize();
} // }}}

// void ib::Controller::showCompletionCandidates() { // {{{
static int cmp_command(const ib::CompletionValue *a, const ib::CompletionValue *b) {
  const ib::BaseCommand *a_cmd = static_cast<const ib::BaseCommand*>(a);
  const ib::BaseCommand *b_cmd = static_cast<const ib::BaseCommand*>(b);
  return a_cmd->getScore() > b_cmd->getScore();
}

void ib::Controller::showCompletionCandidates() {
  auto listbox = ib::ListWindow::inst()->getListbox();
  auto input   = ib::MainWindow::inst()->getInput();

  input->scan();
  if(input->getPrevCursorValue().size() != 0 && input->getCursorToken()->getValue().find(input->getPrevCursorValue()) != 0) {
    listbox->clearAll();
  }

  if(input->isEmpty()) {
    ib::ListWindow::inst()->hide();
    input->take_focus();
    return;
  }
  listbox->startUpdate();
  const std::string &first_value = input->getFirstValue();
  const std::string &cursor_value = input->getCursorValue();
  ib::unique_oschar_ptr os_cursor_value(ib::platform::utf82oschar(cursor_value.c_str()));
  std::vector<ib::CompletionValue*> candidates;

  bool use_max_candidates = false;
#ifdef IB_OS_WIN
  if(first_value == "/" || first_value == "\\"){
    std::vector<ib::unique_oschar_ptr> os_drives;
    ib::Error error;
    char drive[16];
    if(ib::platform::list_drives(os_drives, error) == 0){
      for(auto it = os_drives.begin(), last = os_drives.end(); it != last; ++it){
        ib::platform::oschar2utf8_b(drive, 16, (*it).get());
        candidates.push_back(new ib::CompletionString(drive));
      }
    }

  }else 
#endif
  if(isHistorySearchMode()){
    use_max_candidates = true;
    ib::Completer::inst().completeHistory(candidates, first_value);
    std::stable_sort(candidates.begin(), candidates.end(), cmp_command);
  }else if(input->getCursorTokenIndex() > 0 && ib::Completer::inst().hasCompletionFunc(first_value)){
    ib::Completer::inst().completeOption(candidates, first_value);
  }else if(ib::platform::is_path(os_cursor_value.get())){
    ib::oschar oscwd[IB_MAX_PATH];
    ib::platform::utf82oschar_b(oscwd, IB_MAX_PATH, getCwd().c_str());
    if(input->getCursorTokenIndex() > 0) {
      if(input->isUsingCwd()) {
        ib::platform::utf82oschar_b(oscwd, IB_MAX_PATH, getCwd().c_str());
      }else{
        const std::string &cmdname = input->getFirstValue();
        auto it = commands_.find(cmdname);
        if(it != commands_.end()){
          ib::platform::utf82oschar_b(oscwd, IB_MAX_PATH, (*it).second->getWorkdir().c_str());
        }else{
          ib::unique_oschar_ptr osfirst_value(ib::platform::utf82oschar(cmdname.c_str()));
          if(ib::platform::is_path(osfirst_value.get())){
            ib::platform::dirname(oscwd, osfirst_value.get());
          }
        }
      }
    }
    ib::oschar osabs_path[IB_MAX_PATH];
    ib::platform::to_absolute_path(osabs_path, oscwd, os_cursor_value.get());
    char abs_path[IB_MAX_PATH_BYTE];
    ib::platform::oschar2utf8_b(abs_path, IB_MAX_PATH_BYTE, osabs_path);
    ib::Completer::inst().completePath(candidates, abs_path);
  }else if(input->getCursorTokenIndex() == 0) {
    use_max_candidates = true;
    candidates = listbox->getValues();
    ib::Completer::inst().completeCommand(candidates, cursor_value);
    std::stable_sort(candidates.begin(), candidates.end(), cmp_command);
  }

  listbox->clearAll();

  for(auto it = candidates.begin(), last = candidates.end(); it != last; ++it){
    listbox->addValue(*it);
  }

  if(input->getCursorTokenIndex() == 0 || input->getPrevCursorTokenIndex() == 0){
    ib::MainWindow::inst()->updateIconbox();
  }
  listbox->endUpdate(use_max_candidates);

  if(!listbox->isEmpty()) {
#ifdef IB_OS_WIN
    ib::platform::show_window(ib::ListWindow::inst());
#else
    ib::ListWindow::inst()->show();
#endif
  }else{
    listbox->clearAll();
    ib::ListWindow::inst()->hide();
  }
  input->take_focus();

} // }}}

void ib::Controller::selectNextCompletion(){ // {{{
  auto listbox = ib::ListWindow::inst()->getListbox();
  if(listbox->isEmpty()) { return; }
  listbox->selectNext();
  completionInput();
} // }}}

void ib::Controller::selectPrevCompletion(){ // {{{
  auto listbox = ib::ListWindow::inst()->getListbox();
  if(listbox->isEmpty()) { return; }
  listbox->selectPrev();
  completionInput();
} // }}}

int ib::Controller::setCwd(const char *value, ib::Error &error){ // {{{
  ib::oschar osvalue[IB_MAX_PATH];
  ib::platform::utf82oschar_b(osvalue, IB_MAX_PATH, value);
  ib::oschar oscwd[IB_MAX_PATH];
  ib::platform::utf82oschar_b(oscwd, IB_MAX_PATH, cwd_.c_str());
  ib::oschar osnew_cwd[IB_MAX_PATH];
  ib::platform::to_absolute_path(osnew_cwd, oscwd, osvalue);
  ib::oschar osnorm_path[IB_MAX_PATH];
  ib::platform::normalize_path(osnorm_path, osnew_cwd);
  if(ib::platform::set_current_workdir(osnorm_path, error) != 0){
    return 1;
  }

  char cwd[IB_MAX_PATH_BYTE];
  ib::platform::oschar2utf8_b(cwd, IB_MAX_PATH_BYTE, osnorm_path);
  cwd_ = cwd;
  return 0;
} // }}}

void ib::Controller::setHistorySearchMode(const bool value, bool display){ // {{{
  if(display){
    auto input = ib::MainWindow::inst()->getInput();
    const char *text;
    if(value){
      text = "(history mode)";
      input->value(text);
    }else{
      text = "(normal mode)";
      input->value(text);
    }

    input->position(0);
    input->mark((int)strlen(text));
    auto listbox = ib::ListWindow::inst()->getListbox();
    listbox->clearAll();
    ib::ListWindow::inst()->hide();
  }
  history_search_ = value;
} // }}}

void ib::Controller::setResultText(const char *value){ // {{{
  result_text_ = value;
  auto input   = ib::MainWindow::inst()->getInput();
  input->getKeyEvent().cancelEvent();
} // }}}

void ib::Controller::killWord() { // {{{
  auto input   = ib::MainWindow::inst()->getInput();

  input->scan();
  std::string buf;
  auto &tokens = input->getTokens();
  std::size_t position = 0;
  int i = 0;
  for(auto it = tokens.begin(), last = tokens.end(); it != last; ++it, ++i){
    if(i == input->getCursorTokenIndex()){
      const std::string &cursor_value = (*it)->getValue();
      ib::unique_oschar_ptr os_value(ib::platform::utf82oschar(cursor_value.c_str()));
      if(ib::platform::is_path(os_value.get())){
        ib::oschar os_dirname[IB_MAX_PATH];
        ib::platform::dirname(os_dirname, os_value.get());
        ib::oschar osquoted_path[IB_MAX_PATH];
        ib::platform::quote_string(osquoted_path, os_dirname);
        char dirname[IB_MAX_PATH_BYTE];
        ib::platform::oschar2utf8_b(dirname, IB_MAX_PATH_BYTE, osquoted_path);
        buf += dirname;
      }
      position = buf.length();
      if(!buf.empty() && buf.at(position-1) == '"') {
        position--;
      }
    }else{
      buf += (*it)->getToken();
    }
  }

  input->clear();
  input->value(buf.c_str());
  input->position((int)position);
  input->adjustSize();
  input->getKeyEvent().queueEvent((void*)1);
} // }}}

void ib::Controller::handleIpcMessage(const char* message){ // {{{
  ib::FlScopedLock lock;

  {
    ib::Regex re_exec("exec (.*)", ib::Regex::NONE);
    re_exec.init();
    if(re_exec.match(message) == 0){
      std::string cmd;
      re_exec._1(cmd);
      showApplication();
      ib::ListWindow::inst()->getListbox()->clearAll();
      ib::ListWindow::inst()->hide();
      ib::MainWindow::inst()->getInput()->setValue(cmd.c_str());
      executeCommand();
      return;
    }
  }

  {
    ib::Regex re_set("set (.*)", ib::Regex::NONE);
    re_set.init();
    if(re_set.match(message) == 0){
      std::string cmd;
      re_set._1(cmd);
      showApplication();
      ib::ListWindow::inst()->getListbox()->clearAll();
      ib::ListWindow::inst()->hide();
      ib::MainWindow::inst()->getInput()->setValue(cmd.c_str());
      return;
    }
  }

  {
    ib::Regex re_activate("activate", ib::Regex::NONE);
    re_activate.init();
    if(re_activate.match(message) == 0){
      showApplication();
    }
  }
} // }}}

void ib::Controller::appendClipboardHistory(const char* text){ // {{{
  auto &cfg = ib::Config::inst();
  std::string *data = new std::string(text);
  for(auto it = clipboard_histories_.begin(), last = clipboard_histories_.end(); it != last; ++it){
    if(*(*it) == *data) {
      delete data;
      return;
    }
  }

  if(clipboard_histories_.size() >= cfg.getMaxClipboardHistories()){
    std::string *front = clipboard_histories_.front();
    delete front;
    clipboard_histories_.pop_front();
  }
  clipboard_histories_.push_back(data);
} // }}}

