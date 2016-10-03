Plugins
=================================
Overview
--------------------
iceberg has a plugin system. This document describes how to use and develop plugins.

How to use
----------------------
Just put a plugin directory under ``${ICEBERG_CONFIG_HOME}/plugins`` .

You can disable a certain plugin by renaming its directory name with a new name starting with ``_`` .

How to develop
---------------------
Structures
~~~~~~~~~~~~~~~~~~~~~
A plugin is a simple directory that has ``main.lua`` . Typically, plugins have the following structure :
    
    .. code-block:: lua

        plugin-name          - plugin directory
          |
          +---- main.lua     - main script of the plugin
          |
          +---- icon.svg     - icon file

main.lua template
~~~~~~~~~~~~~~~~~~~~~

    .. code-block:: lua

      01  local ibs = require("icebergsupport")
      02  local script_path = ibs.dirname(debug.getinfo(1).source:sub(2,-1))
      03  
      04  local config = {
      05    name = "myplugin",
      06    conf1 = "aaa"
      07  }
      08  ibs.merge_table(config, plugin_myplugin or {})
      09
      10  commands[config.name] = { 
      11    path = function(args) 
      12    end, 
      13    completion = function(values, pos)
      14    end,
      15    description = "My plugin",
      16    icon = ibs.join_path(script_path, "icon.svg"),
      17    history=false
      18  }

- Line 04 : Holds configurations for this plugin as a local variable.
- Line 08 : Users can overwrite the configurations with a global variable named ``plugin_PLUGINNAME`` .
- Line 09 : Adds a command. Refer to :doc:`config` for further information.

Available plugins
---------------------

- `iceberg-ip <https://github.com/yuin/iceberg-ip>`_ : Shows your machine IP addresses
- `iceberg-worldtime <https://github.com/yuin/iceberg-worldtime>`_ : Shows the current time at the cities in the world.
