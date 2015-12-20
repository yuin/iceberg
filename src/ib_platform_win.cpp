#include "ib_platform.h"
#include "ib_resource.h"
#include "ib_utils.h"
#include "ib_ui.h"
#include "ib_controller.h"
#include "ib_config.h"
#include "ib_regex.h"
#include "ib_comp_value.h"

#if defined IB_OS_WIN64
#  define IB_GWL_WNDPROC GWLP_WNDPROC
#  define SetWindowLongPtrW64( w, i, l ) \
                SetWindowLongPtrW( w, i, (LONG_PTR)(l) )
#  define GetWindowLongPtrW64( w, i) \
                GetWindowLongPtrW( w, i)
#else
#  define IB_GWL_WNDPROC GWL_WNDPROC
#  define SetWindowLongPtrW64( w, i, l ) \
                SetWindowLongPtrW( w, i, (LONG)(l) )
#  define GetWindowLongPtrW64( w, i) \
                GetWindowLongPtrW( w, i)
#endif

#define IB_WIN_TRAYICON_ID  1
#define IB_WM_TRAYICON  (WM_USER + 1)
#define IB_WM_INIT      (WM_USER + 2)
#define IB_IDM_EXIT 401
#define IB_IDH_HOTKEY 100001

static HINSTANCE ib_g_hinst;
static HWND ib_g_hwnd_main;
static HWND ib_g_hwnd_list;
static NOTIFYICONDATA ib_g_trayicon;
static WNDPROC ib_g_wndproc;
static HWND ib_g_hwnd_clbchain_next;

const char *strcasestr(const char *haystack, const char *needle) { // {{{
  int haypos;
  int needlepos;

  haypos = 0;
  while (haystack[haypos]) {
    if (tolower (haystack[haypos]) == tolower(needle[0])) {
      needlepos = 1;
      while ( (needle[needlepos]) &&
              (tolower(haystack[haypos + needlepos])
               == tolower(needle[needlepos])) )
          ++needlepos;
      if (! needle[needlepos]) return (haystack + haypos);
    }
    ++haypos;
  }
  return NULL;
} // }}}

void tcsncpy_s(ib::oschar *dest, const ib::oschar *src, std::size_t bufsize) {
  _tcsncpy_s(dest, bufsize, src, _TRUNCATE);
}

ib::oschar* tcstokread(ib::oschar *buf, const ib::oschar sep) { // {{{
  while(1){
    if(*buf == sep){
      *buf = '\0';
      return buf+1;
    }else if(*buf == L'\0'){
      return buf;
    }else{
      buf++;
    }
  }
} // }}}

static char* ib_platform_get_last_error_message(){ // {{{
  void* msg_buf;
  FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
    FORMAT_MESSAGE_FROM_SYSTEM | 
    FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL, GetLastError(),
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), 
    (LPTSTR) &msg_buf, 0, NULL);
  char *ret = ib::platform::oschar2utf8((const ib::oschar*)msg_buf);
  ret[strlen(ret)-2] = '\0';
  LocalFree(msg_buf);
  return ret;
} // }}}

static void ib_platform_set_error(ib::Error &error){ // {{{
  error.setCode(GetLastError());
  ib::unique_char_ptr msg(ib_platform_get_last_error_message());
  error.setMessage(msg.get());
} // }}}

// ib_platform_key2os_key {{{
// code from fltk1.3: Fl_get_key_win32.cxx
static const struct {unsigned short vk, key;} vktab[] = {
  {VK_SPACE,    ' '},
  {'1',        '!'},
  {0xde,    '\"'},
  {'3',        '#'},
  {'4',        '$'},
  {'5',        '%'},
  {'7',        '&'},
  {0xde,    '\''},
  {'9',        '('},
  {'0',        ')'},
  {'8',        '*'},
  {0xbb,    '+'},
  {0xbc,    ','},
  {0xbd,    '-'},
  {0xbe,    '.'},
  {0xbf,    '/'},
  {0xba,    ':'},
  {0xba,    ';'},
  {0xbc,    '<'},
  {0xbb,    '='},
  {0xbe,    '>'},
  {0xbf,    '?'},
  {'2',        '@'},
  {0xdb,    '['},
  {0xdc,    '\\'},
  {0xdd,    ']'},
  {'6',        '^'},
  {0xbd,    '_'},
  {0xc0,    '`'},
  {0xdb,    '{'},
  {0xdc,    '|'},
  {0xdd,    '}'},
  {0xc0,    '~'},
  {VK_BACK,    FL_BackSpace},
  {VK_TAB,    FL_Tab},
  {VK_CLEAR,    0xff0b/*XK_Clear*/},
  {0xe2 /*VK_OEM_102*/,    FL_Iso_Key},
  {VK_RETURN,    FL_Enter},
  {VK_PAUSE,    FL_Pause},
  {VK_SCROLL,    FL_Scroll_Lock},
  {VK_ESCAPE,    FL_Escape},
  {VK_HOME,    FL_Home},
  {VK_LEFT,    FL_Left},
  {VK_UP,    FL_Up},
  {VK_RIGHT,    FL_Right},
  {VK_DOWN,    FL_Down},
  {VK_PRIOR,    FL_Page_Up},
  {VK_NEXT,    FL_Page_Down},
  {VK_END,    FL_End},
  {VK_SNAPSHOT,    FL_Print},
  {VK_INSERT,    FL_Insert},
  {VK_APPS,    FL_Menu},
  {VK_NUMLOCK,    FL_Num_Lock},
//{VK_???,    FL_KP_Enter},
  {VK_MULTIPLY,    FL_KP+'*'},
  {VK_ADD,    FL_KP+'+'},
  {VK_SUBTRACT,    FL_KP+'-'},
  {VK_DECIMAL,    FL_KP+'.'},
  {VK_DIVIDE,    FL_KP+'/'},
  {VK_LSHIFT,    FL_Shift_L},
  {VK_RSHIFT,    FL_Shift_R},
  {VK_LCONTROL,    FL_Control_L},
  {VK_RCONTROL,    FL_Control_R},
  {0xf0,       FL_Caps_Lock},
  {VK_LWIN,    FL_Meta_L},
  {VK_RWIN,    FL_Meta_R},
  {VK_LMENU,    FL_Alt_L},
  {VK_RMENU,    FL_Alt_R},
  {VK_DELETE,    FL_Delete}
};

static int ib_platform_key2os_key(const int key){
  int b = sizeof(vktab)/sizeof(*vktab);
  for(int i=0; i < b; i++) {
    if (vktab[i].key == key) return vktab[i].vk;
  }
  return key;
}
// }}}

static int ib_platform_modkey2os_modkey(const int key){ // {{{
  switch(key){
    case FL_SHIFT:
      return MOD_SHIFT;
    case FL_ALT:
      return MOD_ALT;
    case FL_CTRL:
      return MOD_CONTROL;
    case FL_META:
      return MOD_WIN;
  }
  return 0;
} // }}}

static int ib_platform_register_hotkey() { /* {{{ */
  int mod, len;
  const int* hot_key = ib::Config::inst().getHotKey();
  len = 0;
  for(;hot_key[len] != 0; ++len){}
  len--;
  mod = 0;
  for(int i = 0; i < len; ++i){
    mod |= ib_platform_modkey2os_modkey(hot_key[i]);
  }
  if(RegisterHotKey(ib_g_hwnd_main, IB_IDH_HOTKEY, mod, ib_platform_key2os_key(hot_key[len])) == 0){
    return -1;
  }
  return 0;
} /* }}} */

