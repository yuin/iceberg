#ifndef __IB_COMP_VALUE_H__
#define __IB_COMP_VALUE_H__

#include "ib_constants.h"
#include "ib_utils.h"

namespace ib{
  class CompletionValue : private NonCopyable<CompletionValue> { // {{{
    public:
      virtual ~CompletionValue() {};
      virtual const std::string& getCompvalue() const = 0;
      virtual const std::string& getDispvalue() const = 0;
      virtual const std::string& getDescription() const = 0;
      virtual bool isTemporary() const { return true; }
      virtual bool isAutocompleteEnable() const { return false; }
      virtual bool hasDescription() const { return false;}
      virtual const std::string* getContextMenuPath() const = 0;
      virtual Fl_RGB_Image* loadIcon(const int size){ return 0; }
  }; // }}}

  class CompletionPathParts : public CompletionValue { // {{{
    public:
      CompletionPathParts(const char *dirname, const char *basename);
      ~CompletionPathParts(){}

      /* virtual methods */
      const std::string& getCompvalue() const { return basename_; }
      const std::string& getDispvalue() const { return basename_; }
      const std::string& getDescription() const { return description_; }
      const std::string* getContextMenuPath() const;
      Fl_RGB_Image* loadIcon(const int size);

    protected:
      std::string dirname_;
      std::string basename_;
      std::string description_;
      std::string path_;
  }; // }}}

  class CompletionString : public CompletionValue { // {{{
    public:
      CompletionString(const char *value, const char *description="") : value_(value), description_(description), icon_file_("") {}
      ~CompletionString() {};

      /* virtual methods */
      const std::string& getCompvalue() const { return value_; }
      const std::string& getDispvalue() const { return value_; }
      const std::string& getDescription() const { return description_; }
      void setDescription(const std::string &value){ description_ = value; }
      void setDescription(const char *value){ description_ = value; }
      bool hasDescription() const { return description_.size() != 0;}
      const std::string* getContextMenuPath() const;
      const std::string& getIconFile() const { return icon_file_; }
      void setIconFile(const std::string &value){ icon_file_ = value; }
      void setIconFile(const char *value){ icon_file_ = value; }
      Fl_RGB_Image* loadIcon(const int size);

    protected:
      std::string value_;
      std::string description_;
      std::string icon_file_;
  }; // }}}

  class BaseCommand : public CompletionValue { // {{{
    public:
      BaseCommand() : category_(""), name_(""), path_(""), workdir_(""), description_(""), icon_file_(), is_enabled_history_(true), score_(0.0), is_dynamic_workdir_(false) {}
      ~BaseCommand() {}
      virtual int execute(const std::vector<std::string*> &args, const std::string* workdir, ib::Error &error) = 0;
      virtual void init() = 0;

      /* virtual methods */
      const std::string& getCompvalue() const { return name_; }
      const std::string& getDispvalue() const { return name_; }
      bool isTemporary() const { return false; }
      bool isAutocompleteEnable() const { return true; }
      bool hasDescription() const { return true;}

      const std::string& getWorkdir();
      const std::string& getRawWorkdir() const { return workdir_;}
      void setWorkdir(const std::string &value){ workdir_ = value; }
      void setWorkdir(const char *value){ workdir_ = value; }
      const std::string& getCategory() const { return category_; }
      void setCategory(const std::string &value){ category_ = value; }
      void setCategory(const char *value){ category_ = value; }
      const std::string& getName() const { return name_; }
      void setName(const std::string &value){ name_ = value; }
      void setName(const char *value){ name_ = value; }
      const std::string& getPath() const { return path_; }
      void setPath(const std::string &value){ path_ = value; }
      void setPath(const char *value){ path_ = value; }
      const std::string& getDescription() const { return description_; }
      void setDescription(const std::string &value){ description_ = value; }
      void setDescription(const char *value){ description_ = value; }
      const std::string& getIconFile() const { return icon_file_; }
      void setIconFile(const std::string &value){ icon_file_ = value; }
      void setIconFile(const char *value){ icon_file_ = value; }
      bool isEnabledHistory() const { return is_enabled_history_;}
      void setEnablesHistory(const bool value){ is_enabled_history_ = value; }
      double getScore() const { return score_; }
      void setScore(const double value){ score_ = value; }
      bool isDynamicWorkdir() const { return is_dynamic_workdir_; }
      void setIsDynamicWorkdir(const bool value){ is_dynamic_workdir_ = value; }

    protected:
      std::string category_;
      std::string name_;
      std::string path_;
      std::string workdir_;
      std::string description_;
      std::string icon_file_;
      bool        is_enabled_history_;
      double      score_;
      bool        is_dynamic_workdir_;


  }; // }}}

  class Command : public BaseCommand { // {{{

    public:
      Command() : BaseCommand(), command_path_(), initialized_(false) {};
      ~Command() {}

      /* virtual methods */
      const std::string* getContextMenuPath() const;
      Fl_RGB_Image* loadIcon(const int size);
      int execute(const std::vector<std::string*> &args, const std::string* workdir, ib::Error &error);
      void init();

      void setInitialized(const bool value) { initialized_ = true; }
      const std::string& getCommandPath() const { return command_path_; }
      void setCommandPath(const std::string &value){ command_path_ = value; }
      void setCommandPath(const char *value){ command_path_ = value; }


    protected:
      std::string command_path_;
      bool        initialized_;
  }; // }}}

  class LuaFunctionCommand : public BaseCommand { // {{{
    public:
      LuaFunctionCommand() : BaseCommand() {};
      ~LuaFunctionCommand() {}

      /* virtual methods */
      const std::string* getContextMenuPath() const;
      Fl_RGB_Image* loadIcon(const int size);

      void init();
      int execute(const std::vector<std::string*> &args, const std::string* workdir, ib::Error &error);
  }; // }}}

  class HistoryCommand : public BaseCommand { // {{{
    public:
      HistoryCommand() : BaseCommand(), org_cmd_(0), command_path_(), times_(), raw_score_(0), initialized_(false) {}

      /* virtual methods */
      const std::string& getCompvalue() const { return path_; }
      const std::string& getDispvalue() const { return path_; }
      void init();
      int execute(const std::vector<std::string*> &args, const std::string* workdir, ib::Error &error);
      const std::string* getContextMenuPath() const;
      Fl_RGB_Image* loadIcon(const int size);

      int getRawScore() const { return raw_score_; }
      void setRawScore(const int value){ raw_score_ = value; }

      BaseCommand* getOriginalCommand() const { return org_cmd_;}

      const std::vector<std::time_t>& getTimes() const { return times_; }
      void setTimes(const std::vector<std::time_t> &value){ times_ = value; }
      void addTime(const std::time_t value) { times_.push_back(value); }

    protected:
      BaseCommand *org_cmd_;
      std::string command_path_;
      std::vector<std::time_t> times_;
      int raw_score_;
      bool        initialized_;
  }; // }}}

}
#endif
