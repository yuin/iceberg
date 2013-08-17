#include "ib_completer.h"
#include "ib_platform.h"
#include "ib_ui.h"
#include "ib_lua.h"
#include "ib_controller.h"
#include "ib_regex.h"
#include "ib_history.h"
#include "ib_config.h"
#include "ib_migemo.h"

// class Completer {{{
void ib::Completer::completeHistory(std::vector<ib::CompletionValue*> &candidates, const std::string &value){ // {{{
  method_history_->beforeMatch(candidates, value);

  auto &history = ib::History::inst();

  auto commands = history.getOrderedCommands();
  const double average = history.getAverageScore();
  const double se     = history.calcScoreSe();
  std::map<std::string, bool> found;
  for(auto it = commands.rbegin(), last = commands.rend(); it != last; ++it){
    ib::HistoryCommand *cmd = (*it);
    if(method_history_->match(cmd->getPath(), value) > -1){
      if(found.find(cmd->getPath()) == found.end()){
        cmd->setScore(history.calcScore(cmd->getPath(), average, se));
        candidates.push_back(cmd);
      }
      found[cmd->getPath()] = true;
    }
  }

  method_history_->afterMatch(candidates, value);

} // }}}

void ib::Completer::completeOption(std::vector<ib::CompletionValue*> &candidates, const std::string &command) { // {{{
  if(!hasCompletionFunc(command)) return;
  const auto &input = ib::MainWindow::inst()->getInput()->getCursorValue();
  const auto token = ib::MainWindow::inst()->getInput()->getCursorToken();
  method_option_->beforeMatch(candidates, input);
  
  const int start = lua_gettop(IB_LUA);
  lua_getglobal(IB_LUA, "system");
  lua_getfield(IB_LUA, -1, "completer");
  lua_getfield(IB_LUA, -1, "option_func");
  lua_getfield(IB_LUA, -1, command.c_str());
  lua_newtable(IB_LUA);
  const int top = lua_gettop(IB_LUA);
  int i = 1;
  auto &args = ib::MainWindow::inst()->getInput()->getParamValues();
  for (auto it = args.begin(), last = args.end(); it != last; ++it, ++i) {
      lua_pushinteger(IB_LUA, i);
      lua_pushstring(IB_LUA, (*it)->c_str());
      lua_settable(IB_LUA, top);
  }

  if(lua_pcall(IB_LUA, 1, 1, 0) != 0) {
    fl_alert("%s", lua_tostring(IB_LUA, lua_gettop(IB_LUA)));
    return;
  }

  for(i = 1;;i++){
    lua_pushinteger(IB_LUA, i); 
    lua_gettable(IB_LUA, -2); 
    if(lua_isnil(IB_LUA, -1)){
      lua_pop(IB_LUA, 1);
      break;
    }
    switch(lua_type(IB_LUA, -1)) {
      case LUA_TSTRING: {
          const char *value = luaL_checkstring(IB_LUA, -1);
          if(!token->isValueToken() || method_option_->match(value, input) > -1){
            candidates.push_back(new ib::CompletionString(value));
          }
        }
        break;

      case LUA_TTABLE: {
          lua_getfield(IB_LUA, -1, "value");
          const char *value = luaL_checkstring(IB_LUA, -1);
          if(!token->isValueToken() || method_option_->match(value, input) > -1){
            ib::CompletionString *compstr = new ib::CompletionString(value);
            lua_pop(IB_LUA, 1);

            lua_getfield(IB_LUA, -1, "description");
            if(!lua_isnil(IB_LUA, -1)){
              compstr->setDescription(luaL_checkstring(IB_LUA, -1));
            }
            lua_pop(IB_LUA, 1);

            lua_getfield(IB_LUA, -1, "icon");
            if(!lua_isnil(IB_LUA, -1)){
              compstr->setIconFile(luaL_checkstring(IB_LUA, -1));
            }
            lua_pop(IB_LUA, 1);
            candidates.push_back(compstr);
          }else{
            lua_pop(IB_LUA, 1);
          }
        }
        break;

      default:
        ;
    }
    lua_pop(IB_LUA, 1);
  }
 
  lua_pop(IB_LUA, lua_gettop(IB_LUA) - start);
  method_option_->afterMatch(candidates, input);
} // }}}