static LRESULT CALLBACK ib_platform_wnd_proc(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam) { /* {{{ */
  static int m_end_composition = 0;

  switch(umsg) {
    case WM_COMMAND:
      switch(LOWORD(wparam)) {
        case IB_IDM_EXIT: 
          ib::utils::exit_application();
      }
      break;

    case WM_IME_STARTCOMPOSITION: {
        ib::MainWindow::inst()->getInput()->setImeComposition(1);
      }
    break;

    case WM_IME_COMPOSITION: {
        ib::MainWindow::inst()->getInput()->setImeComposition(wparam != 0 ? 1: 0);
      }
    break;

    case WM_IME_ENDCOMPOSITION: {
        ib::MainWindow::inst()->getInput()->setImeComposition(0);
        m_end_composition = 1;
      }
      break;

    case WM_HOTKEY: {
        if((int)wparam == IB_IDH_HOTKEY){
          ib::Controller::inst().showApplication();
        }
      }
      return 0;

    case IB_WM_INIT: {
        if(ib_platform_register_hotkey() < 0){
            fl_alert("%s", "Failed to register hotkeys.");
            ib::utils::exit_application(1);
        }
        ib_g_trayicon.cbSize           = sizeof(NOTIFYICONDATA);
        ib_g_trayicon.hWnd             = ib_g_hwnd_main;
        ib_g_trayicon.uID              = IB_WIN_TRAYICON_ID;
        ib_g_trayicon.uFlags           = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        ib_g_trayicon.uCallbackMessage = IB_WM_TRAYICON;
        ib_g_trayicon.hIcon            = LoadIcon(ib_g_hinst, MAKEINTRESOURCE(IB_IDI_APPICON));
        lstrcpy(ib_g_trayicon.szTip, L"iceberg");
        while(!Shell_NotifyIcon(NIM_ADD,&ib_g_trayicon)) {
            Sleep(2000);
            Shell_NotifyIcon(NIM_ADD,&ib_g_trayicon);
            if(Shell_NotifyIcon(NIM_MODIFY,&ib_g_trayicon)) break;
        }

        ib_g_hwnd_clbchain_next = SetClipboardViewer(hwnd);
      }
      return 0;

    case IB_WM_TRAYICON: {
        if(wparam == IB_WIN_TRAYICON_ID) {
          if(lparam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            
            HMENU hMenuPop = CreatePopupMenu();
            AppendMenu(hMenuPop, MF_BYCOMMAND | MF_STRING, IB_IDM_EXIT, L"Exit(&X)");

            TrackPopupMenu(hMenuPop, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
            MSG msg;
            if(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
              TranslateMessage(&msg);
              DispatchMessage(&msg);
            }
            DestroyMenu(hMenuPop);
          }else if(lparam == WM_LBUTTONUP){
            PostMessage(ib_g_hwnd_main, WM_HOTKEY, (WPARAM)IB_IDH_HOTKEY, lparam);
          }
        }
      }
      return 0;

    case WM_DRAWCLIPBOARD: {
        OpenClipboard(hwnd);
        HANDLE htext = GetClipboardData(CF_TEXT);
        if(htext != NULL) {
            char *text = (char*)GlobalLock(htext);
            ib::unique_char_ptr utf8text(ib::platform::local2utf8(text));
            ib::Regex reg("\r\n", ib::Regex::NONE);
            reg.init();
            std::string replaced;
            reg.gsub(replaced, utf8text.get(), "\n");
            ib::Controller::inst().appendClipboardHistory(replaced.c_str());
            GlobalUnlock(htext);
        }
        CloseClipboard();
        if(ib_g_hwnd_clbchain_next != NULL) SendMessage(ib_g_hwnd_clbchain_next, umsg, wparam, lparam);
      }
      break;

    case WM_KEYUP: {
      if(m_end_composition){
          m_end_composition = 0;
          ib::MainWindow::inst()->getInput()->sendEndCompositionEvent();
          return 0;
        }
      }
      break;

    case WM_SYSCOMMAND: {
        if(wparam == SC_CLOSE){
          ib::utils::exit_application(0);
          return 1;
        }
      }
      break;

    case WM_QUERYENDSESSION: {
        return 1;
      }
      break;


    case WM_ENDSESSION: {
        ib::utils::exit_application(0);
      }
      break;

    case WM_CHANGECBCHAIN: {
        if((HWND)wparam == ib_g_hwnd_clbchain_next) ib_g_hwnd_clbchain_next = (HWND)lparam;
        else if(ib_g_hwnd_clbchain_next != NULL) SendMessage(ib_g_hwnd_clbchain_next, umsg, wparam, lparam);
      }
      break;

    case WM_SETTINGCHANGE: {
        LPVOID lpenv;
        RegenerateUserEnvironment(&lpenv, TRUE);
      }
      break;

    case WM_DESTROY: {
        ChangeClipboardChain(hwnd, ib_g_hwnd_clbchain_next);
        PostQuitMessage(0);
      }
      break;
  }

  return CallWindowProc(ib_g_wndproc, hwnd, umsg, wparam, lparam);
} /* }}} */

static uchar * ib_platform_read_hbitmap(HBITMAP hbm, int X, int Y, int w, int h, int alpha) { /* {{{ */
  int d;
  
  d = alpha ? 4 : 3;
  uchar *p = new uchar[w * h * d];
  memset(p, 0, w * h * d);
  int ww = w; 
  int shift_x = 0; 
  int shift_y = 0; 
  if (X < 0) {
    shift_x = -X;
    w += X;
    X = 0;
  }
  if (Y < 0) {
    shift_y = -Y;
    h += Y;
    Y = 0;
  }
  if (h < 1 || w < 1) return p;  
  int line_size = ((d*w+3)/4) * 4;
  uchar *dib = new uchar[line_size*h]; 
  
  BITMAPINFO   bi;
  bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bi.bmiHeader.biWidth = w;
  bi.bmiHeader.biHeight = -h;  
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biCompression = BI_RGB;
  bi.bmiHeader.biBitCount = alpha ? 32 : 24;
  bi.bmiHeader.biSizeImage = 0;
  bi.bmiHeader.biXPelsPerMeter = 0;
  bi.bmiHeader.biYPelsPerMeter = 0;
  bi.bmiHeader.biClrUsed = 0;
  bi.bmiHeader.biClrImportant = 0;
  
  HDC hdc1 =  CreateCompatibleDC(0);
  SelectObject(hdc1,hbm);   
  BitBlt(hdc1,0,0,w,h,0,X,Y,SRCCOPY); 
  
  GetDIBits(hdc1, hbm, 0, h, dib, (BITMAPINFO *)&bi, DIB_RGB_COLORS);
  
  for (int j = 0; j<h; j++) {
    const uchar *src = dib + j * line_size;   
    uchar *tg = p + (j + shift_y) * d * ww + shift_x * d; 
    for (int i = 0; i<w; i++) {
      uchar b = *src++;
      uchar g = *src++;
      uchar r = *src++;
      *tg++ = r;
      *tg++ = g; 
      *tg++ = b; 
      if (alpha) *tg++ = *src++; 
    }
  }
  
  DeleteDC(hdc1);
  delete[] dib;  
  return p;
} /* }}} */

static void ib_platform_list_network_servers(std::vector<ib::oschar*> &result, const ib::oschar *domain, const DWORD types){ // {{{
  SERVER_INFO_100* server_info = 0;
  DWORD read;
  DWORD num_servers;

  NET_API_STATUS ret = NetServerEnum(NULL, 100, (BYTE**)&server_info, MAX_PREFERRED_LENGTH, &read, &num_servers, types, domain, 0);
  if (ret == NERR_Success) {
    for (DWORD i = 0; i < read; ++i) {
      ib::oschar *server_name = new ib::oschar[IB_MAX_PATH];
      _tcscpy(server_name, server_info[i].sv100_name);
      result.push_back(server_name);
    }
  }

  if (server_info) {
    NetApiBufferFree((void*)server_info);
  }
} // }}}

static void ib_platform_list_network_shares(std::vector<ib::oschar*> &result, ib::oschar *server){ // {{{
  SHARE_INFO_502* share_info = 0;
  DWORD read;
  DWORD num_shares;

  NET_API_STATUS ret = NetShareEnum(server, 502, (BYTE**)&share_info,  MAX_PREFERRED_LENGTH, &read, &num_shares, NULL);
  if (ret == NERR_Success) {
    for (DWORD i = 0; i < read; ++i) {
      ib::oschar *share_name = new ib::oschar[IB_MAX_PATH];
      _tcscpy(share_name, share_info[i].shi502_netname);
      result.push_back(share_name);
    }
  }

  if (share_info) {
    NetApiBufferFree((void*)share_info);
  }
} // }}}

int ib::platform::startup_system() { // {{{
  int ret = 0;
  if(!SUCCEEDED(CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE))){
    ret = 1;
    goto finalize;
  }

  WSADATA wsa;
  if (WSAStartup(MAKEWORD(2,0), &wsa) != 0) {
    ret = 1;
    goto finalize;
  }

finalize:
  return ret;
} // }}}

