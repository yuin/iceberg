local ibs = require("icebergsupport")
local script_path = ibs.join_path(ibs.config_path, "luamodule")

require("LuaXml")
require("lfs")
local http = require("socket.http")
local rss_url = "http://weather.livedoor.com/forecast/rss/index.xml"
local cache_expires = 60*60 -- 1 hour
local xo = function(node, ...)
  local path = {...}
  local current = node
  for i, p in ipairs(path) do
    current = current:find(p)
  end
  return current
end
local xt = function(node, ...) 
  return xo(node, ...)[1]
end

system.completer.option_func["weather"] = function(values)
  local xml_file = ibs.join_path(script_path, "weather", "data.xml")
  local xml_text = ""
  if not ibs.file_exists(xml_file) or (os.time() - lfs.attributes(xml_file).modification) > cache_expires then
    xml_text = http.request(rss_url)
    if type(xml_text) ~= "string" then
      ibs.message("Failed to connect URL: " .. rss_url)
      return {}
    end
    local oio = io.open(xml_file, "w")
    oio:write(xml_text)
    oio:close()
  else
    local iio =  io.open(xml_file, "r")
    xml_text = iio:read("*a")
    iio:close()
  end
  local xml_object = xml.eval(xml_text)
  local candidates = {}

  for i, item in ipairs(xo(xml_object, "rss", "channel")) do
    if i > 13 and item[0] == "item" then
      local parts = ibs.regex_split("/", Regex.NONE, xt(item,"image","url"))
      local iconfile = parts[#parts]
      local iconpath = ibs.join_path(script_path, "weather", iconfile)
      if not ibs.file_exists(iconpath) then
        local data = http.request(xt(item,"image","url"))
        if type(data) == "string" then
          local oio = assert(io.open(iconpath, "wb"))
          oio:write(data)
          oio:close()
        else
          ibs.message("Failed to connect URL: " .. xt(item,"image","url"))
          return {}
        end
      end
      local title = ibs.regex_gsub("\\[ 今日の天気 \\](.*)", Regex.NONE, xt(item,"title"), function(re) return re:_1() end)
      local value = {value = title, description = xt(item,"description"), icon=iconpath}
      table.insert(candidates, value)
    end
  end
  return candidates
end

commands["weather"] = { 
  path = function(args) end, 
  description = "日本の主要都市の天気を表示します。",
  icon=ibs.join_path(script_path, "weather", "weather.png"), 
  history=false
}
