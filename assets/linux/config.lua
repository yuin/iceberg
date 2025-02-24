local iceberg_config_dir = _iceberg_config_dir()
package.path = iceberg_config_dir .. [[/lualibs/?.lua;]] .. package.path
package.cpath = iceberg_config_dir ..[[/lualibs/?.so;]] .. package.path
local ibs = require("icebergsupport")

-- configurations --
system = {
  default_search_path_depth = 2,
  enable_icons = true,
  icon_theme = "Yaru",
  max_cached_icons = 999999,
  key_event_threshold = 0,
  max_histories = 500,
  max_candidates = 15,
  history_factor = 0.8,
  file_browser = [[/usr/bin/nautilus ${1}]],
  terminal  = [[/usr/bin/gnome-terminal -- ${1}]],
  server_port = 4501,
  path_autocomplete = false,
  option_autocomplete = true,

  hot_key = "ctrl-space", -- in linux, hot_key is not working some GUI env. use 
                          -- GUI's hot key setting to set the hot key.
                          -- Hot key action is `iceberg -m "activate"`
  escape_key = "escape",
  list_next_key = "ctrl-n",
  list_prev_key = "ctrl-p",
  toggle_mode_key = "ctrl-r",
  kill_word_key = "ctrl-w",

  search_path = {
    {category="shortcuts", path = iceberg_config_dir .. [[/shortcuts]], depth = 5, pattern=[[^.*\.(desktop)$]]},
    {category="applications(usr/share)", path = [[/usr/share/applications]], depth = 5, pattern=[[^.*\.(desktop)$]]},
    {category="applications(usr/local)", path = [[/usr/local/share/applications]], depth = 5, pattern=[[^.*\.(desktop)$]]}
  },
  completer = {
    command = ibs.COMP_ABBR,
    path    = ibs.COMP_PARTIAL,
    history = ibs.COMP_PARTIAL,
    option  = ibs.COMP_PARTIAL,

    option_func = {}
  }
}

styles = {
  window_boxtype = ibs.BOXTYPE_BORDER_BOX, 
  window_posx  = nil,
  window_posy  = nil,
  window_width = 400,
  window_height = 50,
  window_padx  = 7,
  window_pady  = 7,
  window_bg_color = {238, 238, 238},
  window_alpha = 220,
  taskbar_height = 40,

  input_boxtype = ibs.BOXTYPE_FLAT_BOX,
  input_font  = "Sans",
  input_font_size  = 24,
  input_font_color  = {238, 238, 238},
  input_bg_color  = {33, 33, 33},
  input_selection_bg_color = {99,99,99},

  list_boxtype = ibs.BOXTYPE_BORDER_BOX,
  list_padx = 5,
  list_pady = 5,
  list_border_color = {238,238,238},
  list_font  = "Sans",
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
  home = {path = os.getenv("HOME"), description="Home directory"},

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
  [":scan_search_path"] = {
    path = function(args) 
      if #args == 0 then
        ibs.message("Usage: :scan_search_path CATEGORY")
        return 1
      end
      ibs.scan_search_path(args[1]) 
    end, 
    completion = function(values, pos) 
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
    description="scan search paths to find commands", history=false},
  [":empty"] = {path = function(args) ibs.shell_execute("/usr/bin/x-terminal-emulator") end, description = "open terminal", history=false},
  [":opendir"] = { path = function(args)
      if #args == 0 then
        ibs.message("Usage: :opendir (command name | path)")
        return 1
      end
      local path = ibs.to_directory_path(args[1])
      assert(ibs.open_dir(path))
      return 0
    end, history = false},
  google = { path = [[http://www.google.com/search?ie=utf8&q=${1}]], description=[[Searches words on Google]], history=false, icon = iceberg_config_dir ..[[/images/google256.png]]},
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

  -- linux default commands --
  locate = { path = function(args)
      local index = ibs.selected_index()
      if index > 0 then
        local value = commands.locate._list[index]
        assert(ibs.shell_execute(value.description))
      end
      commands.locate._list = {}
      return 0
    end, 
    completion = function(values, pos)
      commands.locate._list = {}
      if #values == 1 and values[1] == "" then return commands.locate._list end
      local ok, stdout, stderr =  ibs.command_output([[locate ]] .. ibs.quote_path(values[1]) .. [[ -l 20]])
      if ok then
        local lines = ibs.regex_split("\n", Regex.NONE, ibs.local2utf8(stdout))
        for i, line in ipairs(lines) do
          if line ~= "" then
            table.insert(commands.locate._list, {value=ibs.basename(line), description=line, icon=line, always_match=true})
          end
        end
      end
      return commands.locate._list
    end, decription = "search files", history = false},
  kill = { path = function(args)
      local buf = {"/bin/kill"}
      for i, v in ipairs(args) do
        local ok, m = ibs.regex_match(".*\\(pid:(\\d+)\\)", Regex.NONE, v)
        if ok then
          table.insert(buf, m:_1())
        else
          table.insert(buf, v)
        end
      end
      local ok, stdout, stderr = ibs.command_output(table.concat(buf, " "))
      if not ok then
        ibs.message("Failed to send given signal.")
        return 1
      end
      return 0
    end, 
    completion = function(values, pos)
      local candidates = {}
      if values[pos] == "-" then
        return {
          {value="-1", description="HUP"}, {value="-2", description="INT"},
          {value="-3", description="QUIT"}, {value="-9", description="KILL"},
          {value="-10", description="USR1"}, {value="-15", description="TERM"}}
      end
      local ok, stdout, stderr =  ibs.command_output([[ps ux]])
      if ok then
        local lines = ibs.regex_split("\n", Regex.NONE, ibs.local2utf8(stdout))
        for i, line in ipairs(lines) do
          if ibs.regex_match("[\\w\\-_]+\\s+\\d+.*", Regex.NONE, line) then
            local cols = ibs.regex_split("\\s+", Regex.NONE, line)
            local proc = table.concat({select(11, unpack(cols))}, " ")
            table.insert(candidates, proc .. "(pid:" .. cols[2] ..")")
          end
        end
      end
      return candidates
    end, decription = "send a signal to a process", history = false}
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

-- load configurations from ~/.iceberg/config.d/*.lua --
ibs.load_lua_files(ibs.join_path(iceberg_config_dir, "config.d"))
ibs.load_plugins()