int ib::platform::init_system() { // {{{
  int ret;
  long style;
  auto &cfg = ib::Config::inst();

  ib_g_hwnd_main = fl_xid(ib::MainWindow::inst());
  ib_g_hwnd_list = fl_xid(ib::ListWindow::inst());
  ib_g_hinst    = fl_display;

  ib_g_wndproc = (WNDPROC)(GetWindowLongPtrW64(ib_g_hwnd_main, IB_GWL_WNDPROC));
  SetWindowLongPtrW64(ib_g_hwnd_main, IB_GWL_WNDPROC, ib_platform_wnd_proc);

  style = (GetWindowLongPtrW64(ib_g_hwnd_main, GWL_EXSTYLE));
  SetWindowLongPtrW64(ib_g_hwnd_main, GWL_EXSTYLE, style | WS_EX_LAYERED);
  ib::platform::set_window_alpha(ib::MainWindow::inst(), cfg.getStyleWindowAlpha());

  style = (GetWindowLongPtrW64(ib_g_hwnd_list, GWL_EXSTYLE));
  SetWindowLongPtrW64(ib_g_hwnd_list, GWL_EXSTYLE, style | WS_EX_LAYERED);
  ib::platform::set_window_alpha(ib::ListWindow::inst(), cfg.getStyleListAlpha());

  PostMessage(ib_g_hwnd_main, IB_WM_INIT, (WPARAM)0, (LPARAM)0);
  ret = 0;

  return ret;
} // }}}

void ib::platform::finalize_system(){ // {{{ 
  UnregisterHotKey(ib_g_hwnd_main, IB_IDH_HOTKEY);
  Shell_NotifyIcon(NIM_DELETE, &ib_g_trayicon);
  CoUninitialize();
  WSACleanup();
} // }}}

void ib::platform::get_runtime_platform(char *ret){ // {{{ 
  BOOL (*is_wow64_process) (HANDLE, PBOOL);
  BOOL is_wow64 = FALSE;
  is_wow64_process = (BOOL (*)(HANDLE, PBOOL))GetProcAddress(
    GetModuleHandle(L"kernel32"), "IsWow64Process");
  int type;
  if(!is_wow64_process){
    type = 0; // 32bit on 32bit
  }else{
    is_wow64_process(GetCurrentProcess(), &is_wow64);
    if(is_wow64){
      type = 1; // 32bit on 64bit
    }else{
#ifdef IB_64BIT
      type = 2; // 64bit on 64bit
#else
      type = 0; // 32bit on 32bit
#endif
    }
  }

   OSVERSIONINFO version_info;
   version_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&version_info);
   sprintf(ret, "%ld.%ld.%ld %s", version_info.dwMajorVersion, version_info.dwMinorVersion,
                version_info.dwBuildNumber,
                type == 0 ? "x32" : (type == 1 ? "wow64" : "x64"));
} // }}}

ib::oschar* ib::platform::utf82oschar(const char *src) { // {{{
  const std::size_t _srclen = strlen(src);
  const unsigned int srclen = (_srclen) > UINT_MAX ? UINT_MAX : (unsigned int)_srclen;

  unsigned int wlen = fl_utf8toUtf16(src, srclen, NULL, 0);
  wlen++;
  wchar_t *wbuf = new wchar_t[wlen];
  wlen = fl_utf8toUtf16(src, srclen, (unsigned short*)wbuf, wlen);
  wbuf[wlen] = 0;
  return wbuf;
} // }}}

void ib::platform::utf82oschar_b(ib::oschar *buf, const unsigned int bufsize, const char *src) { // {{{
  const std::size_t _srclen = strlen(src);
  const unsigned int srclen = (_srclen) > UINT_MAX ? UINT_MAX : (unsigned int)_srclen;
  unsigned int wlen = fl_utf8toUtf16(src, srclen, (unsigned short*)buf, bufsize);
  buf[wlen] = 0;
} // }}}

char* ib::platform::oschar2utf8(const ib::oschar *src) { // {{{
  unsigned int size = WideCharToMultiByte(CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL);
  size++;
  char *buff = new char[size];
  size = WideCharToMultiByte(CP_UTF8, 0, src, -1, buff, size, NULL, NULL);
  buff[size] = 0;
  return buff;
} // }}}

void ib::platform::oschar2utf8_b(char *buf, const unsigned int bufsize, const ib::oschar *src) { // {{{
  int size = WideCharToMultiByte(CP_UTF8, 0, src, -1, buf, bufsize, NULL, NULL);
  buf[size] = 0;
} // }}}

char* ib::platform::oschar2local(const ib::oschar *src) { // {{{
  int size = WideCharToMultiByte(GetACP(), 0, src, -1, NULL, 0, NULL, NULL);
  size++;
  char *buff = new char[size];
  size = WideCharToMultiByte(GetACP(), 0, src, -1, buff, size, NULL, NULL);
  buff[size] = 0;
  return buff;
} // }}}

void ib::platform::oschar2local_b(char *buf, const unsigned int bufsize, const ib::oschar *src) { // {{{
  int size = WideCharToMultiByte(GetACP(), 0, src, -1, buf, bufsize, NULL, NULL);
  buf[size] = 0;
} // }}}

char* ib::platform::utf82local(const char *src) { // {{{
  ib::unique_oschar_ptr osstring(ib::platform::utf82oschar(src));
  return ib::platform::oschar2local(osstring.get());
} // }}}

char* ib::platform::local2utf8(const char *src) { // {{{
  int size = MultiByteToWideChar(GetACP(), 0, src, -1, NULL, 0);
  size++;
  ib::oschar *buff = new ib::oschar[size];
  size = MultiByteToWideChar(GetACP(), 0, src, -1, buff, size);
  buff[size] = '\0';
  char *ret = ib::platform::oschar2utf8(buff);
  delete[] buff;
  return ret;
} // }}}

ib::oschar* ib::platform::quote_string(ib::oschar *result, const ib::oschar *str) { // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  if(result != str) { tcsncpy_s(result, str, IB_MAX_PATH); }
  PathQuoteSpaces(result);
  return result;
} // }}}

ib::oschar* ib::platform::unquote_string(ib::oschar *result, const ib::oschar *str) { // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  if(result != str) { tcsncpy_s(result, str, IB_MAX_PATH); }
  PathUnquoteSpaces(result);
  return result;
} // }}}

void ib::platform::hide_window(Fl_Window *window){ // {{{
  window->clear_visible();
  ShowWindow(fl_xid(window), SW_HIDE);
} // }}}

void ib::platform::activate_window(Fl_Window *window){ // {{{
  window->set_visible_focus();
  ShowWindow(fl_xid(window), SW_SHOW);
  SetForegroundWindow(fl_xid(window));
} // }}}

void ib::platform::raise_window(Fl_Window *window){ // {{{
  window->set_visible();
  ShowWindow(fl_xid(window), SW_SHOWNA);
} // }}}

void ib::platform::set_window_alpha(Fl_Window *window, int alpha){ // {{{
  SetLayeredWindowAttributes(fl_xid(window), 0, alpha, LWA_ALPHA);
} // }}}

