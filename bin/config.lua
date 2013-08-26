local script_path = _iceberg_config_dir()
package.path = script_path .. [[\luamodule\?.lua;]] .. package.path
package.cpath = script_path ..[[\luamodule\?.dll;]] .. package.path
local ibs = require("icebergsupport")
local wins = require("winsupport")
local winalttab = require("winalttab")

-- configurations --
system = {
  default_search_path_depth = 2,
  enable_icons = true,
  max_cached_icons = 3000,
  key_event_threshold = 0,
  max_histories = 500,
  max_candidates = 15,
  history_factor = 0.8,
  file_browser = [[explorer ${1}]],
  server_port = 40000,

  hot_key = "ctrl-space",
  escape_key = "escape",
  list_next_key = "ctrl-n",
  list_prev_key = "ctrl-p",
  toggle_mode_key = "ctrl-r",
  kill_word_key = "ctrl-w",

  search_path = {
    {category="shortcuts", path = script_path .. [[\shortcuts]], depth = 5, pattern=[[^.*\.(exe|lnk|bat)$]]},
    {category="system", path = [[C:\Windows\System32]], depth = 1, pattern="^.*\\.(exe)$"}, 
    {category="programs", path = [[C:\Users\]] .. os.getenv("USERNAME") .. [[\AppData\Roaming\Microsoft\Windows\Start Menu\Programs]], depth = 10, pattern=[[^.*\.(exe|lnk)$]]},
    {category="programs", path = [[C:\ProgramData\Microsoft\Windows\Start Menu\Programs]], depth = 10, pattern=[[^.*\.(exe|lnk)$]]},
  },
  completer = {
    command = ibs.COMP_ABBR,
    path    = ibs.COMP_PARTIAL,
    history = ibs.COMP_PARTIAL,
    option  = ibs.COMP_PARTIAL,

    option_func = {
      [":scan_search_path"] = function(values) 
        local candidates = {"all"}
        local keys       = {all = true}
        for i, value in ipairs(system.search_path) do
          if value.category ~= nil and keys[value.category] == nil then
            table.insert(candidates, value.category)
            keys[value.category] = true
          end
        end
        return candidates
      end, 
      ["alttab"] = function(values)
        local candidates = {}
        for i, tb in ipairs(winalttab.list()) do
          local title = tb.title .. " (ID:" .. tb.hwnd .. ")"
          local value = {value = title, description = tb.path, icon=tb.path}
          table.insert(candidates, value)
        end
        return candidates
      end
    }
  }
}

styles = {
  window_boxtype = ibs.BOXTYPE_BORDER_BOX, 
  window_posx  = nil,
  window_posy  = nil,
  window_width = 300,
  window_height = 40,
  window_padx  = 7,
  window_pady  = 7,
  window_bg_color = {238, 238, 238},
  window_alpha = 220,
  taskbar_height = 40,

  input_boxtype = ibs.BOXTYPE_FLAT_BOX,
  input_font  = "メイリオ",
  input_font_size  = 14,
  input_font_color  = {238, 238, 238},
  input_bg_color  = {33, 33, 33},
  input_selection_bg_color = {99,99,99},

  list_boxtype = ibs.BOXTYPE_BORDER_BOX,
  list_padx = 5,
  list_pady = 5,
  list_border_color = {238,238,238},
  list_font  = "メイリオ",
  list_font_size  = 15,
  list_desc_font_size  = 11,
  list_font_color  = {238, 238, 238},
  list_desc_font_color  = {220, 220, 220},
  list_selection_font_color  = {238, 238, 238},
  list_selection_desc_font_color  = {220, 220, 220},
  list_bg_color1  = {50, 50, 50},
  list_selection_bg_color1  = {99,99,99},
  list_bg_color2  = {33, 33, 33},
  list_selection_bg_color2  = {99,99,99},
  list_alpha = 220
}

