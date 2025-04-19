Tips
=================================
Overview
--------------------
Here are a few helpful tips for using iceberg.

Copy current date or time into the clipboard
-------------------------------------------------

    .. code-block:: lua

        date_YYYYMMDD = {path = function(args) ibs.set_clipboard(os.date("%Y%m%d")) end, history = false},
        date_YYYYMMDDHHMMSS = {path = function(args) ibs.set_clipboard(os.date("%Y%m%d%H%M%S")) end, history = false},

Open a file that selected in an explorer with your editor
-----------------------------------------------------------
Select a file in an explorer and bring iceberg foward, then hit ``ctrl-m`` .

    .. code-block:: lua

        function on_key_down()
          local accept = 0
        
          ibs.bind_key("ctrl-m", function()
            local ex = wins.foreground_explorer()
            ibs.default_after_command_action(ibs.command_execute("vim", {ex.path .. [[\]] .. ex.selected[1]}))
            accept = 1
          end)
        
          if accept == 0 then
            accept = ibs.process_shortcut_keys() 
          end
          return accept
        end

Show files under a directory that is already registered as a command
---------------------------------------------------------------------
Consider a case that ``C:\app`` is registered as a command named ``app`` . When you type ``app`` and hit ``ctrl-l`` in this case, the current directory of iceberg will be changed to ``C:\app`` . Files under ``C:\app`` can be shown by typing ``./`` .


Instant calculator
--------------------------------------------------------
The following ``on_enter`` function allow you to perform numerical calculation quickly. Just type such as ``10 + 20`` and hit Enter.


    .. code-block:: lua

        function on_enter()
          local accept = 0
        
          local text = ibs.get_input_text()
          local ok ,r = ibs.regex_match("\\d+\\s+.*", Regex.NONE, text)
          if ok then
            ibs.default_after_command_action(ibs.command_execute("cal", {text}))
            accept = 1
          end
        
          return accept
        end

Execute :scan_search_path command automatically when you start iceberg
------------------------------------------------------------------------

    .. code-block:: lua
        
        function on_initialize()
          local error = 0
          local autoscan_file = ibs.join_path(ibs.CONFIG_DIR, ".autoscan")
          if ibs.file_exists(autoscan_file) then
            ibs.command_execute(":scan_search_path", {"all"})
            os.remove(autoscan_file)
          else
            local fp = io.open(autoscan_file, "w")
            fp:write("1")
            fp:close()
          end
        
          return error
        end

Interact with foobar2000
--------------------------------------------------------


    .. code-block:: lua
        
        foobar2000 = { 
          path=[[path of a foobar2000 executable]],
          completion = function(values, pos)
            return {
              {value="/playpause", description = "Play/Pause"},
              {value="/stop", description = "Stop"},
              {value="/pause", description = "Pause"},
              {value="/play", description = "Play"},
              {value="/prev", description = "Prev"},
              {value="/next", description = "Next"},
              {value="/rand", description = "Random"},
              {value="/exit", description = "Exit"},
              {value="/show", description = "Show"},
              {value="/hide", description = "Hide"}
            }
          end,
          history = false
        }