static int ib_platform_shell_execute(const std::string &path, const std::string &strparams, const std::string &cwd, const std::string &terminal, ib::Error &error) { // {{{
  auto &cfg = ib::Config::inst();
  ib::Regex reg(".*\\.(cpl)", ib::Regex::NONE);
  reg.init();
#ifdef IB_OS_WIN64
  long long ret;
#else
  int ret;
#endif
  if(reg.match(path.c_str()) == 0){
    ib::unique_wchar_ptr wpath(ib::platform::utf82oschar("control.exe"));
    ib::unique_wchar_ptr wparams(ib::platform::utf82oschar(path.c_str()));
    ib::unique_wchar_ptr wcwd(ib::platform::utf82oschar(cwd.c_str()));
#ifdef IB_OS_WIN64
    ret = (long long)(ShellExecute(ib_g_hwnd_main, L"open", wpath.get(), wparams.get(), wcwd.get(), SW_SHOWNORMAL));
#else
    ret = (int)ShellExecute(ib_g_hwnd_main, L"open", wpath.get(), wparams.get(), wcwd.get(), SW_SHOWNORMAL);
#endif
  }else{
    if(terminal == "yes") {
      std::vector<std::string*> args;
      // TODO escape quotations and spaces
      args.push_back(new std::string(path + " " + strparams));
      ib::Command cmd;
      cmd.setName("tmp");
      cmd.setPath(cfg.getTerminal());
      cmd.setTerminal("no");
      cmd.init();
      int ret = cmd.execute(args, &cwd, error);
      ib::utils::delete_pointer_vectors(args);
      return ret;
    }
    ib::unique_wchar_ptr wpath(ib::platform::utf82oschar(path.c_str()));
    ib::unique_wchar_ptr wparams(ib::platform::utf82oschar(strparams.c_str()));
    ib::unique_wchar_ptr wcwd(ib::platform::utf82oschar(cwd.c_str()));
#ifdef IB_OS_WIN64
    ret = (long long)(ShellExecute(ib_g_hwnd_main, L"open", wpath.get(), wparams.get(), wcwd.get(), SW_SHOWNORMAL));
#else
    ret = (int)ShellExecute(ib_g_hwnd_main, L"open", wpath.get(), wparams.get(), wcwd.get(), SW_SHOWNORMAL);
#endif
  }


  if(ret > 32){ return 0; }

  char buf[1024];
  error.setCode((int)ret);
  switch(ret) {
    case 0:
      sprintf(buf, "%d: %s", error.getCode(), "The operating system is out of memory or resources.");
      break;
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND:
      sprintf(buf, "%d: %s", error.getCode(), "The specified path was not found.");
      break;
    case SE_ERR_ACCESSDENIED:
      sprintf(buf, "%d: %s", error.getCode(), "The operating system denied access to the specified file.");
      break;
    default:
      sprintf(buf, "%d: %s", error.getCode(), "Error.");
  }
  error.setMessage(buf);

  return -1;
} // }}}

int ib::platform::shell_execute(const std::string &path, const std::vector<ib::unique_string_ptr> &params, const std::string &cwd, const std::string &terminal, ib::Error &error) { // {{{
  std::string strparams;
  for(auto it = params.begin(), last = params.end(); it != last; ++it){
    ib::unique_oschar_ptr osparam(ib::platform::utf82oschar((*it).get()->c_str()));
    ib::unique_oschar_ptr osescaped_param(ib::platform::quote_string(0, osparam.get()));
    ib::unique_char_ptr   escaped_param(ib::platform::oschar2utf8(osescaped_param.get()));
    strparams += escaped_param.get();
    strparams += " ";
  }
  return ib_platform_shell_execute(path, strparams, cwd, terminal, error);
} /* }}} */

int ib::platform::shell_execute(const std::string &path, const std::vector<std::string*> &params, const std::string &cwd, const std::string &terminal, ib::Error &error) { // {{{
  std::string strparams;
  for(auto it = params.begin(), last = params.end(); it != last; ++it){
    ib::unique_oschar_ptr osparam(ib::platform::utf82oschar((*it)->c_str()));
    ib::unique_oschar_ptr osescaped_param(ib::platform::quote_string(0, osparam.get()));
    ib::unique_char_ptr   escaped_param(ib::platform::oschar2utf8(osescaped_param.get()));
    strparams += escaped_param.get();
    strparams += " ";
  }
  return ib_platform_shell_execute(path, strparams, cwd, terminal, error);
} /* }}} */

int ib::platform::command_output(std::string &sstdout, std::string &sstderr, const char *cmd, ib::Error &error) { // {{{
  ib::unique_oschar_ptr command(ib::platform::utf82oschar(cmd));
  int funcret = 0;
  
  HANDLE read_pipe = 0, write_pipe = 0;
  HANDLE err_read_pipe = 0, err_write_pipe = 0;
  SECURITY_ATTRIBUTES sa;
  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = 0;
  sa.bInheritHandle = TRUE;
  CreatePipe(&read_pipe, &write_pipe, &sa, 8192);
  CreatePipe(&err_read_pipe, &err_write_pipe, &sa, 8192);
  STARTUPINFO startup_info;
  PROCESS_INFORMATION process_info;
  ZeroMemory(&startup_info, sizeof(STARTUPINFO));
  startup_info.cb = sizeof(STARTUPINFO);
  startup_info.dwFlags = STARTF_USESTDHANDLES;
  startup_info.wShowWindow = SW_HIDE;
  startup_info.hStdOutput = write_pipe;
  startup_info.hStdError  = err_write_pipe;
  SetLastError(NO_ERROR);
  if (CreateProcess(0,command.get(),0,0,true,DETACHED_PROCESS, 0, 0, &startup_info, &process_info)) {
    char buf_stdout[8192], buf_errout[8192];
    DWORD dstdout = 0, derrout = 0;
    DWORD ret;
  
    while ( (ret = WaitForSingleObject(process_info.hProcess, 0)) != WAIT_ABANDONED) {
        memset(buf_stdout, 0, sizeof(buf_stdout));
        memset(buf_errout, 0, sizeof(buf_errout));

        PeekNamedPipe(read_pipe, 0, 0, 0, &dstdout, 0);
        if (dstdout > 0) {
          ReadFile(read_pipe, buf_stdout, sizeof(buf_stdout) - 1, &dstdout, 0);
        }
  
        PeekNamedPipe(err_read_pipe, 0, 0, 0, &derrout, 0);
        if (derrout > 0) {
          ReadFile(err_read_pipe, buf_errout, sizeof(buf_errout) - 1, &derrout, 0);
        }
        sstdout += buf_stdout;
        sstderr += buf_errout;
  
        MSG msg;
        if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE)){
           TranslateMessage(&msg);
           DispatchMessage(&msg);
        }
        if (ret == WAIT_OBJECT_0) break;
    }
  
    DWORD res;
    GetExitCodeProcess(process_info.hProcess, &res);
    if(res != 0) {
      error.setMessage("Command failed.");
      error.setCode(1);
      funcret = res;
    }
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);
  }else{
    ib_platform_set_error(error);
    funcret = -1;
  }
  

  CloseHandle(write_pipe);
  CloseHandle(read_pipe);
  CloseHandle(err_write_pipe);
  CloseHandle(err_read_pipe);
  return funcret;
} // }}}

int ib::platform::show_context_menu(ib::oschar *path){ // {{{
  HRESULT             ret;
  POINT               pt;
  HMENU               hmenu;
  IContextMenu        *context_menu = NULL;
  IShellFolder        *shell_folder = NULL;
  ITEMIDLIST          *childlist;
  CMINVOKECOMMANDINFO ici;
  ITEMIDLIST          *abslist;
  int                 context_id;
  
  abslist = ILCreateFromPath(path);
  if(abslist == 0) {
    ret = S_FALSE;
    goto finalize;
  }
  SHBindToParent(abslist, IID_IShellFolder, (void **)(&shell_folder), NULL);
  childlist = ILFindLastID(abslist);

  ret = shell_folder->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST *)&childlist, IID_IContextMenu, NULL, (void **)&context_menu);
  if (ret != S_OK) {
    goto finalize;
  }

  hmenu = CreatePopupMenu();
  context_menu->QueryContextMenu(hmenu, 0, 1, 0x7fff, CMF_NORMAL);

  GetCursorPos(&pt);
  context_id = TrackPopupMenu(hmenu, TPM_RETURNCMD, pt.x, pt.y, 0, ib_g_hwnd_main, NULL);
  if (context_id == 0) {
    ret = S_FALSE;
    goto finalize;
  }

  ici.cbSize       = sizeof(CMINVOKECOMMANDINFO);
  ici.fMask        = 0;
  ici.hwnd         = ib_g_hwnd_main;
  ici.lpVerb       = (LPCSTR)MAKEINTRESOURCE(context_id - 1);
  ici.lpParameters = NULL;
  ici.lpDirectory  = NULL;
  ici.nShow        = SW_SHOW;
  
  ret = context_menu->InvokeCommand(&ici);