commands = { 
  windir = {path = [[C:\Windows]]},
  np = {path = [[notepad.exe]], description="Notepad"},
  userdir = {path = [[C:\Users\]] .. os.getenv("USERNAME")},
  lua_sample = { path = function(args) 
    local explorer = wins.foreground_explorer()
    if explorer then
      ibs.message(ibs.table_to_string(explorer))
    end
  end ,description="Sample Lua command"},
  group_sample = { path = ibs.group_command({"userdir", {}}, {"np", {}}), description = "runs a group of commands"},


  -- default commands --
  [":version"] = {path = function(args) ibs.message(ibs.version()) end, description="show iceberg version", history=false},
  [":exit"] = {path = function(args) ibs.exit_application(0) end, description="exit iceberg", history=false},
  [":reboot"] = {path = function(args) ibs.reboot_application() end, description="reboot iceberg", history=false},
  [":pwd"] = {path = function(args) ibs.set_result_text(ibs.get_cwd()) end, description="show current work directory", history=false},
  [":cd"] = { path = function(args)
      if #args == 0 then
        ibs.message("Usage: :cd PATH")
        return 1
      end
      local path = ibs.to_directory_path(args[1])
      assert(ibs.set_cwd(path))
      ibs.set_result_text(ibs.get_cwd())
      return 0
    end, history = false},
  [":scan_search_path"] = {path = function(args) 
    if #args == 0 then
      ibs.message("Usage: :scan_search_path CATEGORY")
      return 1
    end
    ibs.scan_search_path(args[1]) 
  end, description="scan search paths to find commands", history=false},
  [":empty"] = {path = function(args) ibs.shell_execute(wins.cmd_path, {}, wins.foreground_explorer_path()) end, description = "open command prompt here", history=false, workdir = wins.foreground_explorer_path},
  [":opendir"] = { path = function(args)
      if #args == 0 then
        ibs.message("Usage: :opendir (command name | path)")
        return 1
      end
      local path = ibs.to_directory_path(args[1])
      assert(ibs.open_dir(path))
      return 0
    end, history = false},
  control_panel = {path = [[C:\WINDOWS\system32\control.exe]], description="Control panel"},
  windows_service = {path = [[C:\WINDOWS\system32\services.msc]], description=[[Service]]},
  my_computer = {path = [[::{20D04FE0-3AEA-1069-A2D8-08002B30309D}]], description=[[My computer]]},
  recycle_bin = {path = [[::{645FF040-5081-101B-9F08-00AA002F954E}]], description=[[Recycle bin]]},
  network_computer = {path = [[::{208D2C60-3AEA-1069-A2D7-08002B30309D}]], description=[[Network computer]]},
  google = { path = [[http://www.google.com/search?ie=utf8&q=${1}]], description=[[Searches words on Google]], history=false,
               icon = script_path ..[[\images\google256.png]]},
  mkdir = { path = function(args) 
      if #args == 0 then
        ibs.message("Usage: mkdir DIRNAME")
        return 1
      end
      local dirname = args[1]
      local fexplorer = wins.foreground_explorer()
      if not fexplorer then return 0 end
      local ok, out, err = ibs.command_output([[C:\Windows\System32\cmd.exe /c md "]] .. fexplorer.path.."\\" .. dirname .. [["]])
      if not ok then
        ibs.message(err)
        return 1
      end
      return 0
    end, description = "Usage: mkdir DIRNAME", history=false },
  cal = { path = function(args)
      local script = "ret = (" .. table.concat(args, " ") .. ")"
      local func = loadstring(script)
      if not func then
        ibs.message("Invalid expression.")
        return 1
      end
      func()
      ibs.set_result_text(ret)
      return 0
    end, description = "inline calculator", history = false},
  alttab = { 
    path = function(args) 
      if #args == 0 then return end
      local ok, r = ibs.regex_search([[\s\(ID:([0-9]+)\)]], Regex.NONE, args[1])
      if ok then
        winalttab.activate(tonumber(r:_1()))
      end
    end, 
    description = "ウインドウを切り替えます",
    history=false
  }
}

shortcuts = {
  { key = "ctrl-d", name = ":opendir" },
  { key = "ctrl-l", name = ":cd" }
}

function on_key_up()
  local accept = 0
  return accept
end

function on_key_down()
  local accept = 0
  if accept == 0 then
    accept = ibs.process_shortcut_keys() 
  end

  return accept
end

function on_enter()
  local accept = 0
  return accept
end

function on_initialize()
  local error = 0
  return error
end

require("weather")

