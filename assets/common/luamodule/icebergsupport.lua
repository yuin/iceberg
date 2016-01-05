local t = {
-- Constants {{{
  CONFIG_DIR = _iceberg_config_dir(),
  EVENT_STATE_SHIFT = 0x00010000,
  EVENT_STATE_CAPS_LOCK = 0x00020000,
  EVENT_STATE_CTRL = 0x00040000,
  EVENT_STATE_ALT = 0x00080000,
  EVENT_STATE_NUM_LOCK = 0x00100000,
  EVENT_STATE_META = 0x00400000,
  EVENT_STATE_SCROLL_LOCK = 0x00800000,
  EVENT_STATE_BUTTON1 = 0x01000000,
  EVENT_STATE_BUTTON2 = 0x02000000,
  EVENT_STATE_BUTTON3 = 0x04000000,
  EVENT_STATE_BUTTONS = 0x7f000000,

  BOXTYPE_NO_BOX = 0,
  BOXTYPE_FLAT_BOX = 1,
  BOXTYPE_UP_BOX = 2,
  BOXTYPE_DOWN_BOX = 3,
  BOXTYPE_UP_FRAME = 4,
  BOXTYPE_DOWN_FRAME = 5,
  BOXTYPE_THIN_UP_BOX = 6,
  BOXTYPE_THIN_DOWN_BOX = 7,
  BOXTYPE_THIN_UP_FRAME = 8,
  BOXTYPE_THIN_DOWN_FRAME = 9,
  BOXTYPE_ENGRAVED_BOX = 10,
  BOXTYPE_EMBOSSED_BOX = 11,
  BOXTYPE_ENGRAVED_FRAME = 12,
  BOXTYPE_EMBOSSED_FRAME = 13,
  BOXTYPE_BORDER_BOX = 14,
  BOXTYPE_BORDER_FRAME = 16,
  BOXTYPE_ROUND_UP_BOX = 22,
  BOXTYPE_ROUND_DOWN_BOX = 23,
  BOXTYPE_SHADOW_BOX = 15,
  BOXTYPE_SHADOW_FRAME = 17,
  BOXTYPE_ROUNDED_BOX = 18,
  BOXTYPE_ROUNDED_FRAME = 20,
  BOXTYPE_RFLAT_BOX = 21,
  BOXTYPE_RSHADOW_BOX = 19,
  BOXTYPE_DIAMOND_UP_BOX = 24,
  BOXTYPE_DIAMOND_DOWN_BOX = 25,
  BOXTYPE_OVAL_BOX = 26,
  BOXTYPE_OSHADOW_BOX = 27,
  BOXTYPE_OVAL_FRAME = 28,
  BOXTYPE_OFLAT_BOX = 29,
  BOXTYPE_PLASTIC_UP_BOX = 30,
  BOXTYPE_PLASTIC_DOWN_BOX = 31,
  BOXTYPE_PLASTIC_UP_FRAME = 32,
  BOXTYPE_PLASTIC_DOWN_FRAME = 33,
  BOXTYPE_PLASTIC_THIN_UP_BOX = 34,
  BOXTYPE_PLASTIC_THIN_DOWN_BOX = 35,
  BOXTYPE_PLASTIC_ROUND_UP_BOX = 36,
  BOXTYPE_PLASTIC_ROUND_DOWN_BOX = 37,

  COMP_BEGINSWITH = 1,
  COMP_PARTIAL  = 2,
  COMP_ABBR  = 3,
-- }}}
}
if type(_iceberg_module) == "function" then
  for k,v in pairs(_iceberg_module()) do t[k] = v end
end

