#ifndef __IB_COMPLETER_H__
#define __IB_COMPLETER_H__

#include "ib_constants.h"
#include "ib_utils.h"
#include "ib_comp_value.h"
#include "ib_regex.h"
#include "ib_singleton.h"

namespace ib {
  class CompletionMethod : private NonCopyable<CompletionMethod> { // {{{
    public:
      static const int BEGINS_WITH = 1;
      static const int PARTIAL     = 2;
      static const int ABBR        = 3;

      CompletionMethod(){}
      virtual ~CompletionMethod(){}

      virtual void   beforeMatch(std::vector<ib::CompletionValue*> &candidates, const std::string &input){};
      virtual double match(const std::string &name, const std::string &input){return 0.0;};
      virtual void   afterMatch(std::vector<ib::CompletionValue*> &candidates, const std::string &input){};
  }; // }}}

  class CompletionMethodMigemoMixin : public CompletionMethod { // {{{
    public:
      CompletionMethodMigemoMixin() : CompletionMethod(), regex_(nullptr) {}
      virtual ~CompletionMethodMigemoMixin() { if(regex_ != nullptr) delete regex_; }
      void  beforeMatch(std::vector<ib::CompletionValue*> &candidates, const std::string &input);
      double match(const std::string &name, const std::string &input);
      void  afterMatch(std::vector<ib::CompletionValue*> &candidates, const std::string &input);

    protected:
      ib::Regex *regex_;
  }; // }}}

  class BeginsWithMatchCompletionMethod : public CompletionMethodMigemoMixin { // {{{
    public:
      BeginsWithMatchCompletionMethod() : CompletionMethodMigemoMixin() {}
      ~BeginsWithMatchCompletionMethod() {}

      double match(const std::string &name, const std::string &input);
  }; // }}}

  class PartialMatchCompletionMethod : public CompletionMethodMigemoMixin { // {{{
    public:
      PartialMatchCompletionMethod() : CompletionMethodMigemoMixin() {}
      ~PartialMatchCompletionMethod() {}

      double match(const std::string &name, const std::string &input);
  }; // }}}

  class AbbrMatchCompletionMethod : public CompletionMethod { // {{{
    public:
      AbbrMatchCompletionMethod() : CompletionMethod() {}
      ~AbbrMatchCompletionMethod() {}

      void  beforeMatch(std::vector<ib::CompletionValue*> &candidates, const std::string &input);
      double match(const std::string &name, const std::string &input);
      void  afterMatch(std::vector<ib::CompletionValue*> &candidates, const std::string &input);
  }; // }}}

  class Completer : public NonCopyable<Completer> {
    friend class ib::Singleton<ib::Completer>;
    public:
      ~Completer() {
        if(method_history_ != nullptr) delete method_history_;
        if(method_option_ != nullptr) delete method_option_;
        if(method_path_ != nullptr) delete method_path_;
        if(method_command_ != nullptr) delete method_command_;
      }

      void setOptionFuncFlag(const std::string &name) { option_func_flags_.insert(name); }
      void setOptionFuncFlag(const char *name) { option_func_flags_.insert(std::string(name)); }
      bool hasCompletionFunc(const std::string &name) const {
        return option_func_flags_.find(name) != option_func_flags_.end(); 
      }

      virtual void completeHistory(std::vector<ib::CompletionValue*> &candidates, const std::string &command);

      virtual void completeOption(std::vector<ib::CompletionValue*> &candidates, const std::string &command);

      virtual void completePath(std::vector<ib::CompletionValue*> &candidates, const std::string &value);

      void completeCommand(std::vector<ib::CompletionValue*> &candidates, const std::string &value);

      ib::CompletionMethod* getMethodHistory()  { return method_history_; }
      void setMethodHistory( ib::CompletionMethod * value){ method_history_ = value; }

      ib::CompletionMethod* getMethodOption()  { return method_option_; }
      void setMethodOption( ib::CompletionMethod * value){ method_option_ = value; }

      ib::CompletionMethod* getMethodPath()  { return method_path_; }
      void setMethodPath( ib::CompletionMethod * value){ method_path_ = value; }

      ib::CompletionMethod* getMethodCommand()  { return method_command_; }
      void setMethodCommand( ib::CompletionMethod * value){ method_command_ = value; }

    protected:
      std::unordered_set<std::string> option_func_flags_;
      CompletionMethod *method_history_;
      CompletionMethod *method_option_;
      CompletionMethod *method_path_;
      CompletionMethod *method_command_;

      Completer(): option_func_flags_(), method_history_(nullptr), method_option_(nullptr),
                   method_path_(nullptr), method_command_(nullptr) {}


  };

}

#endif
