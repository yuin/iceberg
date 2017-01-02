API
=================================
Overview
---------------------------------
iceberg uses `Lua <http://www.lua.org>`_ as a configuration and an extension language. Users can extend the functionality by using lua scripts. ``icebergsupport`` package has many APIs for extending iceberg.

Charcter encodings
---------------------------------
iceberg uses UTF-8 as an internal encoding, so most APIs accept and return UTF-8 encoded strings. However, :lua:func:`icebergsupport.command_output` returns a string output by the command without any character encoding conversion. 

`Lua <http://www.lua.org>`_ does not have multibyte aware string APIs. If you want to correctly iterate over actual characters in a UTF-8 encoded string, you have to use regular expression APIs in the `icebergsupport` package.

Constants
---------------------------------

.. lua:attribute:: icebergsupport.EVENT_STATE_*

  Constants used for :lua:func:`icebergsupport.matches_key` etc. Please refer to :lua:func:`icebergsupport.matches_key` for further information.

.. lua:attribute:: icebergsupport.COMP_*

    Constants mean completion methods.

    - ``COMP_BEGINSWITH`` : prefix match
    - ``COMP_PARTIAL`` : partial match
    - ``COMP_ABBR`` : fuzzy match

Functions
---------------------------------
Paths
~~~~~~~~~~~~~~

.. lua:function:: icebergsupport.dirname(path)

    Returns the directory name of the pathname ``path`` .

    :param string path: the path
    :returns: string

.. lua:function:: icebergsupport.basename(path)

    Returns the base name of pathname ``path`` .

    :param string path: the path
    :returns: string

.. lua:function:: icebergsupport.directory_exists(path)

    Returns ``true`` if ``path`` refers to an existing directory.

    :param string path: the path
    :returns: bool

.. lua:function:: icebergsupport.file_exists(path)

    Returns ``true`` if ``path`` refers to an existing file.

    :param string path: the path
    :returns: bool

.. lua:function:: icebergsupport.path_exists(path)

    Returns ``true`` if ``path`` refers to an existing file or directory.

    :param string path: the path
    :returns: bool

.. lua:function:: icebergsupport.join_path(pathparts[, pathparts, pathparts ...])

    Joins one or more path components.

    :param string pathparts: a path component 
    :returns: string

.. lua:function:: icebergsupport.list_dir(path)

    Returns a list containing the names of the entries in the directory given by ``path`` .

    :param string path: the directory path
    :returns: [bool:true if no errors, false otherwise, table or string:a table contains names if no errors, an error message otherwise]

.. lua:function:: icebergsupport.quote_path(path)

    Searches a ``path`` for spaces. If spaces are found, the entire path is enclosed in ``"`` .

    :param string path: the path
    :returns: string: a quoted path if spaces found, given path otherwise

.. lua:function:: icebergsupport.unquote_path(path)

    Removes ``"`` at beginning or the end of ``path`` if ``path`` is enclosed in ``"``

    :param string path: the path
    :returns: string: an unquote path if enclosed in ``"``, given path otherwise


Bitwise operations
~~~~~~~~~~~~~~~~~~~

.. lua:function:: icebergsupport.band(number[, number, number ...])

    Returns the bitwise and of ``numbers`` . ``numbers`` are interpreted as a ``lua_Integer``.

    :param [number] number:
    :returns: number

.. lua:function:: icebergsupport.bor(number[, number, number ...])

    Returns the bitwise and of ``numbers`` . ``numbers`` are interpreted as a ``lua_Integer``.

    :param [number] number:
    :returns: number

.. lua:function:: icebergsupport.bxor(number[, number, number ...])

    Returns the bitwise exclusive or of ``numbers`` . ``numbers`` are interpreted as a ``lua_Integer``.

    :param [number] number:
    :returns: number

.. lua:function:: icebergsupport.brshift(number, disp)

    Returns the number ``number`` shifted ``disp`` bits to the right. ``number`` and ``disp`` are interpreted as a ``lua_Integer``.

    :param number number:
    :param number disp:
    :returns: number

