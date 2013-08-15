local t = {
-- Constants {{{
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
-- }}}

return t
