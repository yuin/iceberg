Configurations
=================================
Overview
--------------------
All configurations are written in `Lua <http://www.lua.org>`_ . A default configuration directory differs depending on your platform.

Windows:

- Same directory as the base directory of iceberg executable file

Linux:

iceberg searchs a configuration directory in the following order:

- ``${XDG_CONFIG_HOME}/iceberg`` if ``XDG_CONFIG_HOME`` envvar exists.
- ``${HOME}/.config/iceberg`` if ``${HOME}/.config`` is an existing directory path.
- ``${HOME}/.iceberg``

This directory shall be expressed as ``ICEBERG_CONFIG_HOME`` in this document.

A configuration file has several global functions and global variables.

Configuration files are also loaded from ``${ICEBERG_CONFIG_HOME}/config.d/*.lua`` .

``auto_merge`` can be used for writing configurations loaded from ``config.d``.

    .. code-block:: lua

        local ibs = require("icebergsupport")

        auto_merge = true -- configurations such as ``system`` and ``commands`` after this line will automatically be merged

        system = {
            enable_icons = false
        }

Note that if you do not set ``auto_merge`` to ``true``, all global variables will be overwritten.

Key names
--------------------
The following key names can be used in configurations. You can combine modifier keys and other keys by ``-`` (up to 3 keys) . The hot key configuration accepts OS native virtual keycodes.

Examples:

    .. code-block:: lua

        "alt-space"
        "shift-ctrl-a"
        "0x1d" -- Muhenkan on Windows(jp106 keyboard layout)
        "0x1c" -- Henkan on Windows(jp106 keyboard layout)

- Modifiers
    - ``shift``
    - ``caps_lock``
    - ``ctrl``
    - ``alt``
    - ``num_lock``
    - ``meta``
    - ``scroll_lock``

- Others
    - ``a`` - ``z``
    - ``0`` - ``9``
    - ``space``
    - ``backspace``
    - ``tab``
    - ``iso_key``
    - ``enter``
    - ``pause``
    - ``scroll_lock``
    - ``escape``
    - ``home``
    - ``left``
    - ``up``
    - ``right``
    - ``down``
    - ``page_up``
    - ``page_down``
    - ``end``
    - ``print``
    - ``insert``
    - ``menu``
    - ``help``
    - ``num_lock``
    - ``kp0`` : keys start with "kp" are on a numeric keypad
    - ``kp1``
    - ``kp2``
    - ``kp3``
    - ``kp4``
    - ``kp5``
    - ``kp6``
    - ``kp7``
    - ``kp8``
    - ``kp9``
    - ``kp_enter``
    - ``kp_last``
    - ``f1``
    - ``f2``
    - ``f3``
    - ``f4``
    - ``f5``
    - ``f6``
    - ``f7``
    - ``f8``
    - ``f9``
    - ``f10``
    - ``f11``
    - ``f12``
    - ``f_last``
    - ``shift_l``
    - ``shift_r``
    - ``control_l``
    - ``control_r``
    - ``caps_lock``
    - ``meta_l``
    - ``meta_r``
    - ``alt_l``
    - ``alt_r``
    - ``delete``
    - ``volume_down``
    - ``volume_mute``
    - ``volume_up``
    - ``media_play``
    - ``media_stop``
    - ``media_prev``
    - ``media_next``
    - ``home_page``
    - ``mail``
    - ``search``
    - ``back``
    - ``forward``
    - ``stop``
    - ``refresh``
    - ``sleep``
    - ``favorites``

System global variable
---------------------------
Examples and descriptions
~~~~~~~~~~~~~~~~~~~~~~~~~~~

    .. code-block:: lua

        system = {
          -- a default value of limiting the depth of the search path -- 
          default_search_path_depth = 2,

          -- should show icons on listbox? --
          enable_icons = true,

          -- an icon theme : meaningful only for Linux platforms --
          icon_theme = "nuoveXT.2.2",

          -- a maximum number of cached icon data --
          max_cached_icons = 9999,

          -- show completion candidates after N ms since the last key input --
          -- you can suppress unnecessary completions by setting this value on low-end machines --
          key_event_threshold = 0,

          -- a maximum number of commands to remember on the history file --
          max_histories = 500,

          -- a maximum number of candidates on the listbox -- 
          max_candidates = 15,

          -- a maximum number of clipboard histories : meaningful only for Windows platforms -- 
          max_clipboard_histories = 15,

          -- degree of influence of histories on completion candidates sorting(0.0~1.0) --
          history_factor = 0.8,

          -- an application to handle directories. ${1} will be replaced with a directory path --
          file_browser = [[explorer ${1}]],

          -- an application to handle CUI applications. ${1} will be replaced with a command text. This application must be executed as a login shell --
          terminal = [[lxterminal -l -e ${1}]],

          -- a port number to accept commands from external processes.(0: disabled) --
          server_port = 13505,

          -- should do autocompletion in a path completion --
          path_autocomplete = true,

          -- should do autocompletion in an option completion --
          option_autocomplete = true,

          -- do not use DirectWrite --
          disable_direct_write = false,

          -- DirectWrite parameters. an empty string means using default parameter --
          direct_write_params="gamma=1.8,enchanced_contrast=0.5,clear_type_level=0.5,pixel_geometry=0,rendering_mode=5",

          -- Keys --
          -- hot_key accepts OS dependent virtual key codes --
          --   Example: hot_key = "0x1d"  muhenkan(jp106 keyboard layout) --
          hot_key = "ctrl-space",
          escape_key = "escape",
          list_next_key = "ctrl-n",
          list_prev_key = "ctrl-p",
          toggle_mode_key = "ctrl-r",
          kill_word_key = "ctrl-w",
        
          -- Search paths --
          search_path = {
            {category="system", path = [[C:\Windows\System32]], depth = 1, pattern="^.*\\.(exe)$"}, 
            {category="programs", path = [[C:\Users\]] .. os.getenv("USERNAME") .. [[\AppData\Roaming\Microsoft\Windows\Start Menu\Programs]], depth = 10, pattern=[[^.*\.(exe|lnk)$]]},
            {category="programs", path = [[C:\ProgramData\Microsoft\Windows\Start Menu\Programs]], depth = 10, pattern=[[^.*\.(exe|lnk)$]]},
          },

          -- Completions -- 
          completer = {
            -- a command name completion: fuzzy match --
            command = ibs.COMP_ABBR,

            -- a path completion: prefix match -- 
            path    = ibs.COMP_BEGINSWITH,

            -- a history completion: partial match -- 
            history = ibs.COMP_PARTIAL,

            -- an option completion -- 
            option  = ibs.COMP_PARTIAL,
        
            -- completion functions --
            option_func = {
              [":scan_search_path"] = function(values, pos)
                local candidates = {"all"}
                local keys       = {all = true}
                for i, value in ipairs(system.search_path) do
                  if value.category ~= nil and keys[value.category] == nil then
                    table.insert(candidates, value.category)
                    keys[value.category] = true
                  end
                end
                return candidates
              end
            }
          }
        }

Search paths
~~~~~~~~~~~~~~~~~
iceberg can search under search paths and register found commands by single command( ``:scan_search_path`` ). A search path consists of

:category:
    You can pass a category to ``:scan_search_path`` to filter target search paths. If a category is not specified, ``default`` is used as a category.

:path:
    A base directory of this search path.

:depth:
    Limit the depth of the search path to this value. If ``depth`` is not specified, ``system.default_search_path_depth`` is used as ``depth``

:pattern:
    A regular expression to filter files by its name. Files that match this regular expression will be registerd with iceberg.

Completion functions
~~~~~~~~~~~~~~~~~~~~~
Commands can have a completion function.

    .. code-block:: lua
        
        function(values, pos)
          return {"a", "b", "c"}
        end

        -- もしくは

        function(values, pos)
          return { 
           {value="a", icon="path_to/icon.png", description="desc"}, 
           {value="b", icon="path_to/icon.jpg", description="desc"},
           {value="c", icon="path_to/icon.gif", description="desc"}
          }
        end

``values`` is a list of arguments for the command. If no argument exists, ``value`` will be a list that has single empty string entry. ``pos`` is an index of ``value`` where cursor exists. 
Completion functions have to return the list of strings or a table contains following keys:

:value:
    Completion value(required).
:icon:
    An icon file path for this command
:description:
    Description
:always_match:
    Disables candidate filtering by user text input. Typically this option is set to true in a command such as searching a term in Google.

Please refer to :lua:func:`icebergsupport.comp_state` for writing a complex completion function.


commands global variable
-------------------------
Examples
~~~~~~~~~~~~~~~~~~~~~

    .. code-block:: lua


        commands = { 
          -- A directory, will not be recorded in the history file --
          windir = {path = [[C:\Windows]], history = false},

          -- An executable file, runs with a iceberg current directory -- 
          np = {path = [[notepad.exe]], description="Notepad", workdir="."},

          -- A shell script file, executed in the terminal --
          np = {path = [[myscript.sh]], description="My script", terminal="yes"},

          -- A lua function, has a completion function -- 
          lua_sample = { 
            path = function(args) 
              local explorer = wins.foreground_explorer()
              if explorer then
                ibs.message(ibs.table_to_string(explorer))
              end
            end,
            completion = function(values, pos)
              return {"1","2","3"}
            end
            description="Sample Lua command"},


          -- A URL, has a png icon file -- 
          google = { path = [[http://www.google.com/search?ie=utf8&q=${1}]], description=[[Searches words on Google]], history=false,
               icon = script_path ..[[images\google256.png]]},

          -- A group of commands --
          group_sample = { path = ibs.group_command({"windir", {}}, {"np", {}}), description = "runs a group of commands"},
        
        }

Commands
~~~~~~~~~~~~~~~~~~~~~~~
Commands consist of

:name: A key of the ``commands`` table.
:path: 
    A file path or URL or lua function. A path can contain arguments like the following. ::

        path = [["C:\s p a c e\bin.exe" arg1 arg2]]

    Paths that contain spaces must be enclosed in ``"`` .

    Positional variables can be use in a path. If the inputbox has ``google iceberg`` and a path of the ``google`` command is defined as follows, ``${1}`` will be replaced with ``iceberg``  ::

        path = [[http://www.google.com/search?ie=utf8&q=${1}]]

    In a Lua function, the function receives a list of string. Lua functions should return 0 when function executed successfully, 1 otherwise.
:completion:
    A completion function that has same form as ``system.completer.option_func``. Completion functions can either be defined in a command definition or ``system.completer.option_func`` .  This value overrides ``system.completer.option_func`` if both are defined.
:description:
    A description for this command.
:icon:
    An icon file path. On Linux platforms, an icon name in an icon theme is valid. If an icon name is used, iceberg select the best size of an image automatically.
:terminal:
    If this value is set to ``"yes"``, the command will always be executed in the terminal. ``"no"`` means the command will be executed without the termina. If this value is set to ``"auto"`` , iceberg determines whether or not this command should be executed in the terminal.
:history:
    If this value is set to ``false``, Additional arguments will no be recorded in the history file.
:workdir:
    A working directory of this command.

    - strings
    - ``.`` : use iceberg current directory
    - Lua function: use a return value(string) of the function. You can make iceberg and your file browser cooperate by writing a function that returns a current directory of your file browser.

shortcuts global variables
----------------------------
Examples
~~~~~~~~~~~~~~~~~~~~~

    .. code-block:: lua

        shortcuts = {
          { key = "ctrl-d", name = ":opendir" },
          { key = "ctrl-l", name = ":cd" }
        }

Consider, when you have pressed ``ctrl-l`` and the inputbox values is ``c:\`` .
In this situation, the following command will be executed ::

    :cd c:\

In short, an inputbox value is passed as an argument to the command.

on_key_up event handler
--------------------------
This function will be executed when you release the key on the keyboard.

    .. code-block:: lua

        function on_key_up()
          local accept = 0
          return accept
        end

If you want to prevent default behavior, return 1 .

on_key_down event handler
---------------------------------
This function will be executed when you press the key on the keyboard.

    .. code-block:: lua

        function on_key_down()
          local accept = 0
          return accept
        end

If you want to prevent default behavior, return 1 .

on_enter event handler
--------------------------
This function will be executed when you press the Enter key on the keyboard.

    .. code-block:: lua

        function on_enter()
          local accept = 0
          return accept
        end

If you want to prevent default behavior, return 1 .

on_initialize event handler
--------------------------------
This function will be executed just after iceberg is launched.

    .. code-block:: lua

        function on_enter()
          local error = 0
          return error
        end

If this function returns 1, the launching process will be stopped.
