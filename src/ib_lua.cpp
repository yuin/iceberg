#include "ib_lua.h"
#include "ib_ui.h"
#include "ib_history.h"
#include "ib_platform.h"
#include "ib_controller.h"
#include "ib_regex.h"
#include "ib_config.h"
#include "ib_singleton.h"

// Lua Class "Regex" {{{
static ib::Regex* to_regex (lua_State *L, int index) {
  auto regex_ptr = reinterpret_cast<ib::Regex**>(lua_touserdata(L, index));
  if (regex_ptr == nullptr) luaL_typerror(L, index, "Regex");
  return *regex_ptr;
}

static ib::Regex* check_regex (lua_State *L, int index) {
  luaL_checktype(L, index, LUA_TUSERDATA);
  auto regex_ptr = reinterpret_cast<ib::Regex**>(luaL_checkudata(L, index, "Regex"));
  if (regex_ptr == nullptr) luaL_typerror(L, index, "Regex");
  if (*regex_ptr == nullptr) luaL_error(L, "null Regex");
  return *regex_ptr;
}

static ib::Regex* push_regex (lua_State *L, ib::Regex *value) {
  auto regex_ptr = reinterpret_cast<ib::Regex**>(lua_newuserdata(L, sizeof(ib::Regex)));
  *regex_ptr = value;
  luaL_getmetatable(L, "Regex");
  lua_setmetatable(L, -2);
  return *regex_ptr;
}

static int Regex_new (lua_State *L) {
  const auto pattern = luaL_checkstring(L, 1);
  const auto flags = static_cast<unsigned int>(luaL_checkint(L, 2));
  auto value = new ib::Regex(pattern, flags);
  ib::unique_string_ptr msg(new std::string());
  if(value->init(msg.get()) != 0) {
    return luaL_error(L, "Invalid regular expression: %s", msg.get()->c_str());
  };
  value->setSubMetaChar('%');
  push_regex(L, value);
  return 1;
}


static int Regex_static_escape(lua_State *L){
  ib::LuaState lua_state(L);
  const auto string    = luaL_checkstring(L, 1);
  auto ret = ib::Regex::escape(string);

  lua_state.clearStack();
  lua_pushstring(L, ret.c_str());
  return 1;
}

static int Regex_match(lua_State *L) {
  ib::LuaState lua_state(L);
  const auto argc = lua_gettop(L);
  auto value      = check_regex(L, 1);
  const auto string    = luaL_checkstring(L, 2);
  unsigned int startpos = 0;
  unsigned int endpos = 0;
  if(argc == 3){ startpos = static_cast<unsigned int>(luaL_checkint(L, 3)); }
  if(argc == 4){ endpos = static_cast<unsigned int>(luaL_checkint(L, 4)); }

  lua_state.clearStack();
  lua_pushboolean(L, value->match(string, startpos, endpos) == 0);
  value->copyString(string);
  return 1;
}

static int Regex_search(lua_State *L) {
  const auto argc = lua_gettop(L);
  auto value      = check_regex(L, 1);
  const auto string    = luaL_checkstring(L, 2);
  unsigned int startpos = 0;
  unsigned int endpos = 0;
  if(argc == 3){ startpos = static_cast<unsigned int>(luaL_checkint(L, 3)); }
  if(argc == 4){ endpos = static_cast<unsigned int>(luaL_checkint(L, 4)); }
  ib::LuaState lua_state(L);
  lua_state.clearStack();
  lua_pushboolean(L, value->search(string, startpos, endpos) == 0);
  value->copyString(string);
  return 1;
}

static int Regex_split(lua_State *L) {
  auto value      = check_regex(L, 1);
  const auto string    = luaL_checkstring(L, 2);
  auto result = value->split(string);

  ib::LuaState lua_state(L);
  lua_state.clearStack();

  lua_newtable(L);
  int i = 1;
  for(auto it = result.begin(), last = result.end(); it != last; ++it, ++i){
    lua_pushnumber(L, i);
    lua_pushstring(L, (*it).c_str());
    lua_settable(L, -3);
  }
  return 1;
}

static int Regex_gsub(lua_State *L) {
  ib::LuaState lua_state(L);
  auto value      = check_regex(L, 1);
  const auto string    = luaL_checkstring(L, 2);
  const auto type = lua_type(L, 3);
  if(type == LUA_TSTRING){
    const auto repl = luaL_checkstring(L, 3);
    lua_state.clearStack();
    lua_pushstring(L, value->gsub(string, repl).c_str());
  }else{
    auto result = value->gsub(string, [](const ib::Regex &reg, std::string *res, void *userdata) {
      auto L = reinterpret_cast<lua_State*>(userdata);
      const auto top = lua_gettop(L);
      lua_pushvalue(L, 3);
      lua_pushvalue(L, 1);
      if(lua_pcall(L, 1, 1, 0) == 0) {
        if(lua_type(L, -1) == LUA_TSTRING){
          *res += lua_tostring(L, -1);
        }
      }else{
        fl_alert("%s", lua_tostring(L, lua_gettop(L)));
      }
      lua_settop(L, top);
    }, L);
    lua_state.clearStack();
    lua_pushstring(L, result.c_str());
  }
  return 1;
}

