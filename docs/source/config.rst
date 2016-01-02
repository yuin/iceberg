環境設定
=================================
概要
--------------------
設定は `Lua <http://www.lua.org>`_ による設定ファイルに記載します。Windowsの場合iceberg実行ファイルと同一ディレクトリ、Linuxの場合 ``~/.iceberg`` の ``config.lua`` が読み込まれます。ヒストリファイルなどは設定ファイルと同一のディレクトリに保存されます。

また、Linuxの場合 ``~/.iceberg/config.d/*.lua`` ファイルが起動時に自動的に読み込まれるためこちらに設定を記載することも可能です。

設定ファイルにはいくつかのグローバル変数とグローバル関数を記載します。

キー名称
--------------------
この後の章で示す設定には以下のキー名称が使用できます。また、修飾キーと通常キーを ``-`` でつなぎ合わせることで同時押しを表現します。同時押しのキーは3つまでです。また、ホットキーの設定のみOSネイティブの仮想キーコードを受け付けます。

例：

    .. code-block:: lua

        "alt-space"
        "shift-ctrl-a"
        "0x1d" -- Windowsにおける「無変換」キー
        "0x1c" -- Windowsにおける「変換」キー

- 修飾キー
    - ``shift``
    - ``caps_lock``
    - ``ctrl``
    - ``alt``
    - ``num_lock``
    - ``meta``
    - ``scroll_lock``

- 通常キー
    - ``a`` ～ ``z``
    - ``0`` ～ ``9``
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
    - ``kp0`` : kpから始まるキーはテンキー上のキーです。
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

systemグローバル変数
---------------------
設定例
~~~~~~~~~~~~~~~~~~~~~
以下に設定例と値の意味を示します。

    .. code-block:: lua

        system = {
          -- search pathから検索する際の深さのデフォルト値 --
          default_search_path_depth = 2,

          -- 補完候補のアイコンを表示するか(true:する, false:しない) --
          enable_icons = true,

          -- アイコンテーマ名:Linuxでのみ有効 --
          icon_theme = "nuoveXT.2.2",

          -- 補完候補のアイコンのキャッシュ数 -- 
          max_cached_icons = 9999,

          -- 何msキー入力が無い場合に補完候補を表示するか --
            -- 低速マシンの場合、自身のキータイプ間隔以上の値にすることにより
            -- 不要な補完動作を抑制することができます。
          key_event_threshold = 0,

          -- ヒストリの保存数 -- 
          max_histories = 500,

          -- 補完候補の表示数 -- 
          max_candidates = 15,

          -- クリップボード履歴の保存数:Windowsでのみ有効 -- 
          max_clipboard_histories = 15,

          -- 補完候補ソート時のヒストリの影響度合い(0.0~1.0)
          history_factor = 0.8,

          -- ディレクトリを開く際のコマンド、${1}にディレクトリパスが入る --
          file_browser = [[explorer ${1}]],

          -- ターミナルでコマンドを実行する際のコマンド、${1}コマンドが入る。ログインターミナルとして起動すること。:Linuxでのみ有効 --
          terminal = [[lxterminal -l -e ${1}]],

          -- 外部からコマンドを受け付けるポート(0: 無効)
          server_port = 13505,

          -- パス補完時に自動補完を有効にする
          path_autocomplete = true,

          -- 引数の補完時に自動補完を有効にする
          option_autocomplete = true,

          -- コマンド・ヒストリの補完時は自動補完は常に有効です --
        
          -- キー設定 --
          -- hot_keyはOS仮想キーコードでも可、単一キーも可。 --
          --   例: hot_key = "0x1d"  無変換 --
          hot_key = "ctrl-space",
          escape_key = "escape",
          list_next_key = "ctrl-n",
          list_prev_key = "ctrl-p",
          toggle_mode_key = "ctrl-r",
          kill_word_key = "ctrl-w",
        
          -- サーチパス --
          search_path = {
            {category="system", path = [[C:\Windows\System32]], depth = 1, pattern="^.*\\.(exe)$"}, 
            {category="programs", path = [[C:\Users\]] .. os.getenv("USERNAME") .. [[\AppData\Roaming\Microsoft\Windows\Start Menu\Programs]], depth = 10, pattern=[[^.*\.(exe|lnk)$]]},
            {category="programs", path = [[C:\ProgramData\Microsoft\Windows\Start Menu\Programs]], depth = 10, pattern=[[^.*\.(exe|lnk)$]]},
          },

          -- 補完設定 -- 
          completer = {
            -- コマンドの補完:あいまい一致 --
            command = ibs.COMP_ABBR,

            -- パスの補完:前方一致 -- 
            path    = ibs.COMP_BEGINSWITH,

            -- ヒストリの補完:部分一致 -- 
            history = ibs.COMP_PARTIAL,

            -- 引数の補完 -- 
            option  = ibs.COMP_PARTIAL,
        
            -- 引数補完のための関数 --
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

サーチパス
~~~~~~~~~~~~~~~~~
サーチパスは、指定したディレクトリ配下を検索し自動的にコマンドとして登録する機能です。サーチパスの構成要素は以下です。

:category:
    サーチパスはカテゴリを持つことができます。指定したカテゴリのサーチパスのみを更新することが可能です。無指定の場合自動的に ``default`` というカテゴリに属します。

:path:
    検索する起点となるディレクトリです。

:depth:
    ``path`` から何階層検索するかを示す数値です。無指定の場合 ``system.default_search_path_depth`` が適応されます。

:pattern:
    コマンドとして登録するファイル名の正規表現パターン(完全一致)です。

補完関数
~~~~~~~~~~~~~~~~~
icebergではコマンドが入力された際の引数を補完する関数を定義できます。補完関数は以下のシグネチャです。

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

``values`` は入力されている引数の配列です。引数が空の場合、空文字1要素になります。 ``pos`` は現在カーソルがある引数の配列における位置です。関数は補完候補を文字列のリストもしくは次の要素を含むテーブルのリストとして返す必要があります。文字列とテーブルを混在させることはできません。

:value:
    補完文字列です。この項目は必須です。
:icon:
    アイコンとして使用するファイルのパスです。
:description:
    説明として利用する文字列です。
:always_match:
    ``true`` を設定するとどんな入力にもマッチするようになり、選択時に入力欄が更新されなくなります。例えばWEB検索結果をただ表示する場合などに利用します。

補完関数は補完以外に情報表示だけのために利用することもできます。デフォルトの ``weather`` コマンドを参照してください。


commandsグローバル変数
-----------------------
設定例
~~~~~~~~~~~~~~~~~~~~~
以下に代表的なコマンドの定義例を示します。

    .. code-block:: lua


        commands = { 
          -- ディレクトリ,ヒストリに残さない --
          windir = {path = [[C:\Windows]], history = false},

          -- 実行ファイル, icebergのカレントディレクトリで実行 -- 
          np = {path = [[notepad.exe]], description="Notepad", workdir="."},

          -- シェルスクリプト, ターミナルで実行する --
          np = {path = [[myscript.sh]], description="My script", terminal="yes"},

          -- lua関数, 補完関数あり -- 
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


          -- URL, アイコン画像を指定 -- 
          google = { path = [[http://www.google.com/search?ie=utf8&q=${1}]], description=[[Searches words on Google]], history=false,
               icon = script_path ..[[images\google256.png]]},

          -- グループコマンド：連続してコマンドを実行 -- 
          group_sample = { path = ibs.group_command({"windir", {}}, {"np", {}}), description = "runs a group of commands"},
        
        }

コマンド
~~~~~~~~~~~~~~~~~~~~~~~
コマンドは以下の要素から構成されます。

:name: ``commands`` tableのキーとして表現されます。
:path: 
    実行対象のパスもしくはLuaの関数です。
    パスの場合、以下のように引数を含めることができます。また空白を含む場合は ``"`` で囲う必要があります。::

        path = [["C:\s p a c e\bin.exe" arg1 arg2]]

    また入力された変数を参照することができます。 ``google iceberg`` と入力された場合、 ``google`` コマンドの ``path`` が以下の場合、 ``${1}`` に ``iceberg`` が代入されます。::

        path = [[http://www.google.com/search?ie=utf8&q=${1}]]

    関数の場合、引数には文字列のリストが与えられます。関数は実行に成功した場合0を、失敗した場合は非0を返す必要があります。
:completion:
    ``system.completer.option_func`` と同じ形式の補完関数です。補完関数はコマンドでも ``system.completer.option_func`` でも登録できます。両方登録した場合はコマンドで定義したものが優先されます。
:description:
    補完候補ウインドウに表示される説明文です。
:icon:
    補完候補ウインドウに表示されるアイコン画像のパスです。Linuxの場合、アイコンテーマにおけるアイコン名でもかまいません。アイコン名の場合自動的に最適な画像を選択します。
:terminal:
    ターミナルで実行するかを示します。yesの場合常にターミナルで実行します。noの場合ターミナルで実行しません。autoの場合は自動的にターミナルで実行するか判定します。ターミナル実行コマンドは ``system.terminal`` で指定されたコマンドです。
:history:
    ``false`` を指定するとヒストリに残らなくなります。
:workdir:
    コマンドを実行するディレクトリです。以下の指定が可能です。

    - 固定値: そのディレクトリで実行されます。
    - ``.`` : icebergのカレントディレクトリで実行されます。
    - Lua関数: 関数の戻り値(文字列)のディレクトリで実行されます。例えば、外部ファイラのディレクトリを返す関数を設定すれば外部ファイラと連携できます。

shortcutsグローバル変数
------------------------
設定例
~~~~~~~~~~~~~~~~~~~~~
以下に代表的なショートカットの定義例を示します。

    .. code-block:: lua

        shortcuts = {
          { key = "ctrl-d", name = ":opendir" },
          { key = "ctrl-l", name = ":cd" }
        }

上記のようにショートカットを定義し、 ``c:\`` と入力欄に入力された状態で ``ctrl-l`` を押下したとします。その時以下のようにコマンドが実行されます。::

    :cd c:\

つまり、コマンドの引数として入力欄に入力されている値が渡されます。

on_key_upイベントハンドラ
--------------------------
キーが離された時に呼び出されます。

    .. code-block:: lua

        function on_key_up()
          local accept = 0
          return accept
        end

デフォルトの動作を抑止したい場合はこの関数で1を返してください。

on_key_downイベントハンドラ
---------------------------------
キーが押された時に呼び出されます。

    .. code-block:: lua

        function on_key_down()
          local accept = 0
          return accept
        end

デフォルトの動作を抑止したい場合はこの関数で1を返してください。

on_enterイベントハンドラ
--------------------------
enterキーが押下された際に呼び出されます。

    .. code-block:: lua

        function on_enter()
          local accept = 0
          return accept
        end

デフォルトの動作を抑止したい場合はこの関数で1を返してください。

on_initializeイベントハンドラ
--------------------------------
起動時に呼び出されます。

    .. code-block:: lua

        function on_enter()
          local error = 0
          return error
        end

この関数が1を返した場合、起動を停止します。