void ib::Completer::completePath(std::vector<ib::CompletionValue*> &candidates, const std::string &value) { // {{{

  candidates.clear();
  ib::oschar os_value[IB_MAX_PATH];
  ib::platform::utf82oschar_b(os_value, IB_MAX_PATH, value.c_str());
  if(!ib::platform::is_path(os_value)) {
    return;
  }

  ib::oschar os_dirname[IB_MAX_PATH];
  ib::platform::dirname(os_dirname, os_value);

  ib::oschar os_basename[IB_MAX_PATH];
  ib::platform::basename(os_basename, os_value);

  char dirname[IB_MAX_PATH_BYTE];
  ib::platform::oschar2utf8_b(dirname, IB_MAX_PATH_BYTE, os_dirname);
  char basename[IB_MAX_PATH_BYTE];
  ib::platform::oschar2utf8_b(basename, IB_MAX_PATH_BYTE, os_basename);
  size_t basename_length = strlen(basename);
  const bool is_empty_basename = basename_length == 0;

  method_path_->beforeMatch(candidates, basename);

  std::vector<ib::unique_oschar_ptr> files;
  ib::Error error;
  if(ib::platform::walk_dir(files, os_dirname, error, false) == 0) {
    char compvalue[IB_MAX_PATH_BYTE];
    for(auto it = files.begin(), last = files.end(); it != last; ++it){
        ib::platform::oschar2utf8_b(compvalue, IB_MAX_PATH_BYTE, (*it).get());
        if(is_empty_basename || method_path_->match(compvalue, basename) > -1){
          candidates.push_back(new ib::CompletionPathParts(dirname, compvalue));
        }
    }
  }

  method_path_->afterMatch(candidates, basename);
} // }}}

void ib::Completer::completeCommand(std::vector<ib::CompletionValue*> &candidates, const std::string &value){ // {{{
  auto &controller = ib::Controller::inst();
  method_command_->beforeMatch(candidates, value);
  double score;

  if(candidates.size() == 0){
    for(auto it = controller.getCommands().begin(), last = controller.getCommands().end(); it != last; ++it){
      (*it).second->setScore(0.0);
      score = method_command_->match((*it).second->getName(), value);
      if(score > -1) {
        (*it).second->setScore(score);
        candidates.push_back((*it).second);
      }
    }
  }else{
    for(auto it = candidates.begin(); it != candidates.end();){
      ib::BaseCommand *base_command = dynamic_cast<ib::BaseCommand*>(*it);
      if(base_command != 0){
        base_command->setScore(0.0);
        score = method_command_->match(base_command->getName(), value);
        if(score > -1){
          base_command->setScore(score);
          ++it;
          continue;
        }
      }
      it = candidates.erase(it);
    }
  }

  auto &history = ib::History::inst();
  const double average = history.getAverageScore();
  const double se      = history.calcScoreSe();
  const double hfactor  = ib::Config::inst().getHistoryFactor();
  const double rfactor  = 1 - hfactor;

  for(auto it = candidates.begin(); it != candidates.end(); ++it){
    ib::BaseCommand *base_command = dynamic_cast<ib::BaseCommand*>(*it);
    if(base_command != 0) {
      double hist_score = history.calcScore(base_command->getName(), average, se);
      base_command->setScore(base_command->getScore()*rfactor + hist_score * hfactor);
    }
  }

  method_command_->afterMatch(candidates, value);
} // }}}
// }}}

// class CompletionMethodMigemoMixin {{{
void ib::CompletionMethodMigemoMixin::beforeMatch(std::vector<ib::CompletionValue*> &candidates, const std::string &input) { // {{{
  auto &migemo = ib::Migemo::inst();
  if(migemo.isEnable() && input.size() >= ib::Migemo::MIN_LENGTH){
    unsigned char *pattern = migemo.query((unsigned char*)input.c_str());
    regex_ = new ib::Regex((char*)pattern, ib::Regex::NONE);
    migemo.release(pattern);

    if(regex_->init() != 0){
      delete regex_;
      regex_ = 0;
    }else{
      candidates.clear();
    }
  }
} // }}}