static int Regex_groups(lua_State *L) {
  auto value      = check_regex(L, 1);
  ib::LuaState lua_state(L);
  lua_state.clearStack();
  lua_pushnumber(L, value->getNumGroups());
  return 1;
}

static int Regex_group(lua_State *L) {
  auto value = check_regex(L, 1);
  const auto n = luaL_checkint(L, 2);

  ib::LuaState lua_state(L);
  lua_state.clearStack();

  lua_pushstring(L, value->group(n).c_str());
  return 1;
}

static int Regex_0(lua_State *L) {
  auto value = check_regex(L, 1);
  auto ret = value->group(0);

  ib::LuaState lua_state(L);
  lua_state.clearStack();

  lua_pushstring(L, ret.c_str());
  return 1;
}

static int Regex_1(lua_State *L) {
  auto value = check_regex(L, 1);
  auto ret = value->group(1);

  ib::LuaState lua_state(L);
  lua_state.clearStack();

  lua_pushstring(L, ret.c_str());
  return 1;
}

static int Regex_2(lua_State *L) {
  auto value = check_regex(L, 1);
  auto ret = value->group(2);

  ib::LuaState lua_state(L);
  lua_state.clearStack();

  lua_pushstring(L, ret.c_str());
  return 1;
}

static int Regex_3(lua_State *L) {
  auto value = check_regex(L, 1);
  auto ret = value->group(3);

  ib::LuaState lua_state(L);
  lua_state.clearStack();

  lua_pushstring(L, ret.c_str());
  return 1;
}

static int Regex_4(lua_State *L) {
  auto value = check_regex(L, 1);
  auto ret = value->group(4);

  ib::LuaState lua_state(L);
  lua_state.clearStack();

  lua_pushstring(L, ret.c_str());
  return 1;
}

static int Regex_5(lua_State *L) {
  auto value = check_regex(L, 1);
  auto ret = value->group(5);

  ib::LuaState lua_state(L);
  lua_state.clearStack();

  lua_pushstring(L, ret.c_str());
  return 1;
}

static int Regex_6(lua_State *L) {
  auto value = check_regex(L, 1);
  auto ret = value->group(6);

  ib::LuaState lua_state(L);
  lua_state.clearStack();

  lua_pushstring(L, ret.c_str());
  return 1;
}

static int Regex_7(lua_State *L) {
  auto value = check_regex(L, 1);
  auto ret = value->group(7);

  ib::LuaState lua_state(L);
  lua_state.clearStack();

  lua_pushstring(L, ret.c_str());
  return 1;
}

static int Regex_8(lua_State *L) {
  auto value = check_regex(L, 1);
  auto ret = value->group(8);

  ib::LuaState lua_state(L);
  lua_state.clearStack();

  lua_pushstring(L, ret.c_str());
  return 1;
}

static int Regex_9(lua_State *L) {
  auto value = check_regex(L, 1);
  auto ret = value->group(9);

  ib::LuaState lua_state(L);
  lua_state.clearStack();

  lua_pushstring(L, ret.c_str());
  return 1;
}

static int Regex_startpos(lua_State *L) {
  auto value = check_regex(L, 1);
  const auto n = luaL_checkint(L, 2);

  ib::LuaState lua_state(L);
  lua_state.clearStack();

  lua_pushnumber(L, value->getStartpos(n));
  return 1;
}
static int Regex_endpos(lua_State *L) {
  auto value = check_regex(L, 1);
  const auto n = luaL_checkint(L, 2);

  ib::LuaState lua_state(L);
  lua_state.clearStack();

  lua_pushnumber(L, value->getEndpos(n));
  return 1;
}

static int Regex_firstpos(lua_State *L) {
  auto value = check_regex(L, 1);

  ib::LuaState lua_state(L);
  lua_state.clearStack();

  lua_pushnumber(L, value->getFirstpos());
  return 1;
}

static int Regex_lastpos(lua_State *L) {
  auto value = check_regex(L, 1);

  ib::LuaState lua_state(L);
  lua_state.clearStack();

  lua_pushnumber(L, value->getLastpos());
  return 1;
}

static int Regex_bytes(lua_State *L) {
  auto value = check_regex(L, 1);

  ib::LuaState lua_state(L);
  lua_state.clearStack();

  lua_pushnumber(L, value->getBytesMatched());
  return 1;
}

