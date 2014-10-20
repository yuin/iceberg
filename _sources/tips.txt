Tips
=================================
概要
--------------------
このページでは便利な使い方やお勧めの設定を紹介します。

ポチエス連携
--------------------------------------------
`ポチエス <http://www.geocities.jp/pochi_s2004/pochi.html>`_ はファイルをどのアプリケーションで実行するか、選択可能にするアプリケーションです。ここでは、コマンドもしくはファイルパスをicebergに入力し、 ``shift-enter`` を押すことでポスエスで実行する設定例を示します。

コマンド定義

    .. code-block:: lua

        commands = {
          esExt5  = {path=function(args)
            if #args ~= 0 then
              local ok, path = ibs.to_path(args[1])
              if ok then
                ibs.shell_execute([[ポチエスexeへのパス]], {path}, [[ポチエスのディレクトリ]])
              end
            end
            return 0
          end, history=false}
        }

ショートカット定義

    .. code-block:: lua
        
        shortcuts = {
          { key = "shift-enter", name = "esExt5" }
        }


現在の日付・時刻をクリップボードにコピー
-------------------------------------------------
以下のようなコマンド定義を行います。

    .. code-block:: lua

        date_YYYYMMDD = {path = function(args) ibs.set_clipboard(os.date("%Y%m%d")) end, history = false},
        date_YYYYMMDDHHMMSS = {path = function(args) ibs.set_clipboard(os.date("%Y%m%d%H%M%S")) end, history = false},

エクスプローラで選択しているファイルをエディタで開く
--------------------------------------------------------
エクスプローラでファイルを選択し、iceberg上で ``ctrl-m`` を実行するとvimでファイルを開く設定です。

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

コマンド登録されているディレクトリ配下の表示
--------------------------------------------------------
例えば ``app`` というコマンドとして ``c:\app`` というディレクトリが登録されているとします。このときに ``app`` と入力し ``ctrl-l`` を実行するとカレントディレクトリが ``c:\app`` に変更されます。このあとに ``./`` と入力すると ``c:\app`` の配下のファイルを表示することができます。

数式入力で計算を行う
--------------------------------------------------------
以下のように ``on_enter`` を定義することで ``10 + 20`` のように数式を入力してEnterを押下するとすぐに計算を実行することができます。


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

起動時に自動的に:scan_search_pathを行う
--------------------------------------------------------
以下のように ``on_initialize`` を定義することで起動時に自動的に ``:scan_search_path`` することができます。 ``all`` 以外のカテゴリで実行したい場合は ``{"all"}`` となっている場所を書き変えてください。

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

foobar2000を操作する
--------------------------------------------------------
以下の様に定義するとicebergからfoobar2000が操作できます。


    .. code-block:: lua
        
        foobar2000 = { 
          path=[[foobar2000のパス]],
          completion = function(values) 
            return {
              {value="/playpause", description = "再生/一時停止"},
              {value="/stop", description = "停止"},
              {value="/pause", description = "一時停止"},
              {value="/play", description = "再生"},
              {value="/prev", description = "前へ"},
              {value="/next", description = "次へ"},
              {value="/rand", description = "ランダム再生"},
              {value="/exit", description = "終了"},
              {value="/show", description = "表示"},
              {value="/hide", description = "最小化"}
            }
          end,
          history = false
        }