.. lua:function:: icebergsupport.blshift(number, disp)

    Returns the number ``number`` shifted ``disp`` bits to the left. ``number`` and ``disp`` are interpreted as a ``lua_Integer``.

    :param number number:
    :param number disp:
    :returns: number

System information
~~~~~~~~~~~~~~~~~~~~

.. lua:function:: icebergsupport.build_platform()

    Returns the platform where iceberg was compiled.

    :returns: string: a string such as ``win_64``

.. lua:function:: icebergsupport.runtime_platform()

    Returns the platform where iceberg are currently running.

    :returns: string: a string such as ``6.1.7601 x64``

Sub processes
~~~~~~~~~~~~~~

.. lua:function:: icebergsupport.shell_execute(path [, args, workdir, sudo])

    Runs the sub process. If path is not a executable file, path will be opened by an application associated with its extension. 

    :param string path: the file path
    :param [string] args: arguments for the command
    :param string workdir: a working directory. This defaults to the current directory.
    :param bool sudo : If sudo is true, the sub process will be run as an administrator user.
    :returns: [bool:true if no errors, false otherwise, string:an error message]

.. lua:function:: icebergsupport.command_output(command)

    Runs the sub process and returns contents of stdout and stderr .

    :param string command:
    :returns: [bool:true if no errors, false otherwise, string:stdout, string:stderr]

Charcter sets
~~~~~~~~~~~~~~

.. lua:function:: icebergsupport.utf82local(text)

    Converts ``text`` from utf-8 to the local encoding.

    :param string text:
    :returns: string

.. lua:function:: icebergsupport.local2utf8(text)

    Converts ``text`` from the local encoding to utf-8.

    :param string text:
    :returns: string

.. lua:function:: icebergsupport.crlf2lf(text)

    Converts newline characters in ``text`` from ``crlf`` to ``lf`` .

    :param string text:
    :returns: string

Regular expressions
~~~~~~~~~~~~~~~~~~~~~~
UTF-8 aware regular expression APIs.  Flags are a bitwise or of :lua:attr:`Regex.S` , :lua:attr:`Regex.I` and so on. ( :lua:func:`icebergsupport.bor` can be used for bitwise operations). You must pass :lua:attr:`Regex.NONE` if no flags.

.. lua:function:: icebergsupport.regex_match(pattern, flags, string[, startpos, endpos])

    Searches for ``pattern`` in ``string`` . (completely matching)

    :param string pattern: the regular expression
    :param number flags: the flags for searching
    :param string string: the  string to be searched
    :param number startpos: a starting byte position for searching
    :param number endpos: an ending byte position for searching
    :returns: [bool:true if found, false otherwise, Regex:Regex object]

.. lua:function:: icebergsupport.regex_search(pattern, flags, string[, startpos, endpos])

    Searches for ``pattern`` in ``string`` . (partial matching)

    :param string pattern: the regular expression
    :param number flags: the flags for searching
    :param string string: the  string to be searched
    :param number startpos: a starting byte position for searching
    :param number endpos: an ending byte position for searching
    :returns: [bool:true if found, false otherwise, Regex:Regex object]

.. lua:function:: icebergsupport.regex_split(pattern, flags, string)

    Splits ``string`` by the occurrences of ``pattern`` .

    :param string pattern: the regular expression
    :param number flags: the flags for searching
    :param string string: the string to be splitted
    :returns: [string]

.. lua:function:: icebergsupport.regex_gsub(pattern, flags, string, repl)

     Returns the string obtained by replacing the leftmost non-overlapping occurrences of pattern in ``string`` by the replacement ``repl``. Backreferences, such as ``%1``, ``%2``, can be used in ``repl`` . Examples:

    .. code-block:: lua

        icebergsupport.regex_gsub("ABC([A-Z]+)", Regex.NONE, "ABCDEFG", "REPLACED")

        # -> "REPLACED"

        icebergsupport.regex_gsub("ABC([A-Z]+)", Regex.NONE, "ABCDEFG", function(re)
         return re:_1()
        end))

        # -> "DEFG"

    :param string pattern: the regular expression
    :param number flags: the flags for searching
    :param string string: the string to be replaced
    :param callback repl: the callback function or a string
    :returns: string

Completions and Options
~~~~~~~~~~~~~~~~~~~~~~~~~