static const luaL_reg Regex_methods[] = {
  {"new",            Regex_new},
  {"match",          Regex_match},
  {"search",         Regex_search},
  {"split",          Regex_split},
  {"gsub",           Regex_gsub},
  {"groups",         Regex_groups},
  {"group",          Regex_group},
  {"_0",             Regex_0},
  {"_1",             Regex_1},
  {"_2",             Regex_2},
  {"_3",             Regex_3},
  {"_4",             Regex_4},
  {"_5",             Regex_5},
  {"_6",             Regex_6},
  {"_7",             Regex_7},
  {"_8",             Regex_8},
  {"_9",             Regex_9},
  {"startpos",       Regex_startpos},
  {"endpos",         Regex_endpos},
  {"firstpos",       Regex_firstpos},
  {"lastpos",        Regex_lastpos},
  {"bytes",          Regex_bytes},
  {0,0}
};


static int Regex_gc (lua_State *L){
  auto value = to_regex(L, 1);
  if (value != nullptr) { delete value; }
  return 0;
}

static int Regex_tostring (lua_State *L) {
  auto value = to_regex(L, 1);
  lua_pushfstring(L, "<Regex pattern=%s, flags=%d", value->getPattern(), value->getFlags());
  return 1;
}

static const luaL_reg Regex_meta[] = {
  {"__gc",       Regex_gc},
  {"__tostring", Regex_tostring},
  {0, 0}
};


int Regex_register (lua_State *L){
  luaL_register(L, "Regex", Regex_methods); 
  // class methods & constants {{{
  lua_pushinteger(L, ib::Regex::NONE);
  lua_setfield(L, -2, "NONE");
  lua_pushinteger(L, ib::Regex::S);
  lua_setfield(L, -2, "S");
  lua_pushinteger(L, ib::Regex::M);
  lua_setfield(L, -2, "M");
  lua_pushinteger(L, ib::Regex::I);
  lua_setfield(L, -2, "I");
  lua_pushcfunction(L, Regex_static_escape);
  lua_setfield(L, -2, "escape");
  // class methods & constants }}}

  luaL_newmetatable(L, "Regex");
  luaL_register(L, 0, Regex_meta);
  lua_pushliteral(L, "__index");
  lua_pushvalue(L, -3);
  lua_rawset(L, -3);
  lua_pushliteral(L, "__metatable");
  lua_pushvalue(L, -3);
  lua_rawset(L, -3);
  lua_pop(L, 2); 
  return 0;
}

// }}}

// class LuaState {{{
void ib::LuaState::init() { // {{{
  const auto cfg = ib::Singleton<ib::Config>::getInstance();
  luaL_openlibs(l_);
  lua_register(l_, "_iceberg_module", &ib::luamodule::create);
  lua_register(l_, "_iceberg_config_dir", &ib::luamodule::config_dir);
  Regex_register(l_);
  if(luaL_dofile(l_, cfg->getConfigPath().c_str())) {
    fl_alert("%s", lua_tostring(l_, lua_gettop(l_)));
    exit(1);
  }
} // }}}

Fl_Color ib::LuaState::getColorFromStackTop() { // {{{
  int r, g, b;
  auto L = get();

  lua_pushinteger(L, 1);
    lua_gettable(L, -2);
    r = (int)lua_tonumber(L, -1);
  lua_pop(L, 1);

  lua_pushinteger(L, 2);
    lua_gettable(L, -2);
    g = (int)lua_tonumber(L, -1);
  lua_pop(L, 1);

  lua_pushinteger(L, 3);
    lua_gettable(L, -2);
    b = (int)lua_tonumber(L, -1);
  lua_pop(L, 1);

  return fl_rgb_color(r, g, b);
} // }}}
// }}}

