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
#  ifdef __MINGW32__
#    define FindExInfoBasic ((FINDEX_INFO_LEVELS)1)
#    define MINGW_HAS_SECURE_API 1
#  else
#    pragma comment(lib, "lua51.lib")
#    pragma warning( disable : 4351)
#    pragma warning( disable : 4996)
#  endif
#  define snprintf    _snprintf
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
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Browser.H>
#include <FL/Fl_Select_Browser.H>
#include <FL/x.h>
#include <FL/fl_ask.H>
#include <FL/fl_draw.H>
#include <FL/Fl_PNG_Image.H>
#include <FL/Fl_GIF_Image.H>
#include <FL/Fl_JPEG_Image.H>
#include <FL/Fl_Menu.H>
#include "Fl_Font.H"

#include <lua.hpp>
#include <oniguruma.h>
#include <migemo.h>

// constants {{{
#define IB_VERSION "0.9.0"
#ifdef IB_PUBLIC
#      define IB_EXPORT
#else
#      define IB_EXPORT extern
#endif
#ifdef IB_OS_WIN
#  define IB_MAX_PATH_BYTE (MAX_PATH*5+1)
#  define IB_MAX_PATH MAX_PATH+3
#else
#  define IB_MAX_PATH_BYTE (PATH_MAX*5+1)
#  define IB_MAX_PATH PATH_MAX+3
#endif
#define IB_EVENT_END_COMPOSITION 100
#define IB_LOCALE "ja_JP.UTF-8"
#define IB_MAX_ARGS 32
// }}}

// typedef {{{
namespace ib {
  typedef std::map<std::string, std::string> string_map;
  typedef std::pair<std::string, std::string> string_pair;

  typedef std::unique_ptr<std::string> unique_string_ptr;
  typedef std::unique_ptr<wchar_t[]> unique_wchar_ptr;
  typedef std::unique_ptr<char[]> unique_char_ptr;
#ifdef IB_OS_WIN
  typedef wchar_t oschar;
  typedef std::wstring osstring;
#else
  typedef char oschar;
  typedef std::string osstring;
#endif
  typedef std::unique_ptr<oschar[]> unique_oschar_ptr;

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
