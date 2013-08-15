#ifndef __IB_PLATFORM_WIN_H__
#define __IB_PLATFORM_WIN_H__
#ifndef _WINSOCKAPI_
#  include <winsock2.h>
#endif
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <process.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <shlobj.h>
#include <lm.h>

namespace ib{
  typedef unsigned long thread;
  typedef void (*threadfunc) (void *);
  typedef CRITICAL_SECTION mutex;
  typedef HANDLE condition;
  typedef HWND   whandle;
  typedef HMODULE module;
  namespace platform {
    const char PATHSEP = '/';

    void win_draw_text(ib::oschar *str, int x, int y, int w = 0, int h = 0);
    size_t win_calc_text_width(ib::oschar *str);
  }
}
const char *strcasestr(const char *haystack, const char *needle);


#endif