.. lua:function:: icebergsupport.getopts(args, option, [option, option ...])

    Parses options.

    :param table args: the list of arguments, such as ``{"-a", "-b", "action"}``
    :param string option: option names to parse, such as ``-a`` . An option must have suffix ``:`` such as ``-a:`` if the option requires a value
    :returns: [table:successfully parsed options, table:rest values]

.. lua:function:: icebergsupport.comp_state(values, pos, option, [option, option ...])

    Creates a list of completion candidates and analyzes a completion state.

    :param table values: the list of arguments, such as ``{"-a", "-b", "action"}``
    :param number pos: the position in ``values`` that is pointed by the cursor.
    :param table option: option definitions
    :returns: [string:a completion state, table:a list of completion candidates]

These functions are used for implementing completion functions and analyzing options.

:lua:func:`icebergsupport.comp_state` is used in completion functions like the following:

     .. code-block:: lua
     
         function(values, pos)
           local state, opts = ibs.comp_state(values, pos,
             {opt="-a", description="a option", state="aaa"},
             {opt="--abcd", description="a option"},
             {opt="--aefg", description="a option"},
             {opt="-b", description="b option"},
             {opt="-c", description="b option", exclude={"-a"}}
           )
           if state == "aaa" then
             return {"file1", "file2", "file3"}
           elseif state == "opt" then
             return opts
           else
             return {"action1", "action2", "action3"}
           end
         end

An option definition consists of 

:opt:
     An option name. This must start with ``-``.
:description:
     A description for this option.
:state:
     A state name when this option is selected. If the input box value is ``-b -a A`` and now the cursor is just after ``A``, the state name is ``aaa`` in above example.
:exclude:
     Options that cannot be specified if this option is specified. In above example, ``-c`` will not be included in completion candidates if ``-a`` is specified.

In addition, the input box value is ``value -`` and now the cursor is just after the ``-``, the state name is special value ``"opt"`` .

:lua:func:`icebergsupport.getopts` is used in command functions like the following:

     .. code-block:: lua
     
         function(a)
           local opts, args = ibs.getopts(a, "-a:", "-b", "-c")
           if opts.a == nil then
             ibs.message("-a must not be empty.")
           else
             if opt.b then
               ibs.shell_execute(args[1])
               -- blah blah blah
             elseif opt.c then
               -- blah blah blah
             end
           end
         end

``opts`` is a table with keys that are an option name without ``-`` prefix. If the option has a ``:`` suffix, the next argument will be evaluated as a value. Unknown options are stored in ``args`` .

In above example, If ``a`` is ``{"-a", "file1", "-b", "action", "parameter"}``, ``getopts`` returns ``opts.a = "file1"; opts.b = true; args = {"action", "parameter"}`` .


iceberg operations
~~~~~~~~~~~~~~~~~~~

.. lua:function:: icebergsupport.version()

    Returns a version string of iceberg.

    :returns: string

.. lua:function:: icebergsupport.hide_application()

    Hides all windows of iceberg.

.. lua:function:: icebergsupport.show_application()

    Shows iceberg.

.. lua:function:: icebergsupport.do_autocomplete()

    Runs the autocompletion.

.. lua:function:: icebergsupport.get_cwd()

    Returns the current working directory of iceberg.

    :returns: string

.. lua:function:: icebergsupport.set_cwd(path)

    Changes the current working directory of iceberg.

    :param string path: the directory path
    :returns: [bool:true if no errors, false otherwise, string:an error message]

.. lua:function:: icebergsupport.set_result_text(text)

    Sets ``text`` to the input box as a message.

    :param string text: the message

.. lua:function:: icebergsupport.find_command(name)

    Tries to find a command and returns the command as a table.

    :param string name: the name of the command
    :returns:
        [bool:true if the command is found, false otherwise. , table or string:a table if the command is found, an error message otherwise]

        table consists of 

        :name: a command name
        :path: a command path. This is a ``string`` or a ``function`` 
        :cmdpath: a command path without arguments
        :workdir: a directory that will be used as the current directory
        :description: a description
        :icon: an icon path
        :terminal: whether this command must be run in the terminal( ``"yes"`` , ``"no"``  or ``"auto"`` )
        :history: whether this command is added to the history.
        
