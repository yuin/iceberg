local dot_iceberg = _iceberg_config_dir()
package.path = dot_iceberg .. [[/luamodule/?.lua;]] .. package.path
package.cpath = dot_iceberg ..[[/luamodule/?.so;]] .. package.path
local ibs = require("icebergsupport")

-- configurations --
system = {
  default_search_path_depth = 2,
  enable_icons = true,
  max_cached_icons = 999999,
  key_event_threshold = 500,
  max_histories = 500,
  max_candidates = 15,
  max_clipboard_histories = 15,
  history_factor = 0.8,
  file_browser = [[/usr/bin/pcmanfm ${1}]],
  server_port = 40000,
  path_autocomplete = true,
  option_autocomplete = true,

  hot_key = "f12",
  escape_key = "escape",
  list_next_key = "ctrl-n",
  list_prev_key = "ctrl-p",
  toggle_mode_key = "ctrl-r",
  kill_word_key = "ctrl-w",

  search_path = {
    {category="shortcuts", path = dot_iceberg .. [[\shortcuts]], depth = 5, pattern=[[^.*\.(desktop)$]]},
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
  window_width = 300,
  window_height = 40,
  window_padx  = 7,
  window_pady  = 7,
  window_bg_color = {238, 238, 238},
  window_alpha = 220,
  taskbar_height = 40,

  input_boxtype = ibs.BOXTYPE_FLAT_BOX,
  input_font  = "Sans",
  input_font_size  = 14,
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
  gvim = {path = "/usr/share/applications/gvim.desktop", description="gvim"},

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
    completion = function(values) 
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
  google = { path = [[http://www.google.com/search?ie=utf8&q=${1}]], description=[[Searches words on Google]], history=false, icon = dot_iceberg ..[[/images/google256.png]]},
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
  clipboard = {
    path = function(args)
      if #args == 0 then return end
      local ok, r = ibs.regex_match([[\(([0-9]+)\).*]], Regex.NONE, args[1])
      if ok then
        ibs.set_clipboard(ibs.get_clipboard_histories()[tonumber(r:_1())])
      end
      return 0
    end,
    completion = function(values)
        local candidates = {}
        for i, text in ipairs(ibs.get_clipboard_histories()) do
          local value = ibs.regex_gsub("\n", Regex.NONE, text, " ")
          value = ibs.regex_gsub("\t", Regex.NONE, value, "    ")
          local ok, r = ibs.regex_match("(.{50})(.*)", Regex.NONE, value)
          if ok then
            value = r:_1() .. " ... "
          end
          value = "(" .. tostring(i) .. ") : " .. value
          table.insert(candidates, value)
        end
        return candidates
    end,
    description = "clipboard history",
    history = false
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
