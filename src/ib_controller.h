#ifndef __IB_CONTROLLER_H__
#define __IB_CONTROLLER_H__

#include "ib_constants.h"
#include "ib_utils.h"
#include "ib_comp_value.h"
#include "ib_singleton.h"

namespace ib {

  class Controller : private NonCopyable<Controller> { // {{{
    friend class ib::Singleton<ib::Controller>;
    public:
      ~Controller() {
        for(auto &pair : commands_) { delete pair.second; };
      }
      void initFonts();
      void initBoxtypes();
      void loadConfig(const int argc, char* const *argv);
      void cacheCommandsInSearchPath(const char* category);
      void loadCachedCommands();
      void addCommand(const std::string &name, ib::BaseCommand *command);
      void executeCommand();
      void afterExecuteCommand(const bool success, const char *message);
      void hideApplication();
      void showApplication();
      void completionInput();
      void showCompletionCandidates();
      void selectNextCompletion();
      void selectPrevCompletion();
      void handleIpcMessage(const char* message);

      const std::string& getCwd() const { return cwd_; }
      int setCwd(const char *value, ib::Error &error);
      int setCwd(const std::string& value, ib::Error &error) {  return setCwd(value.c_str(), error); }

      bool isHistorySearchMode() const { return history_search_; }
      void setHistorySearchMode(const bool value, bool display = true);
      void toggleHistorySearchMode(bool display = true) { setHistorySearchMode(!history_search_, display);}

      const std::string& getResultText() const { return result_text_; }
      void setResultText(const std::string &value){ setResultText(result_text_.c_str());}
      void setResultText(const char *value);

      void killWord();

      const std::unordered_map<std::string, ib::BaseCommand*>& getCommands() const { return commands_; }
      const std::deque<std::string>& getClipboardHistories() const { return clipboard_histories_; }
      void  appendClipboardHistory(const char *text);

    protected:
      Controller() : commands_(), clipboard_histories_(), cwd_("."), history_search_(false), result_text_(){}

      std::unordered_map<std::string, ib::BaseCommand*> commands_;
      std::deque<std::string> clipboard_histories_;
      std::string cwd_;
      bool history_search_;
      std::string result_text_;

  }; // }}}

}

#endif 
