#ifndef __IB_UTILS_H__
#define __IB_UTILS_H__

#include "ib_constants.h"

#ifdef DEBUG
#ifdef IB_OS_WIN
void *__my_malloc(size_t size);
void __my_free(void *ptr);
void *operator new(size_t size);
void operator delete(void *p);
#define malloc(size) __my_malloc(size)
#define free(ptr) __my_free(ptr)
#endif
#endif

namespace ib {
  class Error;
  namespace utils {
    /* application & debug stuff */
    long malloc_count();
    template<typename T> void delete_pointer_vectors(std::vector<T*> &vector) {
      for (typename std::vector<T*>::iterator it = vector.begin();
           it != vector.end(); ++it ) {
        if(*it != nullptr){ delete *it; }
      }
      vector.clear();
      std::vector<T*>().swap (vector);
    }
    void exit_application(const int code = 0);
    void reboot_application();
    void scan_search_path(const char *category);
    void alert_lua_stack(lua_State *L = nullptr);

    /* string stuff */
    const unsigned char utf8_skip_data[256] = {
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
     3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,5,5,5,5,6,6,1,1
    };
    inline unsigned int utf8len(const char c){return utf8_skip_data[(unsigned char)c]; }
    inline unsigned int utf8len(const char *str){return utf8_skip_data[(unsigned char)str[0]];}
     inline void ltrim_string(std::string &s) {
         s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
             return !std::isspace(ch);
         }));
     }
     inline void rtrim_string(std::string &s) {
         s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
             return !std::isspace(ch);
         }).base(), s.end());
     }

    /* keyboard stuff */
    std::string expand_vars(const std::string &tpl, const string_map &values);
    bool event_key_is_control_key();
    void parse_key_bind(int *result, const char *string);
    inline bool matches_key(const int *key_bind, const int key, const int modifiers) {
      int len=0;
      int mod = modifiers;
      for(;key_bind[len] !=0; ++len){}
      if(len == 1 && mod == 0) return key_bind[0] == key;
      len--;
      for(int i = 0; i < len; ++i){
        if(!(mod & key_bind[i])){
          return false;
        }else{
          mod = mod & ~key_bind[i];
        }
      }
      return key_bind[len] == key && mod == 0;
    }

    /* clipboard stuff */
    void get_clipboard(std::string &ret);
    void set_clipboard(const std::string &text);
    void set_clipboard(const char *text);

    /* hiDPI stuff */
    int scaled_size(int);

    /* command stuff */
    std::string to_command_name(const std::string &string);
    int  open_directory(const std::string &path, ib::Error &error);

    /* socket stuff */
    size_t read_socket(FL_SOCKET s, char *buf, const size_t bufsize);
    int ipc_message(const char *message);
    int ipc_message(const std::string &message);

    /* byte operation stuff */
    void u32int2bebytes(char *result, ib::u32 value);
    ib::u32 bebytes2u32int(const char *bytes);

    /* messagebox stuff */
    void message_box(const char *fmt, ...);
  }

  // Utility classes {{{
  template <class T>
  class NonCopyable { // {{{
    protected:
      NonCopyable () {}
      virtual ~NonCopyable () {} 
    private: 
      NonCopyable (const NonCopyable &);
      NonCopyable& operator=(const NonCopyable &);
      NonCopyable (NonCopyable&&);
      NonCopyable& operator=(NonCopyable&&);
  }; // }}}

  template <typename T>
  class TypeMaker { // {{{
    public:
      typedef T Type;
  }; // }}}
  // }}}

  class Error : private NonCopyable<Error> { // {{{
    protected:
      unsigned int code_;
      std::string  msg_;

    public:
      Error() : code_(0) {}
      Error(unsigned int code, const char *msg) : code_(code), msg_(std::string(msg)) {};
      Error(unsigned int code, const std::string &msg) : code_(code), msg_(msg) {}
    
      const unsigned int& getCode() const {return code_;}
      void setCode(const int value) { code_ = value;}
      const std::string& getMessage() const {return msg_;}
      void setMessage(const char *value) { msg_ = value; }
      void setMessage(const std::string &value) { msg_ = value; }
  }; // }}}

  class FlScopedLock : private NonCopyable<FlScopedLock> { // {{{
    public:
      FlScopedLock () { Fl::lock(); }
      ~FlScopedLock();
  }; // }}}

}

#endif
