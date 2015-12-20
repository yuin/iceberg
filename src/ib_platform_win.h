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
  typedef int          socklen;

  typedef HANDLE       thread;
  typedef CRITICAL_SECTION mutex;
  typedef HANDLE cmutex;
  typedef struct {
    int waiters;
    CRITICAL_SECTION waiters_lock;
    HANDLE sema;
    HANDLE waiters_done;
    size_t was_broadcast;
  } condition;
  typedef void threadret;
  typedef void (*threadfunc)(void*);

  typedef HWND   whandle;
  typedef HMODULE module;
  namespace platform {
    const char PATHSEP = '/';

    void win_draw_text(ib::oschar *str, int x, int y, int w = 0, int h = 0);
    size_t win_calc_text_width(ib::oschar *str);
    int list_drives(std::vector<ib::unique_oschar_ptr> &result, ib::Error &error);
  }
}
const char *strcasestr(const char *haystack, const char *needle);
extern "C" __declspec(dllimport) BOOL __stdcall RegenerateUserEnvironment( LPVOID *lpEnvironment, BOOL bUpdate );

#endif
