#ifndef __IB_CONSTANTS_H__
#define __IB_CONSTANTS_H__

// platform flags & macros {{{
#ifdef _DEBUG
#  define DEBUG 1
#endif
#if defined _WIN32 || defined WIN32
#  define IB_OS_WIN 1
#  define WIN32 1
#  if defined _WIN64 || defined WIN64
#    define WIN64 1
#    define IB_OS_WIN64 1
#    define IB_OS_STRING "win_64"
#    define IB_64BIT 1
#  else
#    define IB_OS_WIN32 1
#    define IB_OS_STRING "win_32"
#  endif
#  define UNICODE 1
#  define _UNICODE 1
#  define _WIN32_DCOM 1
#  define sleep(n) Sleep(1000 * n)
#endif
#if defined linux || defined __linux__
#  define IB_OS_LINUX 1 
#  define IB_OS_STRING "linux"
#endif
// }}}

#include <memory>
#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <locale>
#include <ctime>
#include <cassert>
#include <vector>
#include <map>
#include <algorithm>
#include <new>
#include <unordered_map>
#include <unordered_set>
#include <deque>

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Select_Browser.H>
#include <FL/x.H>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_XPM_Image.H>
#include <FL/Fl_GIF_Image.H>
#include <FL/Fl_JPEG_Image.H>
#include <FL/Fl_Menu.H>
#include "Fl_Font.H"
#include <FL/filename.H>

#include <lua.hpp>
#include <oniguruma.h>
#include <migemo.h>

// constants {{{
#define IB_VERSION "0.9.9"
#ifdef IB_PUBLIC
#      define IB_EXPORT
#else
#      define IB_EXPORT extern
#endif
#ifdef IB_OS_WIN
#  define IB_MAX_PATH_BYTE (MAX_PATH*5)
#  define IB_MAX_PATH MAX_PATH
#else
#  define IB_MAX_PATH_BYTE (PATH_MAX)
#  define IB_MAX_PATH (PATH_MAX)
#endif
#define IB_EVENT_END_COMPOSITION 100

const char* const IB_LOCALE = "ja_JP.UTF-8";
const int         IB_MAX_ARGS = 32;
const int         IB_KEY_EVENT_THRESOLD_MIN = 10;
const int         IB_ICON_SIZE_SMALL = 16;
const int         IB_ICON_SIZE_LARGE = 32;
// }}}

// typedef {{{
namespace ib {
  typedef std::map<std::string, std::string> string_map;
  typedef std::pair<std::string, std::string> string_pair;
  #ifdef IB_OS_WIN
    typedef wchar_t oschar;
    typedef std::wstring osstring;
  #else
    typedef char oschar;
    typedef std::string osstring;
  #endif


#if ( INT_MAX == 2147483647 )
  typedef unsigned       u32;
  typedef int            i32;
#elif ( LONG_MAX == 2147483647 )
  typedef unsigned long  u32;
  typedef long           i32;
#else
  error
#endif
}
// }}}

// compatibility stuff{{{
#if __cplusplus <= 201103L
namespace std {
  template<typename T, typename... Args>
  std::unique_ptr<T> make_unique(Args&&... args) {
      return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
  }
}
#endif
// }}}

namespace ib {
  class Fonts { // {{{
    private:
      Fonts();
      ~Fonts();
      Fonts(const Fonts&);
      Fonts& operator =(const Fonts&);
    public:
      enum {
        input   = (2 << (4+0)),
        list    = (2 << (4+1)),
        console = (2 << (4+2))
      };
  }; // }}}
}

#endif
