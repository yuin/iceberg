#include "ib_history.h"
#include "ib_config.h"
#include "ib_comp_value.h"
#include "ib_platform.h"
#include "ib_regex.h"

void ib::History::load() { // {{{
  auto &cfg = ib::Config::inst();
  ib::unique_oschar_ptr oshistory_path(ib::platform::utf82oschar(cfg.getHistoryPath().c_str()));
  if(!ib::platform::file_exists(oshistory_path.get())) return;

  ib::unique_char_ptr lohistory_path(ib::platform::utf82local(cfg.getHistoryPath().c_str()));
  std::ifstream ifs(lohistory_path.get());
  std::string buf;
  ib::Regex re("\t", ib::Regex::NONE);
  re.init();
  std::vector<std::string*> fields;

  while(ifs && getline(ifs, buf)) {
    re.split(fields, buf);
    ib::HistoryCommand *cmd = new HistoryCommand();
    cmd->setName(*(fields.at(0)));
    cmd->setPath(*(fields.at(1)));
    cmd->addTime((std::time_t)atoi(fields.at(2)->c_str()));
    addCommand(cmd);
    ib::utils::delete_pointer_vectors(fields);
  }

  ifs.close();
} // }}}

void ib::History::dump() { // {{{
  auto &cfg = ib::Config::inst();
  ib::Error error;

  ib::oschar osbkfile[IB_MAX_PATH];
  ib::oschar osfile[IB_MAX_PATH];
  ib::platform::utf82oschar_b(osbkfile, IB_MAX_PATH, (cfg.getHistoryPath()+".bk").c_str());
  ib::platform::utf82oschar_b(osfile, IB_MAX_PATH, cfg.getHistoryPath().c_str());
  ib::platform::remove_file(osbkfile, error); // ignore errors
  ib::platform::copy_file(osfile, osbkfile, error); // ignore errors

  ib::unique_char_ptr lohistory_path(ib::platform::utf82local(cfg.getHistoryPath().c_str()));
  std::ofstream ofs(lohistory_path.get());
  auto it = ordered_commands_.begin();
  if(ordered_commands_.size() > cfg.getMaxHistories()){
    std::advance(it, ordered_commands_.size() - cfg.getMaxHistories());
  }

  for(auto last = ordered_commands_.end(); it != last; ++it){
    ib::HistoryCommand *cmd = *it;
    ofs << cmd->getName() << "\t" << cmd->getPath() << "\t" << cmd->getTimes().at(0) << std::endl;
  }
  ofs.close();
  /*
  size_t file_size = 0;
  ib::platform::file_size(file_size, osfile, error);
  if(file_size == 0){
    ib::platform::copy_file(osbkfile, osfile, error); // ignore errors
  }
  ib::platform::remove_file(osbkfile, error); // ignore errors
  */
} // }}}

void ib::History::addCommand(ib::HistoryCommand *command) { // {{{
  command->init();
  ib::HistoryCommand *cmd = command;
  if(commands_.find(command->getPath()) == commands_.end()){
    commands_[command->getPath()] = command;
  }else{
    commands_[command->getPath()]->addTime(command->getTimes().at(0));
    cmd = commands_[command->getPath()];
  }
  ordered_commands_.push_back(command);

  total_score_ -= cmd->getRawScore();
  calcRawScore(cmd);
  total_score_ += cmd->getRawScore();
} // }}}

void ib::History::addBaseCommandHistory(const std::string &value, const ib::BaseCommand* cmd){ // {{{
   ib::HistoryCommand *hcmd = new HistoryCommand();
   hcmd->setName(cmd->getName());
   hcmd->setPath(value);
   hcmd->addTime(std::time(0));
   addCommand(hcmd);
} // }}}

void ib::History::addRawInputHistory(const std::string &value) { // {{{
   ib::HistoryCommand *hcmd = new HistoryCommand();
   hcmd->setName(value);
   hcmd->setPath(value);
   hcmd->addTime(std::time(0));
   addCommand(hcmd);
} // }}}

void ib::History::calcRawScore(ib::HistoryCommand *command){ // {{{
  int score = 0;
  std::time_t now = std::time(0);
  const std::vector<std::time_t> &times = command->getTimes();
  for(auto it = times.begin(), last = times.end(); it != last; ++it){
    std::time_t diff = now - (*it);
    if(diff < 60*15){
      score += 8;
    }else if(diff < 60*60){
      score += 6;
    }else if(diff < 60*60*6){
      score += 5;
    }else if(diff < 60*60*24){
      score += 4;
    }else if(diff < 60*60*24*3){
      score += 3;
    }else if(diff < 60*60*24*7){
      score += 2;
    }else if(diff < 60*60*24*14){
      score += 1;
    }
  }
  command->setRawScore(score);
} // }}}

double ib::History::calcScore(const std::string &name, double average, double se){ // {{{
  if(commands_.empty()) {
    return 0.0;
  }
  auto it = commands_.find(name);
  if(it == commands_.end()) return 0.0;
  const ib::HistoryCommand *command = (*it).second;
  const double v = command->getRawScore() - average;
  if(v < 0.001) return 0.5;

  return std::min(1.0, ((10 * (command->getRawScore() - average)) / se + 50) / 100.0);
} // }}}

double ib::History::calcScoreSe(){ // {{{
  if(commands_.empty()) {
    return 0.0;
  }

  const double avr = getAverageScore();
  double sum = 0;
  for(auto it = commands_.begin(), last = commands_.end(); it != last; ++it){
    ib::HistoryCommand *cmd = (*it).second;
    sum += pow(cmd->getRawScore() - avr, 2);
  }
  return sqrt(sum / commands_.size());
} // }}}
