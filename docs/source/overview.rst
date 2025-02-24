Overview
=========================
iceberg?
-------------------------
iceberg is a simple and extensible keystroke application launcher.

- Prefix match, Partial match, Fuzzy match
- Migemo search(Japanese text search by English alphabets)
- History search
- Execute lua functions
- Lua APIs

.. image:: images/screenshot_01.jpg

.. image:: images/iceberg_demo_01.gif

.. image:: images/iceberg_demo_02.gif

How to install
-------------------------
Windows
~~~~~~~~~~~~~~~
Download a zip file from `Release <https://github.com/yuin/iceberg/releases>`_ and unzip it where you want. iceberg does not use the registry.

Linux(Ubuntu 22.04)
~~~~~~~~~~~~~~~~~~~~~~~~~
iceberg requires the following packages. You can install these packages with ``apt install`` .

- g++
- gdb
- autoconf
- libpng12-0-dev
- libpng++-dev
- libjpeg8-dev
- libjpeg-turbo8-dev
- libftgl2
- libfontconfig
- libx11-dev
- libglu1-mesa-dev
- libasound2-dev
- libxft-dev
- libonig-dev
- libmigemo-dev
- gksu

Next, download a zip file from `Release <https://github.com/yuin/iceberg/releases>`_ and unzip it .

And then run the following commands :

    .. code-block:: bash
    
        % ./tools/install_requires.sh
        % make
        % sudo make install

On Linux, only UTF-8 is supported for system encodings. Linux version has some limitations compared to Windows version.

- `hot_key` may not work properly. You can activate iceberg with shortcut keys defined in your desktop environment.
  - e.g. Assign ``ctrl-space`` to ``iceberg -m "activate"`` in System Settings -> Keyboard -> Shortcuts(Ubunutu Desktop)
- A tray icon may not be shown

Linux(others)
~~~~~~~~~~~~~~~
Not verified though, It may be possible to compile with libraries that are equivalent to avobe packages. g++ must be newer than 4.9 .

How to use
-------------------------
In this example we will use the default configurations.

Add commands
~~~~~~~~~~~~~~~~~~~~~~~~~
First, you need to add applications. Input ``:scan_search_path all`` and hit Enter. Now iceberg scans your applications and add these applications to iceberg.

A default configuration directory differs depending on your platform.

Windows:

- Same directory as the base directory of iceberg executable file

Linux:

iceberg searchs the default configuration directory in the following order:

- ``${XDG_CONFIG_HOME}/iceberg`` if ``XDG_CONFIG_HOME`` environment variable exists.
- ``${HOME}/.config/iceberg`` if ``${HOME}/.config`` is an existing directory path.
- ``${HOME}/.iceberg``

This directory shall be expressed as ``ICEBERG_CONFIG_HOME`` in this document.

There are many way to add your applications.

1. Create shortcuts( ``*.link`` On Windows or ``.desktop`` On Linux) under ``${ICEBERG_CONFIG_HOME}/shortcuts}`` .
2. Execute ``:scan_search_path shortcuts`` in iceberg.

In addition, iceberg has the following method to add applications.

- Search paths
    1. Define search paths in configuration files.
    2. Execute ``:scan_search_path CATEGORY``. ``CATEGORY`` must be a category that was defined in 1 or ``all`` .
        - Files that match search path patterns will be added to iceberg.
- Write commands in configuration files: Please refer to :doc:`config` for further information.

Built-in internal commands
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iceberg has several commands called built-in internal commands. All built-in internal command names start with ``:`` .

- `:version` : Shows the version of iceberg.
- `:exit` : Shut iceberg down.
- `:reboot` : Reboot iceberg.
- `:pwd` : Shows the current directory of iceberg.
- `:cd`  : Changes the current directory of iceberg.
- `:scan_search_path`  : Scans search paths and add files to iceberg.
- `:opendir` : Opens a first argument as a directory.
- `:empty` : This command will be executed when the inputbox is empty. On Windows, iceberg opens a terminal with the foreground explorer's path by default. On other platforms, iceberg opens a terminal with the current directory of iceberg.

