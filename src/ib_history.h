#ifndef __IB_HISTORY_H__
#define __IB_HISTORY_H__

#include "ib_constants.h"
#include "ib_utils.h"
#include "ib_comp_value.h"
#include "ib_regex.h"

namespace ib {
  class History : public Singleton<History>{ // {{{
    friend class Singleton<History>;
    public:
      ~History() {
        for(auto &c : ordered_commands_) { delete c;}
      }
      void load();
      void dump();

      const std::vector<ib::HistoryCommand*>& getOrderedCommands() const { return ordered_commands_; }
      const std::unordered_map<std::string, ib::HistoryCommand*>& getCommands() const { return commands_; }
      void addCommand(ib::HistoryCommand *command);
      void addBaseCommandHistory(const std::string &value, const ib::BaseCommand* cmd);
      void addRawInputHistory(const std::string &value);
      void calcRawScore(ib::HistoryCommand *command);
      double calcScore(const std::string &name, double average, double se);
      double calcScoreSe();

      long getTotalScore() const { return total_score_; }
      void setTotalScore(const long value){ total_score_ = value; }

      double getAverageScore() const { return total_score_ / (double)commands_.size(); }

    protected:
      History() : commands_(), ordered_commands_(), total_score_(0) {}

      std::unordered_map<std::string, ib::HistoryCommand*> commands_;
      std::vector<ib::HistoryCommand*> ordered_commands_;
      long total_score_;
  }; // }}}

}


#endif
