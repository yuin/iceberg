local script_path = _iceberg_config_dir() .. [[\luamodule]]
local ibs = require("icebergsupport")
local lib = {}


local ok, m = ibs.regex_match([[.*wow64"]], Regex.NONE, ibs.runtime_platform())
lib.is_wow64 = ok
if lib.is_wow64 then
  lib.cmd_path = [[C:\Windows\Sysnative\cmd.exe]]
else
  lib.cmd_path = [[C:\Windows\System32\cmd.exe]]
end

lib.foreground_explorer = function() 
  local script = [[
    var shell = new ActiveXObject("Shell.Application");
    function enumToArray(obj){
        var e = new Enumerator(obj);
        for (var ret=[]; !e.atEnd(); e.moveNext()) ret.push(e.item());
        return ret;
    }
    var windows = enumToArray(shell.Windows());
    for(var i=0,l=windows.length; i<l; i++){
      try {
        windows[i].hwnd;
      }catch(e){
        continue;
      }
      if(windows[i].LocationURL.match(/^file:.*/)) {
        WScript.Echo(windows[i].hwnd);
        WScript.Echo(decodeURIComponent(windows[i].LocationURL.replace("file:///", "")).replace(/\//g, "\\"));
        var items = enumToArray(windows[i].Document.SelectedItems());
        for(var j=0,m=items.length; j < m; j++){
          WScript.Echo(items[j].name);
        }
        WScript.Echo("/////");
      }
    }
    WScript.Echo("SUCCESS");
  ]]
  local tmppath = script_path .. "\\__foreground_explorer__.js"
  local test = io.open(tmppath)
  if not test then
    local file = io.open(tmppath, "w")
    file:write(script)
    file:close()
  end
  local ok,out,err = ibs.command_output("cscript.exe " .. tmppath)
  local rawtext = ibs.local2utf8(ibs.crlf2lf(out))
  local lines = ibs.regex_split("\n", Regex.NONE, rawtext)
  if lines[#lines-1] ~= "SUCCESS" then
    return nil
  else
    local result = {}
    local current = {selected={}}
    for i=4, #lines-2 do
      local line = lines[i]
      if line == "/////" then
        result[current.hwnd] = current
        current = {selected={}}
      elseif not current.hwnd then
        current.hwnd = tonumber(line)
      elseif not current.path then
        current.path = line
      else
        table.insert(current.selected, line)
      end
    end
    local window_ids = ibs.list_all_windows()
    for i=1, #window_ids do
      if result[window_ids[i]] ~= nil then
        return result[window_ids[i]]
      end
    end
  end
  return nil
end

lib.foreground_explorer_path = function()
  local fe = lib.foreground_explorer()
  if not fe then
    return ""
  else
    return fe.path
  end
end

lib.list_files = function(dir, pattern)
  local ok, out, err = ibs.command_output([[cmd.exe /c dir ]] .. ibs.quote_path(dir) .. [[ /b]])
  local files = ibs.regex_split("\n", Regex.NONE, ibs.local2utf8(ibs.crlf2lf(out)))
  local result = {}
  local reg = Regex.new(pattern, Regex.I)
  for i = 1, #files-1 do
    local ok, m = reg:match(files[i])
    if ok then
      table.insert(result, files[i])
    end
  end
  return result
end

return lib