And the following commands are added by default.

- `google` : Searchs a first argument by Google.
- `cal` : A calculator that can be used such as ``cal 10+2``

The following commands are added by default only on Windows.

- `control_panel` : Shows the control panel.
- `windows_service` : Shows the windows services.
- `my_computer` : Shows the 'My Computer' .
- `network_computer` : Shows the network computers.
- `mkdir` : Creates a new directory named a first argument under the foreground explorer's path.
- `alttab` : Equivalent to ``alt+tab`` task switcher. Input ``alttab`` and hit space.
- `clipboard` : Shows a list of the histories. The selected history will be copied into the clipboard when this command is executed.

The following commands are added by default only on Linux.

- `locate` : Equivalent to the ``locate`` command.
- `kill` : Equivalent to the ``kill`` command, processes can be completed by its name.

Current directory
~~~~~~~~~~~~~~~~~~~~~~~~~~
iceberg has its own current directory. This directory can be shown ``pwd`` command and changed by ``:cd`` command.

Execute commands
~~~~~~~~~~~~~~~~~~~~~~~~~~
Once iceberg has been started, you hides it by hitting ``escape`` key. And you bring it forward again by holding the alt key and tapping the space key.

You can then type some keys for searching the commands and select found commands by ``ctrl-p`` and ``ctrl-n``. Once you have selected the command, hit enter to execute the commands. 

If you add a prefix ``!`` such as ``!notepad`` at this time, the commands will be executed under the current directory of iceberg rather than the current directory that is associated with the command itself. You can input a text that include spaces by enclosing the text within ``"`` such as ``"aaaa bbbb"`` .

iceberg also can browse file systems. The file browser mode will start by typing such as ``C:\`` and ``./`` . On Windows, iceberg shows a list of drives by typing ``/`` or ``\`` .

.. note:: 
    On Windows, you can use both ``/`` and ``\`` as a separator of path components. But in some situations, you can use only ``/`` . If an inputbox value is ``"C:\Document and Settings\name"``, you can not type ``"C:\Document and Settings\name\"`` . ``\`` after ``name`` is considered as an escape character for the tail ``"`` .

Shortcuts
~~~~~~~~~~~~~~~~
You can execute commands with an inputbox value by hitting some special key combinations. By default, ``ctrl-d`` opens a directory that is associated with a selected command. In this case, an inputbox value was passed as an argument to ``:open`` command.

Switch to the history mode
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
You can switch mode to the history mode by ``ctrl-r`` . In the history mode, commands will be completed including arguments.

Migemo search
~~~~~~~~~~~~~~~~~~~~~~~~~~~~
iceberg is integrated with the Migemo. To enable the migemo capability, put ``migemo.dll`` or ``libmigemo.so`` on your library path and migemo dictionaries on ``${ICEBERG_CONFIG_HOME}/dict`` . This functionality was verified with ``cmigemo-1.3c`` . On Windows, files should be like the following :

::

    iceberg.exe
    migemo.dll
    dict/ 
       han2zen.dat
       hira2kata.dat
       migemo-dict
       roma2hira.dat
       zen2han.dat

Migemo dictionaries should be encoded in UTF-8.

Send messages from external processes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
You can interact with an already existing iceberg instance by executing iceberg like the following :

::

    iceberg.exe -m "exec COMMAND_TO_EXECUTE"
    iceberg.exe -m "set TEXT_TO_SET_INTO_INPUTBOX"
    iceberg.exe -m "activate"

This functionality will be enabled if ``system.server_port`` is not set to ``0`` .

Further information
------------------------

Please refer to :doc:`config` , :doc:`api` , :doc:`plugin` and :doc:`tips` for further information.