-- Functions {{{
t.is_array = function(tbl) -- {{{
  local keys = 0
  for _, _ in pairs(tbl) do
      keys = keys+1
  end
  local indices = 0
  for _, _ in ipairs(tbl) do
      indices = indices+1
  end
  return keys == indices
end -- }}}

t.is_modifier_pressed = function(keycode) -- {{{
  return t.band(t.event_state(), keycode) ~= 0
end -- }}}

t.bind_key = function(key, func) -- {{{
  if t.matches_key(key) then
    func()
  end
end -- }}}

t.group_command = function(...) -- {{{
  local args = { ... }
  return function(_)
    for k,v in pairs(args) do
      local ok, message = t.command_execute(v[1], v[2])
      if not ok then
        t.message(message)
      end
    end
    t.set_input_text("")
    t.hide_application()
  end
end -- }}}

t.process_shortcut_keys = function() -- {{{
  local accept = 0
  for i,v in ipairs(shortcuts) do
    t.bind_key(v.key, function()
      t.do_autocomplete()
      t.default_after_command_action(t.command_execute(v.name, {t.get_input_text()}))
      accept = 1
    end)
  end
  return accept
end -- }}}

t.execute = function(cmd) -- {{{
  return os.execute(t.utf82local(cmd))
end -- }}}

t.regex_match = function(pattern, flags, string, ...)-- {{{
  local re = Regex.new(pattern, flags)
  return re:match(string, ...), re
end -- }}}

t.regex_search = function(pattern, flags, string, ...)-- {{{
  local re = Regex.new(pattern, flags)
  return re:search(string, ...), re
end -- }}}

t.regex_split = function(pattern, flags, string, ...)-- {{{
  local re = Regex.new(pattern, flags)
  return re:split(string, ...)
end -- }}}

t.regex_gsub = function(pattern, flags, string, repl) -- {{{
  local re = Regex.new(pattern, flags)
  return re:gsub(string, repl)
end -- }}}

t.grep = function(text, pattern, ...) -- {{{
  local opt = {...}
  local flags = Regex.NONE
  if #opt > 0 then
    flags = opt[1]
  end
  local buf = {}
  local reg = Regex.new(pattern, flags)
  local lines = t.regex_split("\r?\n", Regex.NONE, text)
  for i, line in ipairs(lines) do
    local ok, m = reg:search(line)
    if ok then
      table.insert(buf, line)
    end
  end
  return table.concat(buf, "\n")
end -- }}}

t.crlf2lf = function(string) -- {{{
  return t.regex_gsub("\r\n", Regex.NONE, string, "\n")
end -- }}}

t.quote_path = function(string) -- {{{
  if t.regex_search("\\s", Regex.NONE, string) then
    return "\"" .. string .. "\""
  end
  return string
end -- }}}

t.unquote_path = function(string) -- {{{
  local ok, re = t.regex_match([[^"([^\"]*)"$]], Regex.NONE, string)
  if ok then
    return string:sub(2,-2)
  else
    return string
  end
end -- }}}

t.to_directory_path = function(arg) -- {{{
  local ok = false
  local path = arg
  if not t.path_exists(path) then
    ok, path = assert(t.to_path(arg))
  end
  if not t.directory_exists(path) then 
    path = t.dirname(path)
  end
  return path
end -- }}}

t.dump_lua_object = function(obj, indent, isarrayval) -- {{{
  local indent = indent or 0
  local isarrayval = isarrayval or false
  if type(obj) == "function" then
    return "\"<function object>\";\n"
  elseif type(obj) == "thread" then
    return "\"<thread object>\";\n"
  elseif type(obj) == "userdata" then
    return "\"<userdata object>\";\n"
  elseif type(obj) == "lightuserdata" then
    return "\"<lightuserdata object>\";\n"
  elseif type(obj) == "string" then
    return "[[" .. tostring(obj) .. "]];\n"
  elseif type(obj) == "table" then
    local sb = {}
    if isarrayval then
      table.insert(sb, string.rep("  ", indent))
    end
    table.insert(sb, "{\n")
    if t.is_array(obj) then
      for key, value in pairs(obj) do
        table.insert(sb, t.dump_lua_object(value, indent+2, true))
      end
    else
      for key, value in pairs(obj) do
        table.insert(sb, string.rep("  ", indent+2))
        if type(key) == "string" then
          table.insert(sb, string.format("[\"%s\"] = ", tostring(key)))
        else
          table.insert(sb, string.format("[%s] = ", tostring(key)))
        end
        table.insert(sb, t.dump_lua_object(value, indent+2))
      end
    end
    table.insert(sb, string.rep ("  ", indent))
    table.insert(sb, "};\n")
    return table.concat(sb)
  else
    return tostring(obj) .. ";\n"
  end
end -- }}}

t.load_lua_object = function(str) -- {{{
  local script = "ret = " .. str
  local func, message = loadstring(script)
  if not func then
    return false, message
  else
    func()
    return true, ret
  end
end -- }}}

t.load_lua_files = function(directory) -- {{{
  local _, files = assert(t.list_dir(directory))
  table.sort(files)
  for i, file in ipairs(files) do
    if t.regex_match("^[^_].*\\.lua$", Regex.NONE, file) then
      dofile(t.join_path(directory, file))
    end
  end
end -- }}}

t.load_plugins = function() -- {{{
  local directory = t.join_path(t.CONFIG_DIR, "plugins")
  local _, dirs = assert(t.list_dir(directory))
  table.sort(dirs)
  for i, dir in ipairs(dirs) do
    local dirpath = t.join_path(directory, dir)
    if t.directory_exists(dirpath) then
      local mainscript = t.join_path(dirpath, "main.lua")
      if t.file_exists(mainscript) then
        dofile(mainscript)
      end
    end
  end
end -- }}}

t.merge_table = function(dest, ...) -- {{{
  local srcs = {...}
  for i, src in ipairs(srcs) do
    if dest[1] ~= nil then -- array
      table.insert(dest, src)
    else -- map
      for k, v in pairs(src) do
        if type(dest[k]) == "table" and type(v) == "table" then
          t.merge_table(dest[k], v)
        else
          dest[k] = v
        end
      end
    end
  end
end -- }}}

t.table_find = function(tbl, value) -- {{{
   for i, v in ipairs(tbl) do
     if v == value then return i end
   end
   return 0
end -- }}}

t.getopts = function(arguments, options) -- {{{
  local opts, args, i = {}, {}, 1
  local function find_opt(v) return t.table_find(options, v) end
  while i <= #arguments do
    local argument = arguments[i]
    local name = string.gsub(argument, [[^-+(.*)$]], "%1")
    if find_opt(argument) ~= 0 then
      opts[name] = true
    elseif t.table_find(options, argument..":") ~= 0 then
      if find_opt(opts[name]) == 0 and find_opt(tostring(opts[name])..":") == 0 then
        opts[name] = arguments[i+1]
        i = i + 1
      else
        opts[name] = nil
      end
    else
      table.insert(args, argument)
    end
    i = i + 1
  end
  return opts, args
end -- }}}

t.comp_state = function(values, pos, ...) -- {{{
  local cur  = values[pos]
  local prev = values[pos-1] or ""
  local opts = {...}
  local is_prev_opt = t.regex_match([[\-[\-]?[a-zA-Z0-9\-_]*]], Regex.NONE, prev)
  local has_exclude = function(exclude)
    for i, ex in ipairs(exclude or {}) do
      if t.table_find(values, ex) ~= 0 then
        return true
      end
    end
    return false
  end

  if t.regex_match([[\-[\-]?[a-zA-Z0-9\-_]*]], Regex.NONE, prev) then
    for i, opt in ipairs(opts) do
      if opt.opt == prev and not has_exclude(opt.exclude) then
        if opt.state then
          return opt.state, {}
        end
      end
    end
  end

  local candidates = {}
  local seen = {}
  for i, opt in ipairs(opts) do
    if not has_exclude(opt.exclude) and not seen[opt.opt] and t.table_find(values, opt.opt) == 0 then
      table.insert(candidates, {value=opt.opt, description=opt.description})
      seen[opt.opt] = true
    end
  end
  if t.regex_match([[\-[\-]?[a-zA-Z0-9\-_]*]], Regex.NONE, cur) then
    return "opt", candidates
  end
  return "", candidates
end -- }}}

-- }}}

return t