// lua module {{{
int ib::luamodule::create(lua_State *L){ // {{{
  lua_newtable(L);

#define REGISTER_FUNCTION(name) \
  lua_pushcfunction(L, &ib::luamodule::name); \
  lua_setfield(L, 1, #name);

  REGISTER_FUNCTION(build_platform);
  REGISTER_FUNCTION(runtime_platform);
  REGISTER_FUNCTION(hide_application);
  REGISTER_FUNCTION(show_application);
  REGISTER_FUNCTION(do_autocomplete);
  REGISTER_FUNCTION(get_cwd);
  REGISTER_FUNCTION(set_cwd);
  REGISTER_FUNCTION(set_result_text);
  REGISTER_FUNCTION(find_command);
  REGISTER_FUNCTION(to_path);
  REGISTER_FUNCTION(message);
  REGISTER_FUNCTION(event_key);
  REGISTER_FUNCTION(event_state);
  REGISTER_FUNCTION(matches_key);
  REGISTER_FUNCTION(exit_application);
  REGISTER_FUNCTION(reboot_application);
  REGISTER_FUNCTION(scan_search_path);
  REGISTER_FUNCTION(get_input_text);
  REGISTER_FUNCTION(set_input_text);
  REGISTER_FUNCTION(get_clipboard);
  REGISTER_FUNCTION(set_clipboard);
  REGISTER_FUNCTION(get_clipboard_histories);
  REGISTER_FUNCTION(shell_execute);
  REGISTER_FUNCTION(command_execute);
  REGISTER_FUNCTION(command_output);
  REGISTER_FUNCTION(default_after_command_action);
  REGISTER_FUNCTION(add_history);
  REGISTER_FUNCTION(open_dir);
  REGISTER_FUNCTION(version);
  REGISTER_FUNCTION(selected_index);
  REGISTER_FUNCTION(utf82local);
  REGISTER_FUNCTION(local2utf8);
#ifdef IB_OS_WIN
  REGISTER_FUNCTION(list_all_windows);
#endif
  REGISTER_FUNCTION(lock);
  REGISTER_FUNCTION(unlock);
  REGISTER_FUNCTION(band);
  REGISTER_FUNCTION(bor);
  REGISTER_FUNCTION(bxor);
  REGISTER_FUNCTION(brshift);
  REGISTER_FUNCTION(blshift);
  REGISTER_FUNCTION(dirname);
  REGISTER_FUNCTION(basename);
  REGISTER_FUNCTION(directory_exists);
  REGISTER_FUNCTION(file_exists);
  REGISTER_FUNCTION(path_exists);
  REGISTER_FUNCTION(list_dir);
  REGISTER_FUNCTION(join_path);
#undef REGISTER_FUNCTION
  return 1;
} // }}}

int ib::luamodule::config_dir(lua_State *L) { // {{{
  const auto cfg = ib::Singleton<ib::Config>::getInstance();
  ib::oschar osbuf[IB_MAX_PATH];
  ib::oschar osconfig[IB_MAX_PATH];
  ib::platform::utf82oschar_b(osconfig, IB_MAX_PATH, cfg->getConfigPath().c_str());
  ib::platform::dirname(osbuf, osconfig);
  char config_dir[IB_MAX_PATH_BYTE];
  ib::platform::oschar2utf8_b(config_dir, IB_MAX_PATH_BYTE, osbuf);
  lua_pushstring(L, config_dir);
  return 1;
} // }}}

int ib::luamodule::build_platform(lua_State *L) { // {{{
  const auto cfg = ib::Singleton<ib::Config>::getInstance();
  lua_pushstring(L, cfg->getPlatform().c_str());
  return 1;
} // }}}

int ib::luamodule::runtime_platform(lua_State *L) { // {{{
  char info[128];
  ib::platform::get_runtime_platform(info);
  lua_pushstring(L, info);
  return 1;
} // }}}

int ib::luamodule::hide_application(lua_State *L) { // {{{
  ib::Singleton<ib::Controller>::getInstance()->hideApplication();
  return 0;
} // }}}

int ib::luamodule::show_application(lua_State *L) { // {{{
  ib::Singleton<ib::Controller>::getInstance()->showApplication();
  return 0;
} // }}}

int ib::luamodule::do_autocomplete(lua_State *L) { // {{{
  if(ib::Singleton<ib::ListWindow>::getInstance()->getListbox()->isAutocompleted()){
    ib::Singleton<ib::Controller>::getInstance()->completionInput();
    ib::Singleton<ib::MainWindow>::getInstance()->getInput()->scan();
  }
  return 0;
} // }}}

int ib::luamodule::get_cwd(lua_State *L) { // {{{
  lua_pushstring(L, ib::Singleton<ib::Controller>::getInstance()->getCwd().c_str());
  return 1;
} // }}}

int ib::luamodule::set_cwd(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto workdir = luaL_checkstring(L, 1);
  lua_state.clearStack();
  ib::Error error;
  if(ib::Singleton<ib::Controller>::getInstance()->setCwd(workdir, error) != 0){
    lua_pushboolean(L, false);
    lua_pushstring(L, error.getMessage().c_str());
  }else{
    lua_pushboolean(L, true);
    lua_pushnil(L);
  }
  return 2;
} // }}}

int ib::luamodule::set_result_text(lua_State *L) { // {{{
  ib::FlScopedLock fllock;
  ib::LuaState lua_state(L);
  const auto msg = luaL_checkstring(L, 1);
  ib::Singleton<ib::Controller>::getInstance()->setResultText(msg);
  lua_state.clearStack();
  return 0;
} // }}}

int ib::luamodule::find_command(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto name = luaL_checkstring(L, 1);
  lua_state.clearStack();

  auto &commands = ib::Singleton<ib::Controller>::getInstance()->getCommands();
  auto it = commands.find(name);
  if(it != commands.end()){
    lua_pushboolean(L, true);
    lua_newtable(L);
    auto bcmd = (*it).second;
    lua_pushstring(L, "name");
    lua_pushstring(L, bcmd->getName().c_str());
    lua_settable(L, -3);
    lua_pushstring(L, "workdir");
    lua_pushstring(L, bcmd->getRawWorkdir().c_str());
    lua_settable(L, -3);
    lua_pushstring(L, "description");
    lua_pushstring(L, bcmd->getDescription().c_str());
    lua_settable(L, -3);
    lua_pushstring(L, "icon");
    lua_pushstring(L, bcmd->getIconFile().c_str());
    lua_settable(L, -3);
    lua_pushstring(L, "terminal");
    lua_pushstring(L, bcmd->getTerminal().c_str());
    lua_settable(L, -3);
    lua_pushstring(L, "history");
    lua_pushboolean(L, bcmd->isEnabledHistory());
    lua_settable(L, -3);

    auto cmd = dynamic_cast<ib::Command*>(bcmd);
    if(cmd != nullptr){
      lua_pushstring(L, "path");
      lua_pushstring(L, cmd->getPath().c_str());
      lua_settable(L, -3);

      lua_pushstring(L, "cmdpath");
      lua_pushstring(L, cmd->getCommandPath().c_str());
      lua_settable(L, -3);
    }else{
      auto lcmd = dynamic_cast<ib::LuaFunctionCommand*>((*it).second);

      lua_pushstring(L, "path");
      lua_getglobal(L, "commands");
      lua_getfield(L, -1, lcmd->getName().c_str());
      lua_remove(L, -2);
      lua_getfield(L, -1, "path");
      lua_remove(L, -2);
      lua_settable(L, -3);
    }
  }else{
    lua_pushboolean(L, false);
    lua_pushfstring(L, "command '%s' not found.", name);
  }
  return 2;
} // }}}

int ib::luamodule::to_path(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto name_or_path = luaL_checkstring(L, 1);
  lua_state.clearStack();

  ib::unique_oschar_ptr osname_or_path(ib::platform::utf82oschar(name_or_path));
  ib::unique_oschar_ptr unquoted_osname_or_path(ib::platform::unquote_string(nullptr, osname_or_path.get()));
  if(ib::platform::is_path(unquoted_osname_or_path.get())){
    ib::oschar oscwd[IB_MAX_PATH];
    ib::oschar osabs_path[IB_MAX_PATH];
    char abs_path[IB_MAX_PATH_BYTE];

    ib::platform::utf82oschar_b(oscwd, IB_MAX_PATH, ib::Singleton<ib::Controller>::getInstance()->getCwd().c_str());
    ib::platform::to_absolute_path(osabs_path, oscwd, unquoted_osname_or_path.get());
    ib::platform::oschar2utf8_b(abs_path, IB_MAX_PATH_BYTE, osabs_path);
    lua_pushboolean(L, true);
    lua_pushstring(L, abs_path);
  }else{
    const auto &commands = ib::Singleton<ib::Controller>::getInstance()->getCommands();
    auto it = commands.find(name_or_path);
    if(it != commands.end()){
      auto cmd = dynamic_cast<ib::Command*>((*it).second);
      if(cmd != nullptr){
        lua_pushboolean(L, true);
        lua_pushstring(L, cmd->getCommandPath().c_str());
      }else{
        lua_pushboolean(L, true);
        lua_pushstring(L, ib::Singleton<ib::Controller>::getInstance()->getCwd().c_str());
      }
    }else{
      lua_pushboolean(L, false);
      lua_pushfstring(L, "Path '%s' does not exist.", name_or_path);
    }
  }
  return 2;
} // }}}

int ib::luamodule::message(lua_State *L) { // {{{
  ib::FlScopedLock fllock;
  ib::LuaState lua_state(L);
  const auto msg = luaL_checkstring(L, 1);
  fl_message("%s", msg);
  lua_state.clearStack();
  return 0;
} // }}}

int ib::luamodule::event_key(lua_State *L) { // {{{
  lua_pushinteger(L, Fl::event_key());
  return 1;
} // }}}

int ib::luamodule::event_state(lua_State *L) { // {{{
  lua_pushinteger(L, Fl::event_state());
  return 1;
} // }}}

int ib::luamodule::matches_key(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto string = luaL_checkstring(L, 1);
  int buf[3] = {0};
  ib::utils::parse_key_bind(buf, string);
  lua_state.clearStack();
  lua_pushboolean(L, ib::utils::matches_key(buf, Fl::event_key(), Fl::event_state()));
  return 1;
} // }}}

int ib::luamodule::exit_application(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto code = static_cast<int>(luaL_checkinteger(L, 1));
  lua_state.clearStack();
  ib::utils::exit_application(code);
  return 0;
} // }}}

int ib::luamodule::reboot_application(lua_State *L) { // {{{
  ib::utils::reboot_application();
  return 0;
} // }}}

int ib::luamodule::scan_search_path(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto category = luaL_checkstring(L, 1);
  ib::utils::scan_search_path(category);
  lua_state.clearStack();
  return 0;
} // }}}

int ib::luamodule::get_input_text(lua_State *L) { // {{{
  ib::FlScopedLock fllock;
  lua_pushstring(L, ib::Singleton<ib::MainWindow>::getInstance()->getInput()->value());
  return 1;
} // }}}

int ib::luamodule::set_input_text(lua_State *L) { // {{{
  ib::FlScopedLock fllock;
  ib::LuaState lua_state(L);
  const auto msg = luaL_checkstring(L, 1);
  ib::Singleton<ib::ListWindow>::getInstance()->getListbox()->clearAll();
  ib::Singleton<ib::ListWindow>::getInstance()->hide();
  ib::Singleton<ib::MainWindow>::getInstance()->getInput()->setValue(msg);
  lua_state.clearStack();
  return 0;
} // }}}

int ib::luamodule::get_clipboard(lua_State *L) { // {{{
  ib::FlScopedLock fllock;
  std::string ret;
  ib::utils::get_clipboard(ret);
  lua_pushstring(L, ret.c_str());
  return 1;
} // }}}

int ib::luamodule::set_clipboard(lua_State *L) { // {{{
  ib::FlScopedLock fllock;
  ib::LuaState lua_state(L);
  const auto msg = luaL_checkstring(L, 1);
  ib::utils::set_clipboard(msg);
  lua_state.clearStack();
  return 0;
} // }}}

int ib::luamodule::get_clipboard_histories(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  lua_state.clearStack();

  const auto &histories  = ib::Singleton<ib::Controller>::getInstance()->getClipboardHistories();
  int i = 1;
  lua_newtable(L);
  for(auto it = histories.rbegin(), last = histories.rend(); it != last; ++it, ++i){
    const auto &s = *it;
    lua_pushnumber(L, i);
    lua_pushstring(L, s.c_str());
    lua_settable(L, -3);
  }
  return 1;
} // }}}

int ib::luamodule::shell_execute(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  std::vector<ib::unique_string_ptr> args;
  const auto top = lua_gettop(L);
  const std::string cmd(luaL_checkstring(L, 1));
  std::string workdir;
  if(top > 2) {
    workdir = luaL_checkstring(L, -1);
    lua_pop(L, 1);
  }else{
    workdir = ib::Singleton<ib::Controller>::getInstance()->getCwd();
  }

  if(top > 1) {
    for(int i = 1;;i++){
      lua_pushinteger(IB_LUA, i); 
      lua_gettable(IB_LUA, -2);
      if(lua_isnil(IB_LUA, -1)){
        lua_pop(IB_LUA, 1);
        break;
      }
      args.push_back(ib::unique_string_ptr(new std::string(luaL_checkstring(L,-1))));
      lua_pop(IB_LUA, 1);
    }
    lua_pop(L, 1);
  }

  lua_state.clearStack();

  ib::Error error;
  if(ib::platform::shell_execute(cmd, args, workdir, "auto", error) != 0){
    lua_pushboolean(L, false);
    lua_pushstring(L, error.getMessage().c_str());
  }else{
    lua_pushboolean(L, true);
    lua_pushstring(L, "");
  }
  return 2;
} // }}}

int ib::luamodule::command_execute(lua_State *L) { // {{{
  auto controller = ib::Singleton<ib::Controller>::getInstance();
  ib::LuaState lua_state(L);
  std::vector<std::string*> args;
  const auto top = lua_gettop(L);
  const std::string cmd(luaL_checkstring(L, 1));

  if(top > 1) {
    lua_pushnil(L);
    while(lua_next(L, 2) != 0) {
      args.push_back(new std::string(luaL_checkstring(L,-1)));
      lua_pop(L, 1);
    }
  }
  lua_state.clearStack();
  const auto &commands_ = controller->getCommands();
  auto it = commands_.find(cmd);
  if(it != commands_.end()){
    ib::Error error;
    if((*it).second->execute(args, nullptr, error) != 0) {
      lua_pushboolean(L, false);
      std::string message("Failed to execute the command:");
      message += cmd;
      message += "\n";
      message += error.getMessage();
      lua_pushstring(L, message.c_str());
    }else{
      lua_pushboolean(L, true);
      lua_pushstring(L, "");
    }
  }else{
    lua_pushboolean(L, false);
    std::string message("Command not found:");
    message += cmd;
    lua_pushstring(L, message.c_str());
  }
  ib::utils::delete_pointer_vectors(args);
  return 2;
} // }}}

int ib::luamodule::command_output(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto cmd = luaL_checkstring(L, 1);
  std::string sstdout, sstderr;
  ib::Error error;

  lua_state.clearStack();
  if(ib::platform::command_output(sstdout, sstderr, cmd, error) != 0){
    lua_pushboolean(L, false);
  }else{
    lua_pushboolean(L, true);
  }

  lua_pushstring(L, sstdout.c_str());
  lua_pushstring(L, sstderr.c_str());
  return 3;
} // }}}

int ib::luamodule::default_after_command_action(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  luaL_checktype(L, 1, LUA_TBOOLEAN);
  const auto success = lua_toboolean(L, 1);
  const auto message = luaL_checkstring(L, 2);
  ib::Singleton<ib::Controller>::getInstance()->afterExecuteCommand(success, strlen(message) != 0 ? message : nullptr);
  lua_state.clearStack();
  return 0;
} // }}}

int ib::luamodule::add_history(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto top = lua_gettop(L);
  std::string name = "";
  if(top > 1) {
    name = luaL_checkstring(L, -1);
    lua_pop(L, 1);
  }
  auto input = std::string(luaL_checkstring(L, -1));
  lua_state.clearStack();

  if(name.empty()){
    ib::Singleton<ib::History>::getInstance()->addRawInputHistory(input);
  }else{
    auto it = ib::Singleton<ib::Controller>::getInstance()->getCommands().find(name);
    if(it != ib::Singleton<ib::Controller>::getInstance()->getCommands().end()){
      ib::Singleton<ib::History>::getInstance()->addBaseCommandHistory(input, (*it).second);
    }
  }
  return 0;
} // }}}

int ib::luamodule::open_dir(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto path = luaL_checkstring(L, 1);
  ib::Error error;

  lua_state.clearStack();
  std::string directory(path);
  if(ib::utils::open_directory(directory, error) != 0){
    lua_pushboolean(L, false);
    lua_pushstring(L, error.getMessage().c_str());
  }else{
    lua_pushboolean(L, true);
    lua_pushnil(L);
  }
  return 2;
} // }}}

int ib::luamodule::version(lua_State *L) { // {{{
  char buf[128];
  sprintf(buf, "iceberg %s %s (%s)", ib::Singleton<ib::Config>::getInstance()->getPlatform().c_str(), IB_VERSION, __DATE__);
  lua_pushstring(L, buf);
  return 1;
} // }}}

int ib::luamodule::selected_index(lua_State *L) { // {{{
  lua_pushinteger(L, ib::Singleton<ib::ListWindow>::getInstance()->getListbox()->value());
  return 1;
} // }}}

int ib::luamodule::utf82local(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto text = luaL_checkstring(L, 1);
  ib::unique_char_ptr ret(ib::platform::utf82local(text));

  lua_state.clearStack();
  lua_pushstring(L, ret.get());
  return 1;
} // }}}

int ib::luamodule::local2utf8(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto text = luaL_checkstring(L, 1);
  ib::unique_char_ptr ret(ib::platform::local2utf8(text));

  lua_state.clearStack();
  lua_pushstring(L, ret.get());
  return 1;
} // }}}

// int ib::luamodule::list_all_windows(lua_State *L) { // {{{
#ifdef IB_OS_WIN
static void list_all_windows_iter(std::vector<ib::whandle> &result, const ib::whandle parent){
  auto hwnd = FindWindowEx(parent, NULL, NULL, NULL);
  result.push_back(hwnd);
  while (hwnd != NULL){
    list_all_windows_iter(result, hwnd);
    hwnd = FindWindowEx(parent, hwnd, NULL, NULL);
    result.push_back(hwnd);
  }
}

void list_all_windows(std::vector<ib::whandle> &result){
  list_all_windows_iter(result, GetDesktopWindow());
}

int ib::luamodule::list_all_windows(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  std::vector<ib::whandle> result;
  list_all_windows(result);
  lua_state.clearStack();
  lua_newtable(L);
  int i = 1;
  for(auto it = result.begin(), last = result.end(); it != last; ++it, ++i){
    lua_pushinteger(L, i);
#if defined IB_64BIT
    lua_pushnumber(L, (lua_Number)((long long)(*it)));
#else
    lua_pushnumber(L, (long)(*it));
#endif
    lua_settable(L, -3);
  }
  return 1;
} // }}}
#endif
// }}}

int ib::luamodule::lock(lua_State *L) { // {{{
  Fl::lock();
  return 0;
} // }}}

int ib::luamodule::unlock(lua_State *L) { // {{{
  Fl::unlock();
  return 0;
} // }}}

int ib::luamodule::band(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto argc = lua_gettop(L);
  lua_Integer value = luaL_checkinteger(L, 1);
  for(int i = 2; i <= argc; ++i){
    value = value & luaL_checkinteger(L, i);
  }
  lua_state.clearStack();
  lua_pushnumber(L, value);
  return 1;
} // }}}

int ib::luamodule::bor(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto argc = lua_gettop(L);
  lua_Integer value = luaL_checkinteger(L, 1);
  for(int i = 2; i <= argc; ++i){
    value = value | luaL_checkinteger(L, i);
  }
  lua_state.clearStack();
  lua_pushnumber(L, value);
  return 1;
} // }}}

int ib::luamodule::bxor(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto argc = lua_gettop(L);
  auto value = luaL_checkinteger(L, 1);
  for(int i = 2; i <= argc; ++i){
    value = value ^ luaL_checkinteger(L, i);
  }
  lua_state.clearStack();
  lua_pushnumber(L, value);
  return 1;
} // }}}

int ib::luamodule::brshift(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  auto value = luaL_checkinteger(L, 1);
  auto disp = luaL_checkint(L, 2);
  lua_state.clearStack();
  lua_pushnumber(L, value >> disp);
  return 1;
} // }}}

int ib::luamodule::blshift(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  auto value = luaL_checkinteger(L, 1);
  auto disp = luaL_checkint(L, 2);
  lua_state.clearStack();
  lua_pushnumber(L, value << disp);
  return 1;
} // }}}

int ib::luamodule::dirname(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto path = luaL_checkstring(L, 1);
  lua_state.clearStack();
  ib::oschar ospath[IB_MAX_PATH];
  ib::platform::utf82oschar_b(ospath, IB_MAX_PATH, path);
  ib::oschar osdirname[IB_MAX_PATH];
  ib::platform::dirname(osdirname, ospath);
  char dirname[IB_MAX_PATH_BYTE];
  ib::platform::oschar2utf8_b(dirname, IB_MAX_PATH_BYTE, osdirname);

  lua_pushstring(L, dirname);
  return 1;
} // }}}

int ib::luamodule::basename(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto path = luaL_checkstring(L, 1);
  lua_state.clearStack();
  ib::oschar ospath[IB_MAX_PATH];
  ib::platform::utf82oschar_b(ospath, IB_MAX_PATH, path);
  ib::oschar osbasename[IB_MAX_PATH];
  ib::platform::basename(osbasename, ospath);
  char basename[IB_MAX_PATH_BYTE];
  ib::platform::oschar2utf8_b(basename, IB_MAX_PATH_BYTE, osbasename);

  lua_pushstring(L, basename);
  return 1;
} // }}}

int ib::luamodule::directory_exists(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto path = luaL_checkstring(L, 1);
  lua_state.clearStack();
  ib::oschar ospath[IB_MAX_PATH];
  ib::platform::utf82oschar_b(ospath, IB_MAX_PATH, path);
  lua_pushboolean(L, ib::platform::directory_exists(ospath));
  return 1;
} // }}}

int ib::luamodule::file_exists(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto path = luaL_checkstring(L, 1);
  lua_state.clearStack();
  ib::oschar ospath[IB_MAX_PATH];
  ib::platform::utf82oschar_b(ospath, IB_MAX_PATH, path);
  lua_pushboolean(L, ib::platform::file_exists(ospath));
  return 1;
} // }}}

int ib::luamodule::path_exists(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto path = luaL_checkstring(L, 1);
  lua_state.clearStack();
  ib::oschar ospath[IB_MAX_PATH];
  ib::platform::utf82oschar_b(ospath, IB_MAX_PATH, path);
  lua_pushboolean(L, ib::platform::path_exists(ospath));
  return 1;
} // }}}

int ib::luamodule::list_dir(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto path = luaL_checkstring(L, 1);
  lua_state.clearStack();
  ib::oschar ospath[IB_MAX_PATH];
  ib::platform::utf82oschar_b(ospath, IB_MAX_PATH, path);

  char file[IB_MAX_PATH_BYTE];
  std::vector<ib::unique_oschar_ptr> files;
  ib::Error error;
  if(ib::platform::walk_dir(files, ospath, error, false) != 0){
    lua_pushboolean(L, false);
    lua_pushstring(L, error.getMessage().c_str());
  }else{
    lua_pushboolean(L, true);
    lua_newtable(L);
    int i = 1;
    for(auto it = files.begin(), last = files.end(); it != last; ++it, ++i){
      lua_pushnumber(L, i);
      ib::platform::oschar2utf8_b(file, IB_MAX_PATH_BYTE, (*it).get());
      lua_pushstring(L, file);
      lua_settable(L, -3);
    }
  }
  return 2;
} // }}}

int ib::luamodule::join_path(lua_State *L) { // {{{
  ib::LuaState lua_state(L);
  const auto argc = lua_gettop(L);
  const auto base = luaL_checkstring(L, 1);
  ib::oschar path_ret[IB_MAX_PATH];
  ib::oschar path_a[IB_MAX_PATH];
  ib::oschar path_b[IB_MAX_PATH];
  ib::platform::utf82oschar_b(path_a, IB_MAX_PATH, base);

  for(int i = 2; i <= argc; ++i){
    const auto child = luaL_checkstring(L, i);
    ib::platform::utf82oschar_b(path_b, IB_MAX_PATH, child);
    ib::platform::join_path(path_ret, path_a, path_b);
    memcpy(path_a, path_ret, IB_MAX_PATH);
  }

  char ret[IB_MAX_PATH_BYTE];
  ib::platform::oschar2utf8_b(ret, IB_MAX_PATH_BYTE, path_ret);

  lua_state.clearStack();
  lua_pushstring(L, ret);

  return 1;
} // }}}

// }}}

