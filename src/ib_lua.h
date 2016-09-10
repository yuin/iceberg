#ifndef __IB_LUA_H__
#define __IB_LUA_H__

#include "ib_constants.h"
#include "ib_utils.h"
#include "ib_server.h"

// Macros {{{
#define IB_LUA (ib::Singleton<ib::MainLuaState>::getInstance()->get())
// }}}

namespace ib {
  class LuaState { // {{{
    public:
      LuaState() : l_(luaL_newstate()), alloced_(true) {}
      LuaState(lua_State *L) : l_(L), alloced_(false) {}
      virtual ~LuaState() { if(alloced_){lua_close(l_);} }
      void init();
      lua_State* get() const {return l_;}
      void clearStack() { lua_pop(l_, lua_gettop(l_)); }
      Fl_Color getColorFromStackTop();

    protected:
      lua_State *l_;
      bool      alloced_;

    private:
      LuaState (const LuaState &rhs) = delete;
      LuaState& operator=(const LuaState &rhs) = delete;
      LuaState (LuaState &&rhs) = delete;
      LuaState& operator=(LuaState &&rhs) = delete;
  }; // }}}

  class MainLuaState : public LuaState, private NonCopyable<MainLuaState> { // {{{
    friend class ib::Singleton<MainLuaState>;
    protected:
      MainLuaState() : LuaState() {}
  }; // }}}

  namespace luamodule { // {{{
    int create(lua_State *L);
    int config_dir(lua_State *L); // void -> string:config path
    int build_platform(lua_State *L); // void -> string:platform_information
    int runtime_platform(lua_State *L); // void -> string:platform_information
    int hide_application(lua_State *L); // void -> void
    int show_application(lua_State *L); // void -> void
    int do_autocomplete(lua_State *L); // void -> void
    int get_cwd(lua_State *L); // void -> string
    int set_cwd(lua_State *L); // string -> success:bool, errmessage:string
    int set_result_text(lua_State *L); // text:string -> void
    int find_command(lua_State *L); // name:string -> found:bool, (commandinfo:table|errmessage:text)
    int to_path(lua_State *L); // name or path:string -> success:bool, (path:string | errmessage:text)
    int message(lua_State *L); // message:string -> void
    int event_key(lua_State *L); // void -> int
    int event_state(lua_State *L); // void -> int(bit field)
    int matches_key(lua_State *L); // string -> bool
    int exit_application(lua_State *L); // int -> void(exit application)
    int reboot_application(lua_State *L); // void -> void(reboot application)
    int scan_search_path(lua_State *L); // category:string -> void
    int get_input_text(lua_State *L); // void -> string
    int set_input_text(lua_State *L); // string -> void
    int get_input_text_values(lua_State *L); // void -> [string]
    int get_clipboard(lua_State *L); // void -> string
    int set_clipboard(lua_State *L); // string -> void
    int get_clipboard_histories(lua_State *L); // void -> clipboard_histories:list
    int shell_execute(lua_State *L); // path:string, args:[string], workdir:string -> bool:success, string:message
    int command_execute(lua_State *L); // name:string, args:[string] -> bool:success, string:message
    int command_output(lua_State *L); // command:string -> bool:success, string:stdout, string:stderr
    int default_after_command_action(lua_State *L); // success:bool, errmessage:text -> void
    int add_history(lua_State *L); //  input:string [, name:string] -> void
    int open_dir(lua_State *L); // path:string -> bool:success, text:errmessage
    int version(lua_State *L); // void -> string
    int selected_index(lua_State *L); // void -> int(start from 1)
    int utf82local(lua_State *L); // string -> string
    int local2utf8(lua_State *L); // string -> string
#ifdef IB_OS_WIN
    int list_all_windows(lua_State *L); // void -> window_handles:list
#endif
    int lock(lua_State *L); // void -> void;
    int unlock(lua_State *L); // void -> void;
    int band(lua_State *L); // values:int[...] -> result:int
    int bor(lua_State *L); // values:int[...] -> result:int
    int bxor(lua_State *L); // values:int[...] -> result:int
    int brshift(lua_State *L); // int,int -> result:int
    int blshift(lua_State *L); // int,int -> result:int
    int dirname(lua_State *L); // string -> string
    int basename(lua_State *L); // string -> string
    int directory_exists(lua_State *L); // string -> bool
    int file_exists(lua_State *L); // string -> bool
    int path_exists(lua_State *L); // string -> bool
    int list_dir(lua_State *L); // path:string -> ok:bool, (files:table or errmessage:text)
    int join_path(lua_State *L); // paths[...]:string -> path:string

  } // }}}

}

#endif