finalize:
  if(context_menu != NULL) context_menu->Release();
  if(shell_folder != NULL) shell_folder->Release();
  if(abslist != NULL) ILFree(abslist);

  return ret == S_OK ? 0 : 1;
} // }}}

void ib::platform::on_command_init(ib::Command *cmd) { // {{{
  ib::Regex lnk_reg("^.*\\.lnk$", ib::Regex::I);
  lnk_reg.init();
  if(lnk_reg.match(cmd->getCommandPath().c_str()) == 0){
    ib::oschar command_path[IB_MAX_PATH];
    ib::platform::utf82oschar_b(command_path, IB_MAX_PATH, cmd->getCommandPath().c_str());
    IShellLink* shell_link_if = NULL;
    IPersistFile* persist_file_if = NULL;
    HRESULT result;
    CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void**)&shell_link_if);
    shell_link_if->QueryInterface(IID_IPersistFile, (void**)&persist_file_if);
    result = persist_file_if->Load(command_path, TRUE);
    if(SUCCEEDED(result)){
      ib::oschar real_path[IB_MAX_PATH];
      shell_link_if->GetPath(real_path, IB_MAX_PATH, 0, 0);
      if(_tcslen(real_path) != 0 && ib::platform::is_path(real_path)){
        char new_command_path[IB_MAX_PATH_BYTE];
        ib::platform::oschar2utf8_b(new_command_path, IB_MAX_PATH_BYTE, real_path);
        ib::Regex ignored_reg("C:\\\\Windows\\\\Installer", ib::Regex::I);
        ignored_reg.init();
        if(ignored_reg.match(new_command_path) != 0){
          ib::oschar real_args[IB_MAX_PATH];
          shell_link_if->GetArguments(real_args, IB_MAX_PATH);

          ib::oschar real_path_quoted[IB_MAX_PATH];
          ib::platform::quote_string(real_path_quoted, real_path);

          char new_command_path_quoted[IB_MAX_PATH_BYTE];
          ib::platform::oschar2utf8_b(new_command_path_quoted, IB_MAX_PATH_BYTE, real_path_quoted);
          char new_args[IB_MAX_PATH_BYTE];
          ib::platform::oschar2utf8_b(new_args, IB_MAX_PATH_BYTE, real_args);

          cmd->setCommandPath(new_command_path);
          std::string full_path;
          full_path += new_command_path_quoted;
          if(strlen(new_args) != 0){
            full_path += " ";
            full_path += new_args;
          }
          cmd->setPath(full_path);
        }
      }

      ib::oschar real_workdir[IB_MAX_PATH];
      shell_link_if->GetWorkingDirectory(real_workdir, IB_MAX_PATH);
      ib::oschar real_description[IB_MAX_PATH];
      shell_link_if->GetDescription(real_description, IB_MAX_PATH);

      if(ib::platform::is_path(real_workdir)){
        char new_workdir[IB_MAX_PATH_BYTE];
        ib::platform::oschar2utf8_b(new_workdir, IB_MAX_PATH_BYTE, real_workdir);
        cmd->setWorkdir(new_workdir);
      }

      if(_tcslen(real_description) != 0){
        char new_description[IB_MAX_PATH_BYTE];
        ib::platform::oschar2utf8_b(new_description, IB_MAX_PATH_BYTE, real_description);
        cmd->setDescription(new_description);
      }
    }
    persist_file_if->Release();
    if(shell_link_if) shell_link_if->Release();
  }
} // }}}

ib::oschar* ib::platform::default_config_path(ib::oschar *result) { // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  ib::oschar osbuf[IB_MAX_PATH];
  ib::platform::get_self_path(osbuf);
  ib::platform::dirname(result, osbuf);
  return result;
} // }}}

ib::oschar* ib::platform::resolve_icon(ib::oschar *result, ib::oschar *file, int size){ // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  tcsncpy_s(result, file, IB_MAX_PATH);
  return result;
} // }}}

//////////////////////////////////////////////////
// path functions {{{
//////////////////////////////////////////////////
Fl_Image* ib::platform::get_associated_icon_image(const ib::oschar *path, const int size){ // {{{
   SHFILEINFO shinfo;
   Fl_RGB_Image *result_image = 0;
   uchar* data = 0;
   HDC hdc1 = 0;
   HDC hdc2 = 0;
   HBITMAP hbmp = 0;
   HICON hicon = 0;

   int special_folder = -1;
   if(_tcscmp(path, L"::{20D04FE0-3AEA-1069-A2D8-08002B30309D}") == 0){
     special_folder = CSIDL_DRIVES;
   }else if(_tcscmp(path, L"::{208D2C60-3AEA-1069-A2D7-08002B30309D}") == 0){
     special_folder = CSIDL_NETWORK;
   }else if(_tcscmp(path, L"::{645FF040-5081-101B-9F08-00AA002F954E}") == 0){
     special_folder = CSIDL_BITBUCKET;
   }else if(_tcscmp(path, L"::{450D8FBA-AD25-11D0-98A8-0800361B1103}") == 0){
     special_folder = CSIDL_PERSONAL;
   }else if(_tcscmp(path, L"::{2227A280-3AEA-1069-A2DE-08002B30309D}") == 0){
     special_folder = CSIDL_PRINTERS;
   }

   const int size_flag = size > 16 ? SHGFI_LARGEICON : SHGFI_SMALLICON;

   if(special_folder > -1){
     LPITEMIDLIST pidl;
     if(SHGetSpecialFolderLocation(NULL, special_folder, &pidl) != S_OK) {
       goto label_finalize;
     }
     if(!SHGetFileInfo((LPCWSTR)pidl, 0, &shinfo, sizeof(SHFILEINFO), SHGFI_PIDL | SHGFI_ICON | size_flag)) {
       goto label_finalize;
     }
   }else{
     ib::Regex reg(".*\\.(cpl)", ib::Regex::NONE);
     reg.init();
     char native_path[IB_MAX_PATH_BYTE];
     ib::platform::oschar2utf8_b(native_path, IB_MAX_PATH_BYTE, path);
     if(reg.match(native_path) == 0){
       ExtractIconEx(path, 0, &hicon, 0, 1);
     }else{
       const int attr = ib::platform::directory_exists(path) ? FILE_ATTRIBUTE_DIRECTORY
                                                             : FILE_ATTRIBUTE_NORMAL;
       const int flags = SHGFI_ICON | size_flag | SHGFI_USEFILEATTRIBUTES;
       if(!SHGetFileInfo(path, attr, &shinfo, sizeof(SHFILEINFO), flags)) {
         goto label_finalize;
       }
     }
   }
   if(hicon == 0) hicon = shinfo.hIcon;
   hdc1 = GetDC(0);
   hbmp = CreateCompatibleBitmap(hdc1, size, size);
   hdc2 =  CreateCompatibleDC(hdc1);
   SelectObject(hdc2, hbmp);
   DrawIconEx(hdc2, 0, 0, hicon, size, size, 0, 0, DI_NORMAL); 

   data = ib_platform_read_hbitmap(hbmp, 0, 0, size, size, 1);
   result_image = new Fl_RGB_Image((const uchar*)data, size, size, 4);
   result_image->alloc_array = 1;

label_finalize:
   if(hbmp != 0) { DeleteObject(hbmp); }
   if(hicon != 0){ DestroyIcon(hicon); }
   if(hdc1 != 0){ DeleteDC(hdc1); }
   if(hdc2 != 0){ DeleteDC(hdc2); }
   return result_image;
} /* }}} */

