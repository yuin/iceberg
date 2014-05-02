#include "ib_comp_value.h"
#include "ib_config.h"
#include "ib_utils.h"
#include "ib_platform.h"
#include "ib_controller.h"
#include "ib_lexer.h"
#include "ib_lua.h"
#include "ib_icon_manager.h"


// class CompletionPathParts {{{
ib::CompletionPathParts::CompletionPathParts(const char *dirname, const char *basename) : dirname_(dirname), basename_(basename), description_(""), path_("") {
  ib::oschar wdir[IB_MAX_PATH] ;
  ib::oschar wbase[IB_MAX_PATH] ;
  ib::oschar wpath[IB_MAX_PATH] ;

  ib::platform::utf82oschar_b(wdir, IB_MAX_PATH, dirname_.c_str());
  ib::platform::utf82oschar_b(wbase, IB_MAX_PATH, basename_.c_str());
  ib::platform::normalize_join_path(wpath, wdir, wbase);

  char path[IB_MAX_PATH_BYTE];
  ib::platform::oschar2utf8_b(path, IB_MAX_PATH_BYTE, wpath);
  path_ = path;
}

const std::string* ib::CompletionPathParts::getContextMenuPath() const { // {{{
  return &path_;
} // }}}

bool ib::CompletionPathParts::isAutocompleteEnable() const { // {{{
  return ib::Config::inst().getPathAutocomplete();
} // }}}

Fl_RGB_Image* ib::CompletionPathParts::loadIcon(const int size) { // {{{
  return ib::IconManager::inst()->getAssociatedIcon(path_.c_str(), size, true);
} // }}}
// }}}

// class CompletionString {{{
const std::string* ib::CompletionString::getContextMenuPath() const { // {{{
  return 0;
} // }}}

bool ib::CompletionString::isAutocompleteEnable() const { // {{{
  return ib::Config::inst().getOptionAutocomplete();
} // }}}

Fl_RGB_Image* ib::CompletionString::loadIcon(const int size) { // {{{
  ib::oschar ospath[IB_MAX_PATH];
  if(icon_file_.empty()){
    ib::platform::utf82oschar_b(ospath, IB_MAX_PATH, value_.c_str());
    if(ib::platform::is_path(ospath)){
      return ib::IconManager::inst()->getAssociatedIcon(value_.c_str(), size, true);
    }else{
      return 0;
    }
  }else{
    ib::oschar ospath[IB_MAX_PATH];
    ib::platform::utf82oschar_b(ospath, IB_MAX_PATH, icon_file_.c_str());
    if(ib::platform::is_path(ospath)){
      return ib::IconManager::inst()->readFileIcon(icon_file_.c_str(), size);
    }else{
      return 0;
    }
  }
} // }}}
// }}}

//class BaseCommand {{{
const std::string& ib::BaseCommand::getWorkdir() {
  if(is_dynamic_workdir_){
    lua_getglobal(IB_LUA, "commands");
    lua_getfield(IB_LUA, -1, getName().c_str());
    lua_getfield(IB_LUA, -1, "workdir");
    if(lua_pcall(IB_LUA, 0, 1, 0) != 0) {
      fl_alert("Failed to get workdir.");
    }else{
      setWorkdir(luaL_checkstring(IB_LUA, -1));
    }
  }

  if(workdir_ == "."){
    return Controller::inst().getCwd();
  }
  return workdir_;
}
// }}}

// class Command {{{
void ib::Command::init() { // {{{
  if(initialized_) return;
  initialized_ = true;
  ib::CommandLexer lexer;
  lexer.parse(path_.c_str());
  command_path_ += lexer.getFirstValue();
  if(description_.size() == 0){
    setDescription(path_);
  }
  if(workdir_ == "") {
    ib::oschar os_dirname[IB_MAX_PATH];
    ib::oschar os_command_path[IB_MAX_PATH];
    ib::platform::utf82oschar_b(os_command_path, IB_MAX_PATH, command_path_.c_str());
    ib::platform::dirname(os_dirname, os_command_path);
    char dirname[IB_MAX_PATH_BYTE];
    ib::platform::oschar2utf8_b(dirname, IB_MAX_PATH_BYTE, os_dirname);
    setWorkdir(dirname);
  }
  ib::platform::on_command_init(this);
} // }}}

const std::string* ib::Command::getContextMenuPath() const { // {{{
  return &getCommandPath();
} // }}}

Fl_RGB_Image* ib::Command::loadIcon(const int size) { // {{{
  if(icon_file_.empty()){
    return ib::IconManager::inst()->getAssociatedIcon(command_path_.c_str(), size, true);
  }else{
    return ib::IconManager::inst()->readFileIcon(icon_file_.c_str(), size);
  }
} // }}}

