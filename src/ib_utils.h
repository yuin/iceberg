#ifndef __IB_UTILS_H__
#define __IB_UTILS_H__

#include "ib_constants.h"

#ifdef DEBUG
void *__my_malloc(size_t size);
void __my_free(void *ptr);
void *operator new(size_t size);
void operator delete(void *p);
#define malloc(size) __my_malloc(size)
#define free(ptr) __my_free(ptr)
#endif

namespace ib {
  class Error;
  namespace utils {
    /* application & debug stuff */
    long malloc_count();
    template<class T> void delete_pointer_vectors(std::vector<T*> &vector) {
      for (typename std::vector<T*>::const_iterator it = vector.begin();
           it != vector.end(); ++it ) {
        if(*it != 0){ delete *it; }
      }
      vector.clear();
      std::vector<T*>().swap (vector);
    }
    void exit_application(const int code = 0);
    void reboot_application();
    void scan_search_path(const char *category);
    void alert_lua_stack(lua_State *L = 0);

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

    /* keyboard stuff */
    void expand_vars(std::string &ret, const std::string &tpl, const string_map &values);
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

    /* command stuff */
    void to_command_name(std::string &ret, const std::string &string);
    int  open_directory(const std::string &path, ib::Error &error);

    /* socket stuff */
    size_t read_socket(FL_SOCKET s, char *buf, const size_t bufsize);
    int ipc_message(const char *message);
    int ipc_message(const std::string &message);

    /* byte operation stuff */
    void u32int2bebytes(char *result, ib::u32 value);
    ib::u32 bebytes2u32int(const char *bytes);
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
  }; // }}}

  template <typename T>
  class TypeMaker { // {{{
    public:
      typedef T Type;
  }; // }}}

  // needs g++ && -fthreadsafe-statics
  template<class T>
  class Singleton : private NonCopyable<T>{ // {{{
    public:
      static T& inst(void) {
        static T instance;
        return instance;
      }
  }; // }}}

  // needs g++ && -fthreadsafe-statics
  template<class T>
  class Singletonp : private NonCopyable<T>{ // {{{
    public:
      static T* inst(void) {
        static T instance;
        return &instance;
      }
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