ib::oschar* ib::platform::join_path(ib::oschar *result, const ib::oschar *parent, const ib::oschar *child) { // {{{
  if(result == 0){
    result = new ib::oschar[IB_MAX_PATH];
  }
  tcsncpy_s(result, parent, IB_MAX_PATH);
  result[IB_MAX_PATH-1]= '\0';
  std::size_t length = _tcslen(result);
  const ib::oschar *sep;
  if(result[length-1] != L'/' && result[length-1] != L'\\') {
    sep = L"\\";
  }else{
    sep = L"";
  }
  _tcsncat(result, sep, IB_MAX_PATH-length);
  _tcsncat(result, child, IB_MAX_PATH-length-1);
  return result;
} // }}}

ib::oschar* ib::platform::normalize_path(ib::oschar *result, const ib::oschar *path){ // {{{
  ib::oschar tmp[IB_MAX_PATH];
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  const std::size_t length = _tcslen(path);
  bool is_dot_path = length > 1 && path[0] == L'.' && (path[1] == L'/' || path[1] == L'\\');
  if(!is_dot_path) is_dot_path = length == 1 && path[0] == L'.';
  bool is_dot_dot_path = length > 2 && path[0] == L'.' && path[1] == L'.' && (path[2] == L'/' || path[2] == L'\\');
  if(!is_dot_dot_path) is_dot_dot_path = length == 2 && path[0] == L'.' && path[1] == L'.';

  for(std::size_t i =0; i < length; ++i){
    if(path[i] == L'/') tmp[i] = L'\\';
    else tmp[i] = path[i];
  }
  if(is_dot_path){
    tmp[0] = L'_';
  }else if(is_dot_dot_path){
    tmp[0] = L'_'; tmp[1] = L'_';
  }
  tmp[length] = L'\0';
  if(!is_dot_path && !is_dot_dot_path){
    PathCanonicalize(result, tmp);
  }else{
    tcsncpy_s(result, tmp, IB_MAX_PATH);
  }
  if(is_dot_path){
    result[0] = L'.';
  }else if(is_dot_dot_path){
    result[0] = L'.'; result[1] = L'.';
  }
  return result;
} // }}}

ib::oschar* ib::platform::normalize_join_path(ib::oschar *result, const ib::oschar *parent, const ib::oschar *child){ // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  ib::oschar tmp[IB_MAX_PATH];
  ib::platform::join_path(tmp, parent, child);
  ib::platform::normalize_path(result, tmp);
  return result;
} // }}}

ib::oschar* ib::platform::dirname(ib::oschar *result, const ib::oschar *path){ // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  tcsncpy_s(result, path, IB_MAX_PATH);
  const std::size_t len = _tcslen(path);
  if(len == 0) return result;
  std::size_t i = len -1;
  for(; i > 0; --i){
    if(result[i] == L'\\' || result[i] == L'/'){
      break;
    }
  }
  result[i] = L'\0';
  if(_tcscmp(result, L"\\") == 0){
    _tcscpy(result, L"\\\\");
  }
  return result;
} // }}}

ib::oschar* ib::platform::basename(ib::oschar *result, const ib::oschar *path){ // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  const std::size_t len = _tcslen(path);
  if(len == 0) return result;
  std::size_t i = len-1;
  for(; i > 0; --i){
    if(path[i] == L'\\' || path[i] == L'/'){
      break;
    }
  }
  _tcscpy(result, path+i+1);
  return result;
} // }}}

ib::oschar* ib::platform::to_absolute_path(ib::oschar *result, const ib::oschar *dir, const ib::oschar *path) { // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  if(ib::platform::is_relative_path(path)){
    ib::platform::normalize_join_path(result, dir, path);
  }else{
    tcsncpy_s(result, path, IB_MAX_PATH);
  }
  return result;
} // }}}

bool ib::platform::is_directory(const ib::oschar *path) { // {{{
  const size_t length = _tcslen(path);
  return (path[length-1] == L'/' || path[length-1] == L'\\' || PathIsDirectory(path));
} // }}}

bool ib::platform::is_path(const ib::oschar *wstr){ // {{{
  bool ret = false;
  if(_tcslen(wstr) >= 3){
    if(iswalpha(wstr[0]) && wstr[1] == L':' && (wstr[2] == L'/' || wstr[2] == L'\\')){
      ret = true;
    } else if(wstr[0] == L'.' && wstr[1] == '.' && (wstr[2] == L'/' || wstr[2] == L'\\')){
      ret = true;
    } else if(wstr[0] == L'.' && (wstr[1] == L'/' || wstr[1] == L'\\')){
      ret = true;
    } else if(wstr[0] == L'\\' && wstr[1] == L'\\'){
      ret = true;
    }
  }else if(_tcslen(wstr) >= 2){
    if(wstr[0] == L'.' && (wstr[1] == L'/' || wstr[1] == L'\\')){
      ret = true;
    }else if(wstr[0] == L'\\' && wstr[1] == L'\\'){
      ret = true;
    }
  }
  return ret;
} // }}}

bool ib::platform::is_relative_path(const ib::oschar *path) { // {{{
  return PathIsRelative(path) == TRUE;
} // }}}

bool ib::platform::directory_exists(const ib::oschar *path) { // {{{
  return PathIsDirectory(path) != 0 || _tcscmp(path, L"\\\\") == 0;
} // }}}

bool ib::platform::file_exists(const ib::oschar *path) { // {{{
  return path_exists(path) && !PathIsDirectory(path);
} // }}}

bool ib::platform::path_exists(const ib::oschar *path) { // {{{
  if(PathFileExists(path) != 0) return true;
  HANDLE hfind;
  WIN32_FIND_DATA find_data;
  hfind = FindFirstFile( path, &find_data );
  if(hfind != INVALID_HANDLE_VALUE) return true;
  // TODO should check wether a given drive exists
  if(_tcslen(path) == 3){
    if(iswalpha(path[0]) && path[1] == L':' && (path[2] == L'/' || path[2] == L'\\')){
      return true;
    }
  }
  return false;
} // }}}

int ib::platform::walk_dir(std::vector<ib::unique_oschar_ptr> &result, const ib::oschar *dir, ib::Error &error, bool recursive) { // {{{

  const std::size_t dir_length = _tcslen(dir);
  const ib::oschar *sep = (dir[dir_length-1] == L'\\' || dir[dir_length-1] == L'/') ? L"" : L"\\";
  if(_tcscmp(dir, L"\\\\") == 0){
    if(!recursive){
      std::vector<ib::oschar*> servers;
      ib_platform_list_network_servers(servers, NULL, SV_TYPE_WORKSTATION | SV_TYPE_SERVER);
      for(auto it = servers.begin(), last = servers.end(); it != last; ++it){
        result.push_back(ib::unique_oschar_ptr(*it));
      }
      return 0;
    }else{
      error.setCode(2);
      error.setMessage("Recursive option with UNC path is not supported.");
      return 2;
    }
  }else{
    if(dir_length > 2 && dir[0] == L'\\' && dir[1] == L'\\'){
      bool flag = true;
      for(std::size_t i = 2; i < dir_length; ++i) {if(dir[i] == L'\\'){ flag = false; break;}}
      if(flag){
        if(!recursive){
          std::vector<ib::oschar*> shares;
          ib::oschar not_const_dir[IB_MAX_PATH];
          tcsncpy_s(not_const_dir, dir, IB_MAX_PATH);
          ib_platform_list_network_shares(shares, not_const_dir);
          for(auto it = shares.begin(), last = shares.end(); it != last; ++it){
            result.push_back(ib::unique_oschar_ptr(*it));
          }
          return 0;
        }else{
          error.setCode(2);
          error.setMessage("Recursive option with UNC path is not supported.");
          return 2;
        }
      }
    }
  }

  ib::oschar pattern[IB_MAX_PATH];
  swprintf(pattern, L"%ls%ls*", dir, sep);
  ib::oschar *tmp_full_path;

  WIN32_FIND_DATA fd;
  HANDLE h;

  OSVERSIONINFO OSver; 
  OSver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO); 
  GetVersionEx(&OSver);
  if((OSver.dwMajorVersion == 6 && OSver.dwMinorVersion >= 1) || OSver.dwMajorVersion > 6){
    // supported in windows7, windows2008R2 or above
    h = FindFirstFileEx(pattern, FindExInfoBasic, &fd, FindExSearchNameMatch, NULL, 0);
  }else{
    h = FindFirstFileEx(pattern, FindExInfoStandard, &fd, FindExSearchNameMatch, NULL, 0);
  }
  if (INVALID_HANDLE_VALUE == h) {
    error.setCode(1);
    error.setMessage("Failed to read directory.");
    return 1;
  }else{
    do {
      std::size_t file_name_length = _tcslen(fd.cFileName);
      if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) {
        if(_tcscmp(fd.cFileName, L".") && _tcscmp(fd.cFileName, L"..")){
          if(recursive){
            tmp_full_path = new ib::oschar[dir_length+1+file_name_length+1];
            swprintf(tmp_full_path, L"%ls%ls%ls", dir, sep, fd.cFileName);
            result.push_back(ib::unique_oschar_ptr(tmp_full_path));
            if(ib::platform::walk_dir(result, tmp_full_path, error, recursive) != 0){
              return 1;
            };
          }else{
            tmp_full_path = new ib::oschar[file_name_length+1];
            swprintf(tmp_full_path, L"%ls", fd.cFileName);
            result.push_back(ib::unique_oschar_ptr(tmp_full_path));
          }
        }
      }else {
        if(recursive){
          tmp_full_path = new ib::oschar[dir_length+1+file_name_length+1];
          swprintf(tmp_full_path, L"%ls%ls%ls", dir, sep, fd.cFileName);
          result.push_back(ib::unique_oschar_ptr(tmp_full_path));
        }else{
          tmp_full_path = new ib::oschar[file_name_length+1];
          swprintf(tmp_full_path, L"%ls", fd.cFileName);
          result.push_back(ib::unique_oschar_ptr(tmp_full_path));
        }
      }
    }while ( FindNextFile( h, &fd ) );
    FindClose( h );
  }
  return 0;
} // }}}