int ib::Command::execute(const std::vector<std::string*> &args, const std::string *workdir, ib::Error &error) { // {{{
  ib::string_map values;
  char buf[32];
  int ret = 0;

  int i = 1;
  for(auto it = args.begin(), last = args.end(); it != last && i < IB_MAX_ARGS; ++it, ++i){
    sprintf(buf, "%d", i);
    values.insert(ib::string_pair(std::string(buf), **it));
  }
  std::string clipboard;
  ib::utils::get_clipboard(clipboard);
  values.insert(ib::string_pair("CL", clipboard));

  ib::CommandLexer lexer;
  lexer.parse(path_.c_str());
  std::vector<std::string*> command_params;
  const std::vector<std::string*> &params = lexer.getParamValues();
  bool user_args_flag = false;

  for(auto it = params.begin(), last = params.end(); it != last; ++it){
    std::string *before = *it;
    std::string *after = new std::string;
    ib::utils::expand_vars(*after, *before, values);
    if(!user_args_flag && (*before != *after)) user_args_flag = true;
    command_params.push_back(after);
  }

  std::string command;
  ib::utils::expand_vars(command, getCommandPath(), values);
  if(!user_args_flag && (command != getCommandPath())) user_args_flag = true; 
  if(!user_args_flag){
    i = 1;
    for(auto it = args.begin(), last = args.end(); it != last && i < IB_MAX_ARGS; ++it, ++i){
      std::string *new_value = new std::string(*(*it));
      command_params.push_back(new_value);
    }
  }

  ib::oschar oscommand[IB_MAX_PATH];
  ib::platform::utf82oschar_b(oscommand, IB_MAX_PATH, command.c_str());

  std::string wd;
  if(workdir == 0){
    wd = getWorkdir();
  }else{
    wd = *workdir;
  }

  ib::Error e;
  if(ib::platform::directory_exists(oscommand)){
    if(ib::utils::open_directory(command, e) != 0){
      error.setCode(e.getCode());
      error.setMessage(e.getMessage());
      ret = 1;
    }
  }else if(ib::platform::shell_execute(command, command_params, wd, e) != 0){
    error.setCode(e.getCode());
    error.setMessage(e.getMessage());
    ret = 1;
  };
  ib::utils::delete_pointer_vectors(command_params);
  return ret;
} // }}}
// }}}

//class LuaFunctionCommand {{{
const std::string* ib::LuaFunctionCommand::getContextMenuPath() const { // {{{
  return 0;
} // }}}

Fl_RGB_Image* ib::LuaFunctionCommand::loadIcon(const int size) { // {{{
  if(icon_file_.empty()){
    return ib::IconManager::inst()->getLuaIcon(size);
  }else{
    return ib::IconManager::inst()->readPngFileIcon(icon_file_.c_str(), size);
  }
} // }}}

void ib::LuaFunctionCommand::init() { // {{{
  if(description_.size() == 0){
    setDescription("Lua function");
  }
  if(workdir_ == ""){
    workdir_ = ".";
  }
} // }}}

int ib::LuaFunctionCommand::execute(const std::vector<std::string*> &args, const std::string *workdir, ib::Error &error){ // {{{

  std::string wd;
  if(workdir == 0){
    wd = getWorkdir();
  }else{
    wd = *workdir;
  }
  std::string old_workdir = ib::Controller::inst().getCwd();
  ib::Error cderror;
  if(getName() != ":cd") {
    ib::Controller::inst().setCwd(wd, cderror);
  }
  // ignore errors;

  const int start = lua_gettop(IB_LUA);
  lua_getglobal(IB_LUA, "commands");
  lua_getfield(IB_LUA, -1, getName().c_str());
  lua_getfield(IB_LUA, -1, "path");

  lua_newtable(IB_LUA);
  const int top = lua_gettop(IB_LUA);
  int i = 1;
  for (auto it = args.begin(), last = args.end(); it != last; ++it, ++i) {
      lua_pushinteger(IB_LUA, i);
      lua_pushstring(IB_LUA, (*it)->c_str());
      lua_settable(IB_LUA, top);
  }

  int ret = 0;
  if(lua_pcall(IB_LUA, 1, 1, 0) != 0) {
    error.setCode(1);
    error.setMessage(lua_tostring(IB_LUA, -1));
    ret = 1;
  }else{
    ret = (int)lua_tointeger(IB_LUA, -1);
  }
  lua_pop(IB_LUA, start);

  if(getName() != ":cd") {
    ib::Controller::inst().setCwd(old_workdir, cderror);
    // ignore errors;
  }
  return ret;
} // }}}
// }}}

// class HistoryCommand {{{
void ib::HistoryCommand::init() { // {{{
  if(initialized_) return;
  initialized_ = true;
  auto &commands = ib::Controller::inst().getCommands();
  auto it = commands.find(name_);
  if(it != commands.end()) {
    org_cmd_ = (*it).second;
    workdir_ = org_cmd_->getRawWorkdir();
    description_ = org_cmd_->getDescription();
  }else{
    ib::CommandLexer lexer;
    lexer.parse(path_.c_str());
    command_path_ += lexer.getFirstValue();
  }
} // }}}

int ib::HistoryCommand::execute(const std::vector<std::string*> &args, const std::string *workdir, ib::Error &error) { // {{{
  // should not be called.
  return 1;
} // }}}

const std::string* ib::HistoryCommand::getContextMenuPath() const { // {{{
  if(org_cmd_){
    return org_cmd_->getContextMenuPath();
  }
  return &command_path_;
} // }}}

Fl_RGB_Image* ib::HistoryCommand::loadIcon(const int size) { // {{{
  if(org_cmd_){
    return org_cmd_->loadIcon(size);
  }
  return ib::IconManager::inst()->getAssociatedIcon(command_path_.c_str(), size, true);
} // }}}
// }}}