.. lua:function:: icebergsupport.to_path(text)

    Returns ``text`` if ``text`` is a path, otherwise tries to find a command named ``text`` and returns ``path`` of the command.

    :param string text:
    :returns: [bool:true if no errors, false otherwise. , string: a path if no errors, an error message otherwise]

.. lua:function:: icebergsupport.to_directory_path(text)

    Returns ``text`` if ``text`` is a path, otherwise tries to find a command named ``text`` and returns ``path`` of the command.

    :param string text:
    :returns: [bool:true if no errors, false otherwise. , string: a path if no errors, an error message otherwise]

.. lua:function:: icebergsupport.message(text)

    Shows a popup message.

    :param string text: the message

.. lua:function:: icebergsupport.event_key()

    Returns a number corresponds to the current GUI event.

    :returns: number

.. lua:function:: icebergsupport.event_state()

    Returns a number that is a bitfield of what modifier keys were on during the most recent GUI event. This bitfield consists of :lua:data:`icebergsupport.EVENT_STATE_*` .

    :returns: number

.. lua:function:: icebergsupport.matches_key(key)

    Returns true if event ``key`` is held, false otherwise. ``key`` is a string such as ``ctrl-a`` and ``ctrl-alt-space`` .

    :param string key:
    :returns: bool

.. lua:function:: icebergsupport.exit_application()

    Shuts iceberg down.

.. lua:function:: icebergsupport.reboot_application()

    Reboots iceberg.

.. lua:function:: icebergsupport.scan_search_path(category)

    Scans the search path that belongs to ``category`` .

    :param string category:

.. lua:function:: icebergsupport.get_input_text()

    Returns a value of the input box.

    :returns: string

.. lua:function:: icebergsupport.set_input_text(text)

    Sets ``text`` to the input box.

    :param string text:
    
.. lua:function:: icebergsupport.get_input_text_values()

    Parses the value of the input box and returns a list of strings. This function parses a text that is shown in the input box, so if you want to parse a value including an autocompleted text, you have to call :lua:func:`icebergsupport.do_autocomplete` before calling this function.

    :returns: table

.. lua:function:: icebergsupport.get_clipboard()

    Returns the content of the clipboard.

    :returns: string

.. lua:function:: icebergsupport.set_clipboard(text)

    Sets ``text`` to the clipboard

    :param string text:

.. lua:function:: icebergsupport.get_clipboard_histories()

    Returns the list of the clipboard histories.

    :returns: table

.. lua:function:: icebergsupport.selected_index()

    Returns the index of the selected completion candidate. An index starts at 1(not 0). 0 means no selection.

    :returns: number

.. lua:function:: icebergsupport.command_execute(name [, args])

    Runs the command named ``name`` .

    :param string name:
    :param [string] args: a list of arguments
    :returns: [bool:true if no errors, false otherwise. , string:an error message]

.. lua:function:: icebergsupport.default_after_command_action(success, message)

    Make the default behavior after the command execution. If ``success`` is true, this function clears the input box and hides all iceberg windows, otherwise shows ``message`` using the popup window. Typically, this function is used in combination with :lua:func:`icebergsupport.command_execute` such as ``icebergsupport.default_after_command_action(icebergsupport.command_execute("cmd", {"arg0", "arg1"}))``

    :param bool success: whether the command was succeeded
    :param string message:

.. lua:function:: icebergsupport.add_history(input [, name])

    Adds ``input`` to the last of the history. If you want to add a command that is registered with iceberg, pass the command name as ``name`` .

    :param string input: the text to be added to the history(including all arguments)
    :param string name: a command name if ``input`` is registered with iceberg, nil otherwise

.. lua:function:: icebergsupport.open_dir(path)

    Opens ``path`` with the application ``system.file_browser`` .

    :param string path: the directory path
    :returns: [bool:true if no errors, false otherwise. , string:an error message]