ib::oschar* ib::platform::get_self_path(ib::oschar *result){ // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  GetModuleFileName(ib_g_hinst, result, IB_MAX_PATH-1);
  return result;
} // }}}

ib::oschar* ib::platform::get_current_workdir(ib::oschar *result){ // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  GetCurrentDirectory(IB_MAX_PATH-1, result);
  return result;
} // }}}

int ib::platform::set_current_workdir(const ib::oschar *dir, ib::Error &error){ // {{{
  if(SetCurrentDirectory(dir) == 0){
    ib_platform_set_error(error);
    return -1;
  }else{
    return 0;
  }
} // }}}

bool ib::platform::which(ib::oschar *result, const ib::oschar *name) { // {{{
  const int MAX_ENV_SIZE = 32767;
  ib::oschar path_buf[MAX_ENV_SIZE];
  ib::oschar ext_buf[MAX_ENV_SIZE];
  ib::oschar fullpath_buf[IB_MAX_PATH];
  size_t size;

  size = GetEnvironmentVariable(L"PATH", path_buf, MAX_ENV_SIZE);
  path_buf[size] = L'\0';
  size = GetEnvironmentVariable(L"PATHEXT", ext_buf, MAX_ENV_SIZE);
  ext_buf[size] = L'\0';

  bool has_ext = _tcsstr(name, L".") != 0;
  bool found = false;
  ib::oschar *path_next = 0;
  ib::oschar *path_current = path_buf;
  do{ 
    path_next = tcstokread(path_current, L';');
    if(!has_ext){
      ib::oschar *ext_next = 0;
      ib::oschar *ext_current = 0;
      ib::oschar ext_buf2[MAX_ENV_SIZE];
      tcsncpy_s(ext_buf2, ext_buf, MAX_ENV_SIZE);
      ext_current = ext_buf2;
      do {
        ext_next = tcstokread(ext_current, L';');
        ib::platform::normalize_join_path(fullpath_buf, path_current, name);
        _tcscat(fullpath_buf, ext_current);
        if(ib::platform::file_exists(fullpath_buf)){
          tcsncpy_s(result, fullpath_buf, IB_MAX_PATH);
          found = true;
          goto finalize;
        }
        ext_current = ext_next; 
      }while(*ext_next != L'\0');
    }else{
      ib::platform::normalize_join_path(fullpath_buf, path_current, name);
      if(ib::platform::file_exists(fullpath_buf)){
        tcsncpy_s(result, fullpath_buf, IB_MAX_PATH);
        found = true;
        goto finalize;
      }
    }
    path_current = path_next; 
  }while(*path_next != L'\0');

finalize:
  return found;
} // }}}

ib::oschar* ib::platform::icon_cache_key(ib::oschar *result, const ib::oschar *path) { // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  ib::oschar file_type[IB_MAX_PATH];
  ib::platform::file_type(file_type, path);
  // PathIsDirectory returns true even if path is a drive.
  // Because of it, should use FindFirstFile & FILE_ATTRIBUTE_DIRECTORY
  HANDLE hfind;
  WIN32_FIND_DATA find_data;
  hfind = FindFirstFile( path, &find_data );
  if(hfind != INVALID_HANDLE_VALUE && find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
    _tcscpy(result, L":folder:common");
  }else if(_tcsicmp(file_type, L"exe") == 0 || _tcsicmp(file_type, L"lnk") == 0 || _tcsicmp(file_type, L"ico") == 0) {
    _tcscpy(result, path);
  }else{
    if(_tcslen(path) == 3 && iswalpha(path[0]) && path[1] == L':' && (path[2] == L'/' || path[2] == L'\\')){
      swprintf(result, L":folder:drives");
    }else if(_tcslen(path) == 2 && iswalpha(path[0]) && path[1] == L':'){
      swprintf(result, L":folder:drives");
    }else if(_tcslen(file_type) > 0){
      swprintf(result, L":filetype:%ls", file_type);
    }else{
      _tcscpy(result, path);
    }
  }
  return result;
} // }}}

//////////////////////////////////////////////////
// path functions }}}
//////////////////////////////////////////////////

//////////////////////////////////////////////////
// filesystem functions {{{
//////////////////////////////////////////////////
int ib::platform::remove_file(const ib::oschar *path, ib::Error &error){ // {{{
  SetLastError(NO_ERROR);
  int ret = DeleteFile(path);
  if(ret == 0){
    ib_platform_set_error(error);
    return 1;
  };
  return 0;
} // }}}

int ib::platform::copy_file(const ib::oschar *source, const ib::oschar *dest, ib::Error &error){ // {{{
  SetLastError(NO_ERROR);
  int ret = CopyFile(source, dest, FALSE);
  if(ret == 0){
    ib_platform_set_error(error);
    return 1;
  };
  return 0;
} // }}}

int ib::platform::file_size(size_t &size, const ib::oschar *path, ib::Error &error){ // {{{
  SetLastError(NO_ERROR);
  HANDLE file = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if(file == INVALID_HANDLE_VALUE ){
    ib_platform_set_error(error);
    return 1;
  }
  long sz = GetFileSize(file, NULL );
  if(sz == -1){
    ib_platform_set_error(error);
    return 1;
  }
  size = sz;
  return 0;
} // }}}

ib::oschar* ib::platform::file_type(ib::oschar *result, const ib::oschar *path){ // {{{
  if(result == 0){ result = new ib::oschar[IB_MAX_PATH]; }
  ib::oschar tmp[IB_MAX_PATH];
  ib::platform::basename(tmp, path);
  ib::oschar *ptr = PathFindExtensionW(tmp);
  memset(result, 0, 1);
  if(ptr != tmp && *ptr != L'\0') { 
    ptr++; 
    _tcscpy(result, ptr);
  }
  return result;
} // }}}
//////////////////////////////////////////////////
// filesystem functions }}}
//////////////////////////////////////////////////

//////////////////////////////////////////////////
// thread functions {{{
//////////////////////////////////////////////////
void ib::platform::create_thread(ib::thread *t, ib::threadfunc f, void* p) { // {{{
  *t = (ib::thread)_beginthread(f, 0, p);
}
/* }}} */

