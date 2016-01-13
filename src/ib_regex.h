#ifndef __IB_REGEX_H__
#define __IB_REGEX_H__

#include "ib_constants.h"
#include "ib_utils.h"
#include "ib_platform.h"

namespace ib{
  class Regex;
  typedef void (*regsubfunc)(const Regex &regex, std::string *result, void *userdata);

  class OnigrumaService: public Singleton<OnigrumaService> {
    friend class Singleton<OnigrumaService>;
    public:
      ~OnigrumaService(){
        //onig_end();
        //ib::platform::destroy_mutex(&mutex_);
      }
      ib::mutex* getMutex() { return &mutex_; }

    protected:
      OnigrumaService() : mutex_(){
        ib::platform::create_mutex(&mutex_);
      }

    protected:
      ib::mutex     mutex_;
  };

  class Regex : private NonCopyable<Regex> {
    public:
      const static unsigned int NONE = ONIG_OPTION_NONE;
      const static unsigned int S    = ONIG_OPTION_SINGLELINE;
      const static unsigned int M    = ONIG_OPTION_MULTILINE;
      const static unsigned int I    = ONIG_OPTION_IGNORECASE;

      static void escape(std::string &ret, const char*str);

      Regex(const char *pattern, unsigned int flags) : pattern_(0), flags_(flags), reg_(0), error_info_(), region_(0), const_string_(0), var_string_(0), var_string_alloced_(0), last_result_(-1), sub_meta_char_('\\') {
        size_t len = strlen(pattern) + 1;
        char *tmpptr = (char*)malloc(sizeof(char) * len);
        strcpy(tmpptr, pattern);
        pattern_ = (unsigned char*)tmpptr;
        //pattern_ = (unsigned char*)strdup(pattern);
      }

      int init(std::string *error_msg = 0) {
        ib::platform::ScopedLock lock(OnigrumaService::inst().getMutex());
        if(reg_ == 0){
          region_  = onig_region_new();
          onig_set_default_case_fold_flag(0);
          int r = onig_new(&reg_, pattern_, pattern_ + strlen((char* )pattern_),
            flags_, ONIG_ENCODING_UTF8, ONIG_SYNTAX_PERL_NG, &error_info_);
          if (r != ONIG_NORMAL) {
            if(error_msg != 0){
              char s[ONIG_MAX_ERROR_MESSAGE_LEN];
              onig_error_code_to_str((unsigned char*)s, r, &error_info_);
              *error_msg += s;
            }
            return r;
          }
          return 0;
        }else{
          return 0;
        }
      }

      ~Regex() {
        ib::platform::ScopedLock lock(OnigrumaService::inst().getMutex());
        freeString();
        free(pattern_);
        if(reg_ != 0){ onig_free(reg_); }
        if(region_ != 0) { onig_region_free(region_, 1); }
      }

      const char* getPattern() const { return (const char*)pattern_;}
      unsigned int getFlags() const { return flags_;}

      /* returns 0 if matched, otherwise 1 */
      int match(const char* string, const std::size_t startpos = 0, const std::size_t endpos=0);
      int match(const std::string &string, const std::size_t startpos = 0, const std::size_t endpos=0) {
        return match(string.c_str(), startpos, endpos);
      }
      /* returns 0 if matched, otherwise 1 */
      int search(const char* string, const std::size_t startpos = 0, const std::size_t endpos=0);
      int search(const std::string &string, const std::size_t startpos = 0, const std::size_t endpos=0){
        return search(string.c_str(), startpos, endpos);
      }

      void split(std::vector<std::string> &result, const char *string);
      void split(std::vector<std::string> &result, const std::string &string){
        split(result, string.c_str());
      }

      void gsub(std::string &ret, const char *string, const char *repl);
      void gsub(std::string &ret, const std::string &string, const std::string &repl){
        return gsub(ret, string.c_str(), repl.c_str());
      }

      void gsub(std::string &ret, const char *string, ib::regsubfunc replacer, void *userdata);
      void gsub(std::string &ret, const std::string &string, ib::regsubfunc replacer, void *userdata){
        return gsub(ret, string.c_str(), replacer, userdata);
      }

      /* these functions does not work correctly if current state is not 'matched' */
      int getNumGroups() const { return region_->num_regs; }
      std::string group(const int n) const {
        if(n >= region_->num_regs) { return std::string(""); }
        const char *string = getString();
        return std::string(string+region_->beg[n], region_->end[n] - region_->beg[n]);
      };
      std::string _0() const { return group(0); }
      std::string _1() const { return group(1); }
      std::string _2() const { return group(2); }
      std::string _3() const { return group(3); }
      std::string _4() const { return group(4); }
      std::string _5() const { return group(5); }
      std::string _6() const { return group(6); }
      std::string _7() const { return group(7); }
      std::string _8() const { return group(8); }
      std::string _9() const { return group(9); }
      int getStartpos(const int n) const { return region_->beg[n]; }
      int getEndpos(const int n) const { return region_->end[n]; }
      int getGroupLength(const int n) const { return region_->end[n] - region_->beg[n]; }
      int getFirstpos() const { return region_->beg[0];}
      int getLastpos() const { return region_->end[0]; }
      int getBytesMatched() const { return last_result_;}

      void copyString(const char *value){
        freeString();
        var_string_alloced_ = 1;
        size_t len = strlen(value) + 1;
        var_string_ = (char*)malloc(sizeof(char) * len);
        strcpy(var_string_, value);
        const_string_ = 0;
      }

      void setString(const char *value) {
        freeString();
        const_string_ = value;
      }

      const char* getString() const {
        return (var_string_alloced_ == 0) ? const_string_ : var_string_;
      }

      void setSubMetaChar(const char value) { sub_meta_char_ = value; }

    protected:
      unsigned char  *pattern_;
      unsigned int flags_;
      regex_t* reg_;
      OnigErrorInfo error_info_;
      OnigRegion *region_;
      const char *const_string_;
      char       *var_string_;
      int        var_string_alloced_;
      int last_result_;
      char sub_meta_char_;

      void freeString(){
        if(var_string_alloced_ == 1){
          free(var_string_);
        }
        var_string_alloced_ = 0;
        var_string_ = 0;
      }
  };

}


#endif