double ib::CompletionMethodMigemoMixin::match(const std::string &name, const std::string &input) { // {{{
  return -1;
} // }}}

void ib::CompletionMethodMigemoMixin::afterMatch(std::vector<ib::CompletionValue*> &candidates, const std::string &input) { // {{{
  if(regex_ != 0) { 
    delete regex_; 
    regex_ = 0;
  }
} // }}}
// }}}

// class BeginsWithMatchCompletionMethod  {{{
double ib::BeginsWithMatchCompletionMethod::match(const std::string &name, const std::string &input) {
  if(regex_ != 0){
    if(regex_->match(name) == 0) return 0.0;
  }

  if(name.size() == 0 || input.size() == 0) return -1;
  if(input.size() > name.size()) return -1;
  return strcasestr(name.c_str(), input.c_str()) == name.c_str() ? 0.0 : -1;
}
// }}}

// class PartialMatchCompletionMethod  {{{
double ib::PartialMatchCompletionMethod::match(const std::string &name, const std::string &input) {
  if(regex_ != 0){
    if(regex_->search(name) == 0) return 0.0;
  }

  if(name.size() == 0 || input.size() == 0) return -1;
  if(input.size() > name.size()) return -1;
  return strcasestr(name.c_str(), input.c_str()) != 0 ? 0.0 : -1;
}
// }}}

// class AbbrMatchCompletionMethod  {{{
void ib::AbbrMatchCompletionMethod::beforeMatch(std::vector<ib::CompletionValue*> &candidates, const std::string &input) { // {{{
} // }}}

double ib::AbbrMatchCompletionMethod::match(const std::string &name, const std::string &input) { // {{{
  int cmd_utf8len = 0,
      input_utf8len = 0,
      pre_utf8len = 0;
  std::size_t cmd_len = name.size(),
              input_len = input.size(),
              cmd_ptr = 0,
              input_ptr = 0;

  const char *cmd_str = name.c_str();
  const char *input_str = input.c_str();
  double score = 0;
  bool  match = false;
  if(cmd_len == 0 || input_len == 0) goto notmatch;
  if(input_len > cmd_len) goto notmatch;

  while(cmd_ptr != cmd_len){
    match = false;
    cmd_utf8len = ib::utils::utf8len(cmd_str[cmd_ptr]);
    input_utf8len = ib::utils::utf8len(input_str[input_ptr]);
    if(cmd_utf8len != input_utf8len){
    }else if(cmd_utf8len == 1){
      const char cmdc = tolower(cmd_str[cmd_ptr]);
      const char inputc = tolower(input_str[input_ptr]);
      if(cmdc == inputc){
        score += cmd_ptr == 0 ? 1.5 : 0.9;
        input_ptr += input_utf8len;
        match = true;
      }
    }else{
      if(memcmp(cmd_str+cmd_ptr, input_str+input_ptr, cmd_utf8len) == 0){
        score += cmd_ptr == 0 ? 1.5 : 0.9;
        input_ptr += input_utf8len;
        match = true;
      }
    }

    if(!match && score != 0){
      if(cmd_utf8len == 0 && (cmd_str[cmd_ptr] == '_' || isspace(cmd_str[cmd_ptr]))){
      }else if(input_ptr == input_len) { score += 0.8; }
      else{score += 0.75;}
    }else if(match && pre_utf8len != 0){
      if(cmd_utf8len == 1){
        const char precmdc = cmd_str[cmd_ptr - pre_utf8len];
        if(islower(precmdc) && isupper(cmd_str[cmd_ptr])) {
          score += 0.1;
        }else if(precmdc == '_' || isspace(precmdc)){
          score += 0.1;
        }
      }
    }

    cmd_ptr += cmd_utf8len;
    pre_utf8len = cmd_utf8len;
  }

  if(input_ptr == input_len){
    return std::min(1.0, score / cmd_len);
  }else{
notmatch:
    return -1;
  }
} // }}}

void ib::AbbrMatchCompletionMethod::afterMatch(std::vector<ib::CompletionValue*> &candidates, const std::string &input) { // {{{
} // }}}

// }}}