void ib::platform::on_thread_start(){ /* {{{ */
  CoInitializeEx(0, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
} /* }}} */

void ib::platform::join_thread(ib::thread *t){ /* {{{ */
  WaitForSingleObject((void*)*t, INFINITE);
} /* }}} */

void ib::platform::exit_thread(int exit_code) { // {{{
  CoUninitialize();
  _endthread();
} // }}}

void ib::platform::create_mutex(ib::mutex *m) { /* {{{ */
  InitializeCriticalSection(m);
} /* }}} */

void ib::platform::destroy_mutex(ib::mutex *m) { /* {{{ */
  DeleteCriticalSection(m);
} /* }}} */

void ib::platform::lock_mutex(ib::mutex *m) { /* {{{ */
  EnterCriticalSection(m);
} /* }}} */

void ib::platform::unlock_mutex(ib::mutex *m) { /* {{{ */
  LeaveCriticalSection(m);
} /* }}} */

void ib::platform::create_cmutex(ib::cmutex *m) { /* {{{ */
  *m = CreateMutex(NULL,FALSE,NULL);
} /* }}} */

void ib::platform::destroy_cmutex(ib::cmutex *m) { /* {{{ */
  ReleaseMutex(*m);
} /* }}} */

void ib::platform::lock_cmutex(ib::cmutex *m) { /* {{{ */
  WaitForSingleObject(*m, INFINITE); 
} /* }}} */

void ib::platform::unlock_cmutex(ib::cmutex *m) { /* {{{ */
  ReleaseMutex(*m);
} /* }}} */

void ib::platform::create_condition(ib::condition *c) { /* {{{ */
  c->waiters = 0;
  c->was_broadcast = 0;
  c->sema = CreateSemaphore (NULL, 0, LONG_MAX, NULL);
  InitializeCriticalSection (&c->waiters_lock);
  c->waiters_done = CreateEvent (NULL, FALSE, FALSE, NULL);
} /* }}} */

void ib::platform::destroy_condition(ib::condition *c) { /* {{{ */
  CloseHandle(c->sema);
  CloseHandle(c->waiters_done);
  DeleteCriticalSection(&c->waiters_lock);
} /* }}} */

int ib::platform::wait_condition(ib::condition *c, ib::cmutex *m, int ms) { /* {{{ */
  EnterCriticalSection (&c->waiters_lock);
  c->waiters++;
  LeaveCriticalSection (&c->waiters_lock);
  int ret = (SignalObjectAndWait (*m, c->sema, ms==0?INFINITE:ms, FALSE) == WAIT_TIMEOUT) ? 1 : 0;
  EnterCriticalSection (&c->waiters_lock);
  c->waiters--;
  int last_waiter = c->was_broadcast && c->waiters == 0;
  LeaveCriticalSection (&c->waiters_lock);
  if (last_waiter)
    SignalObjectAndWait (c->waiters_done, *m, INFINITE, FALSE);
  else
    WaitForSingleObject (*m, INFINITE);
  return ret;
} /* }}} */

void ib::platform::notify_condition(ib::condition *c) { /* {{{ */
  EnterCriticalSection (&c->waiters_lock);
  int have_waiters = 0;
  if (c->waiters > 0) {
    c->was_broadcast = 1;
    have_waiters = 1;
  }
  if (have_waiters) {
    ReleaseSemaphore(c->sema, 1, 0);
    LeaveCriticalSection (&c->waiters_lock);
    WaitForSingleObject(c->waiters_done, INFINITE);
    c->was_broadcast = 0;
  } else {
    LeaveCriticalSection (&c->waiters_lock);
  }
} /* }}} */

//////////////////////////////////////////////////
// thread functions }}}
//////////////////////////////////////////////////

//////////////////////////////////////////////////
// process functions {{{
//////////////////////////////////////////////////
int ib::platform::wait_pid(const int pid) { // {{{
  HANDLE hprocess = OpenProcess(SYNCHRONIZE, FALSE, pid);
  if (hprocess != NULL) {
    if (WaitForSingleObject(hprocess, 10 * 1000) == WAIT_TIMEOUT) {
      return -1;
    }
  }
  return 0;
} // }}}

int ib::platform::get_pid() { // {{{
  return GetCurrentProcessId();
} // }}}

//////////////////////////////////////////////////
// process functions }}}
//////////////////////////////////////////////////

//////////////////////////////////////////////////
// dynamic loading functions {{{
//////////////////////////////////////////////////
int ib::platform::load_library(ib::module &dl, const ib::oschar *name, ib::Error &error) { // {{{
  SetLastError(NO_ERROR);
  dl = LoadLibrary(name);
  if(dl == 0){
    ib_platform_set_error(error);
    return 1;
  }
  return 0;
} // }}}

void* ib::platform::get_dynamic_symbol(ib::module dl, const char *name){ //{{{
  return (void*)GetProcAddress(dl, name);
} //}}}

void ib::platform::close_library(ib::module dl) { //{{{
  FreeLibrary(dl);
} //}}}

//////////////////////////////////////////////////
// dynamic loading functions }}}
//////////////////////////////////////////////////

//////////////////////////////////////////////////
// socket functions {{{
//////////////////////////////////////////////////
void ib::platform::close_socket(FL_SOCKET s){ // {{{
  closesocket(s);
} // }}}

//////////////////////////////////////////////////
// socket functions }}}
//////////////////////////////////////////////////

//////////////////////////////////////////////////
// system functions {{{
//////////////////////////////////////////////////
int ib::platform::get_num_of_cpu(){ // {{{
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return (int)info.dwNumberOfProcessors;
} // }}}

int ib::platform::convert_keysym(int key){ // {{{
  return key;
} // }}}

//////////////////////////////////////////////////
// system functions }}}
//////////////////////////////////////////////////

//////////////////////////////////////////////////
// platform specific functions {{{
//////////////////////////////////////////////////
void ib::platform::win_draw_text(ib::oschar *str, int x, int y, int w, int h){ // {{{
  COLORREF oldColor = SetTextColor(fl_gc, fl_RGB());
  SelectObject(fl_gc, fl_graphics_driver->font_descriptor()->fid);
  RECT rect;
  rect.left = x;
  rect.top  = y;
  if(w != 0) rect.right = x + w;
  if(h != 0) {
    rect.bottom  = y + h;
  }
  //TextOut(fl_gc, rect.left, rect.top, str, _tcslen(str));
  DrawTextW(fl_gc, str, -1, &rect, DT_NOCLIP | DT_TOP | DT_SINGLELINE | DT_LEFT | DT_PATH_ELLIPSIS | DT_NOPREFIX);
  SetTextColor(fl_gc, oldColor);
} // }}}

size_t ib::platform::win_calc_text_width(ib::oschar *str){ // {{{
  COLORREF oldColor = SetTextColor(fl_gc, fl_RGB());
  SelectObject(fl_gc, fl_graphics_driver->font_descriptor()->fid);
  RECT rect;
  DrawTextW(fl_gc, str, -1, &rect, DT_NOCLIP | DT_TOP | DT_SINGLELINE | DT_LEFT | DT_CALCRECT);
  SetTextColor(fl_gc, oldColor);
  return (rect.right - rect.left);

} // }}}

int ib::platform::list_drives(std::vector<ib::unique_oschar_ptr> &result, ib::Error &error) { // {{{
  ib::oschar buf[128]; 
  ib::oschar *ptr;
  ib::oschar *tmp;
  size_t     length;

  SetLastError(NO_ERROR);
  if(GetLogicalDriveStrings(sizeof(buf), buf) == 0){
    ib_platform_set_error(error);
    return 1;
  }

  for(ptr = buf; *ptr != L'\0' ; ptr++){
    length = _tcslen(ptr);
    tmp = new ib::oschar[length+1];
    swprintf(tmp, L"%ls", ptr);
    result.push_back(ib::unique_oschar_ptr(tmp));
    ptr += length;
  }
  return 0;
} // }}}

//////////////////////////////////////////////////
// platform specific functions }}}
//////////////////////////////////////////////////
