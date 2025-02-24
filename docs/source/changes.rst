Changes
=======================
0.9.13 (2025-02-24)
-----------------------
- CHANGED: update fltk dependencies(1.4.2), so iceberg now works nicely with HiDPI displays on Linux.

0.9.12 (2019-01-17)
-----------------------
- CHANGED: update fltk dependencies(1.3.4)
- IMPROVED: Now commands and ``icebergsupport.shell_execute`` take a ``sudo`` parameter.
  If this value is set to ``true``, the command will be run as an administrator user.
- IMPROVED: Now iceberg uses a more good looking message box

How to upgrade from 0.9.11
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- Download 0.9.12
- Copy ``iceberg.exe`` from 0.9.11 and paste it to the 0.9.12
- Delete ``commands.cache`` and  ``icons.cache``
- Execute ``:scan_search_path all``

0.9.11 (2016-10-03)
-----------------------
- IMPROVED: Improve DirectWrite drawing perfornamce.
- IMPROVED: Add ``direct_write_params`` option, This allows you to configure the details of DirectWrite.
- IMPROVED: Now ``XDG_CONFIG_PATH`` affects a config file path on Linux.
- IMPROVED: Now double clicking on the list box runs a selected command.
- CHANGED:  Change the lua module directory name from  ``luamodule`` to ``lualibs`` .
- FIXED: Fix a bug where a multibyte font name was ignored when DirectWrite is enabled.
- NEW: Add ``icebergsupport.get_input_text_values`` 
- NEW: Add ``auto_merge`` 
- CHANGED: Translate the main content of the documents from Japanese to English.

How to upgrade from 0.9.10
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- Download 0.9.11
- Copy ``iceberg.exe`` from 0.9.11 and paste it to the 0.9.10
- Replace ``luamodule`` to ``lualibs`` in ``config.lua`` and ``luamodule/winsupport.lua``

0.9.10 (2016-08-30)
-----------------------
- IMPROVED: Implement DirectWrite on Windows.

How to upgrade from 0.9.9
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- Download 0.9.10
- Copy ``iceberg.exe`` from 0.9.10 and paste it to the 0.9.9
- Add ``disable_direct_write=true`` to the ``system`` global variable in ``config.lua`` . If you want to enable DirectWrite, set false.

0.9.9 (2016-01-31)
-----------------------
- IMPROVED: Now completion functions takes an argument position as a second parameter.
- NEW: Add ``icebergsupport.table_find`` , ``icebergsupport.getopts`` and ``icebergsupport.comp_state`` .
- FIXED: Fix a possible deadlock while loading icons.
- FIXED: Fix a possible segfault while typing a keyboard.

How to upgrade from 0.9.8
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- Download 0.9.9
- Copy ``iceberg.exe`` from 0.9.9 and paste it to the 0.9.8
- Copy ``luamodule/icebergsupport.lua`` from 0.9.9 and paste it to the 0.9.8

0.9.8 (2015-12-27)
-----------------------
- NEW: Command definitions now have a ``terminal`` attribute.
- IMPROVED: Now iceberg detects an update of environment variables on Windows, so you do not need to reboot the iceberg when you update environment variables anymore.
- IMPROVED: Standardization a plugin structure.
- IMPROVED: Now the ``config.d`` directory is available on Windows.
- FIXED: Fix the bug where loading a GIF or an XPM file causes segfault.

How to upgrade from 0.9.7
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- Download 0.9.8
- Copy ``iceberg.exe`` and ``luamodule/icebergsupport.lua`` from 0.9.8 and paste it to the 0.9.7
- Delete ``commands.cache`` and  ``icons.cache``
- Create ``plugins`` directory under ``ICEBERG_CONFIG_HOME``
- Create ``config.d`` directory under ``ICEBERG_CONFIG_HOME`` (Windows user only)
- Add ``terminal`` to ``system`` global variable in ``config.lua`` like the following (Windows user only)

    .. code-block:: lua

        system = {
          terminal = "cmd /k ${1}",
        }

- Add the following line to the end of the ``config.lua`` 

    .. code-block:: lua

        ibs.load_lua_files(ibs.join_path(script_path, "config.d"))
        ibs.load_plugins()


0.9.7 (2015-11-19)
-----------------------
- NEW: Now iceberg works on Linux!
- NEW: Now you can use svg images as icons.
- CHANGED: Update fltk to 1.3.3.
- CHANGED: Update Oniguruma to 5.9.6.

How to upgrade from 0.9.6
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- Download 0.9.7
- Copy ``iceberg.exe`` from 0.9.7 and paste it to the 0.9.6
- Delete ``commands.cache`` and  ``icons.cache``

0.9.6 (2014-10-20)
-----------------------
- FIXED: Fix the bug where some icons are not loaded correctly.
- NEW:  Add ``path_autocomplete`` and ``option_autocomplete`` to ``system`` .
- FIXED: Fix a a possible crash while reading certain paths.
- CHANGED: Now iceberg binary distributions are compiled with MinGW-W64 4.9.1 .
- IMPROVED: Now you can set a single key to ``hot_key`` .

How to upgrade from 0.9.5
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- Download 0.9.6
- Copy ``iceberg.exe`` from 0.9.6 and paste it to the 0.9.5
- Delete  ``icons.cache``
- Add ``path_autocomplete`` and ``option_autocomplete`` to ``system`` global variable in ``config.lua`` like the following:

    .. code-block:: lua

        system = {
          path_autocomplete = true,
          option_autocomplete = true,
        }


0.9.5 (2014-03-04)
-----------------------
- FIXED: ``'&'`` can not be shown in the list box.
- FIXED: Some key combinations(e.g. ``shift-tab`` ) do not work correctly.

0.9.4 (2013-11-05)
-----------------------
- NEW: Add ``always_match`` to completion candidates.
- NEW: ``icebergsupport.selected_index`` , ``icebergsupport.brshift`` and ``icebergsupport.blshift`` 

0.9.3 (2013-11-01)
-----------------------
- FIXED: shift+arrow keys does not work.
- FIXED: Fix a possible crash while opening the context menu of certain items.
- FIXED: Relative paths are not converted correctly to an absolute path.
- FIXED: Fix the bug where rebooting is failed if ``server_port`` is enabled.
- CHANGED: Now record a command name to the history even though ``history`` is set to ``false`` .
- IMPROVED: A modal window can be closed by the Enter key.

0.9.2 (2013-09-07)
-----------------------
- FIXED: Non-threadsafe functions are called under a multithreaded environment without locks.
- IMPROVED: Now completion functions are defined in a command definition.
- NEW: ``icebergsupport.get_clipboard_histories`` and ``system.max_clipboard_histories`` .
- NEW: ``clipboard`` command as a default command .
- NEW: ``icebergsupport.add_history``
- NEW: ``-m activate`` CUI option

0.9.1 (2013-08-24)
-----------------------
- FIXED: Can not execute ``*.lnk`` when the input box has a value that is enclosed in ``"`` .
- FIXED: Compiletion candidates is not filtered correctly when a completion function returns a table.
- IMPROVED: Now you can use jpeng files as an icon.
- IMPROVED: More effective icon caching methods.
- NEW: ``icebergsupport.unquote_path``  .
- NEW: ``alttab`` command as a default command.

0.9.0 (2013-08-15)
-----------------------
- First release