.. lua:function:: icebergsupport.group_command(command[, commmand, command ...])

    Creates a new command that executes several commands in sequence. Each commands are run by :lua:func:`icebergsupport.command_execute`, so commands must be registered with iceberg.

    .. code-block:: lua

        group_sample = { path = ibs.group_command({"userdir", {}}, {"np", {}}), description = "runs a group of commands"},

.. lua:function:: icebergsupport.bind_key(key, func)

    Executes ``func`` when ``key`` is pressed. This function is used in the ``on_key_down`` callback function.

    :param string key: the string such as ``ctrl-m``
    :param function func:

.. lua:function:: icebergsupport.is_modifier_pressed(keycode)

    Returns true if modifier keys ``keycode`` are pressed.

    :param number keycode: the bitwise or of the :lua:data:`icebergsupport.EVENT_STATE_*` constants
    :returns: bool

Miscs
~~~~~~~~~~~~~~

.. lua:function:: icebergsupport.dump_lua_object(object, indent, isarrayval)

    Converts ``object`` to a string that can be interpreted as a Lua code.

    :param object object:
    :param number indent: the indent level of the result, this must be 0
    :param bool isarrayval: pass ``true`` if ``object`` is an array
    :returns: string

.. lua:function:: icebergsupport.load_lua_object(text)

    Parses ``text`` as a lua code and returns a table.

    :param string text:
    :returns: [bool:true if no errors, false otherwise, table or string: a table if no errors, an error message otherwise]

.. lua:function:: icebergsupport.grep(text, pattern [, flags])

    Returns the lines of ``text`` that include a regular expression ``pattern``

    :param string text:
    :param string pattern:
    :param number flags:
    :returns: [string]

.. lua:function:: icebergsupport.is_array(table)

    Returns ``true`` if ``table`` is an array. ``table`` is considered as an array if all indecies are a positive number.

    :param table table:
    :returns: bool

.. lua:function:: icebergsupport.merge_table(table, [obj, obj ...])

    Merges multiple tables into the first ``table`` . Elements will be appended after the last element of ``table`` if ``table`` is a list.

    :param table table:

.. lua:function:: icebergsupport.table_find(table, obj)

    Returns the lowest index in the ``table`` where the ``obj`` is found. Returns 0 if ``obj`` is not found.

    :param table table:
    :returns: number


Windows support functions
---------------------------------

.. lua:function:: winsupport.foreground_explorer()

    Returns information of the foreground explorer.exe

    :returns: {path= a path of the explorer, selected={a list of the selected file names}}

.. lua:function:: winsupport.foreground_explorer_path()

    Returns a path of the foreground explorer.exe. When no explorer exists, returns an empty string.

    :returns: string

.. lua:function:: winsupport.shell_start(cmd)

    Launches a new application via ``cmd /c "start CMD"`` .

    :param string cmd: command to run
 

Classes
---------------------------------

.. lua:class:: Regex.new(pattern, flags)

    A regular expression object that handles UTF-8 strings correctly. This object is used in ``icebergsupport.regex_*`` functions.

    :param string pattern: the regular expression that can be used in the Oniguruma
    :param number flags: the regular expression flags(Regex.NONE or a bitwise or of the Regex.S,M and I)

.. lua:attribute:: Regex.NONE

    A regular expression flag that means no flags are set.

.. lua:attribute:: Regex.S

    A regular expression flag that is equivalent to the Perl's s flag.

.. lua:attribute:: Regex.M

    A regular expression flag that is equivalent to the Perl's m flag.

.. lua:attribute:: Regex.I

    A regular expression flag that is equivalent to the Perl's i flag.

.. lua:function:: Regex.escape(text)

    Escapes all meta characters in ``text``

    :param string text:
    :returns: string

.. lua:function:: Regex:_1()

    Returns the subgroup string of the match.

    ``Regex:_1()`` , ``Regex:_2()`` ... ``Regex:_9()`` are defined.

    :returns: string

.. lua:function:: Regex:group(group)

    Returns the subgroup string of the match.

    :param number group:
    :returns: string

.. lua:function:: Regex:startpos(group)

    Returns the index of the start of the substring matched by ``group`` .

    :param number group:
    :returns: number

.. lua:function:: Regex:endpos(group)

    Returns the index of the end of the substring matched by ``group`` .

    :param number group:
    :returns: number
