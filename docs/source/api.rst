API
=================================
概要
---------------------------------
icebergは `Lua <http://www.lua.org>`_ を拡張言語として組み込んでいます。ユーザはLuaスクリプトによってicebergの機能を拡張することができます。 ``icebergsupport`` モジュールには拡張用の関数やクラスが用意されています。

文字コード
---------------------------------
icebergは内部的にutf8で処理を行っています。そのためAPIでやりとりされる文字列は基本的にutf8が用いられます。ただし、 :lua:func:`icebergsupport.command_output` はコマンドが出力した文字コードのまま結果を返します。

`Lua <http://www.lua.org>`_ はマルチバイト文字を扱うAPIを持ちません。マルチバイト文字を文字単位で操作したい場合は `icebergsupport` モジュールの正規表現APIを利用してください。


定数
---------------------------------

.. lua:attribute:: icebergsupport.EVENT_STATE_*

  :lua:func:`icebergsupport.matches_key` などで使用できるイベント定数群です。詳細は :lua:func:`icebergsupport.matches_key` を参照してください。

.. lua:attribute:: icebergsupport.COMP_*

   補完方法を示す定数です。

   - ``COMP_BEGINSWITH`` : 前方一致
   - ``COMP_PARTIAL`` : 部分一致
   - ``COMP_ABBR`` : 曖昧検索

関数
---------------------------------
パス操作
~~~~~~~~~~~~~~

.. lua:function:: icebergsupport.dirname(path)

   ``path`` のディレクトリパスを返します。

   :param string path: パス
   :returns: string:ディレクトリパス

.. lua:function:: icebergsupport.basename(path)

   ``path`` のbasenameを返します。

   :param string path: パス
   :returns: string:basename

.. lua:function:: icebergsupport.directory_exists(path)

   ``path`` で示されるディレクトリが存在するか返します。

   :param string path: パス
   :returns: bool:存在すればtrue,でなければfalse 

.. lua:function:: icebergsupport.file_exists(path)

   ``path`` で示されるファイルが存在するか返します。

   :param string path: パス
   :returns: bool:存在すればtrue,でなければfalse 

.. lua:function:: icebergsupport.path_exists(path)

   ``path`` で示されるパスが存在するか返します。

   :param string path: パス
   :returns: bool:存在すればtrue,でなければfalse 

.. lua:function:: icebergsupport.join_path(pathparts[, pathparts, pathparts ...])

   ``pathparts`` をパスとして結合します。

   :param string pathparts: パス
   :returns: string:結合後の文字列

.. lua:function:: icebergsupport.list_dir(path)

   ``path`` で示されるディレクトリ配下のファイルリストを返します。

   :param string path: パス
   :returns: [bool:成功すればtrue,でなければfalse, table or string:成功すればファイルのリストを示すtable,でなければエラーメッセージ]

.. lua:function:: icebergsupport.quote_path(path)

   ``path`` に空白が含まれる場合 ``"`` で囲います。

   :param string path: パス
   :returns: string: 変換後パス

.. lua:function:: icebergsupport.unquote_path(path)

   ``path`` が ``"`` で囲われている場合、 ``"`` をとりのぞきます。

   :param string path: パス
   :returns: string: 変換後パス


ビット演算
~~~~~~~~~~~~~~

.. lua:function:: icebergsupport.band(number[, number, number ...])

   ``numbers`` の論理積を返します。numberはlua_Integer型として扱われます。

   :param [number] number: 数値
   :returns: number: 結果の値

.. lua:function:: icebergsupport.bor(number[, number, number ...])

   ``numbers`` の論理和を返します。numberはlua_Integer型として扱われます。

   :param [number] number: 数値
   :returns: number: 結果の値

.. lua:function:: icebergsupport.bxor(number[, number, number ...])

   ``numbers`` の排他的論理和を返します。numberはlua_Integer型として扱われます。

   :param [number] number: 数値
   :returns: number: 結果の値

.. lua:function:: icebergsupport.brshift(number, disp)

   ``number`` を ``disp`` ビット右にシフトした値を返します。numberはlua_Integer型として扱われます。

   :param number number: 数値
   :param number disp: シフト分
   :returns: number: 結果の値

.. lua:function:: icebergsupport.blshift(number, disp)

   ``number`` を ``disp`` ビット左にシフトした値を返します。numberはlua_Integer型として扱われます。

   :param number number: 数値
   :param number disp: シフト分
   :returns: number: 結果の値


システム
~~~~~~~~~~~~~~

.. lua:function:: icebergsupport.build_platform()

   ビルドされたプラットフォーム情報を返します。

   :returns: string: ``win_64`` のような文字列

.. lua:function:: icebergsupport.runtime_platform()

   実行しているプラットフォーム情報を返します。

   :returns: string: ``6.1.7601 x64`` のような文字列

外部コマンド
~~~~~~~~~~~~~~

.. lua:function:: icebergsupport.shell_execute(path [, args, workdir])

   外部コマンドを起動します。

   :param string path: 実行するコマンドのパス
   :param [string] args: コマンドに対する引数の配列
   :param string workdir: 実行ディレクトリ。指定が無い場合はカレントディレクトリで実行されます。
   :returns: [bool:成功ならtrueでなければfalse, string:エラーメッセージ]

.. lua:function:: icebergsupport.command_output(command)

    外部コマンド ``command`` を実行し標準出力と標準エラー出力を返します。

   :param string command: 実行するコマンド
   :returns: [bool:成功ならtrueでなければfalse, string:標準出力, string:標準エラー出力]

文字コード
~~~~~~~~~~~~~~

.. lua:function:: icebergsupport.utf82local(text)

   ``text`` をutf-8からマシンローカルの文字コードに変換します。

   :param string text: 文字列
   :returns: string:変換後の文字列

.. lua:function:: icebergsupport.local2utf8(text)

   ``text`` マシンローカルの文字コードからutf-8に変換します。

   :param string text: 文字列
   :returns: string:変換後の文字列

.. lua:function:: icebergsupport.crlf2lf(text)

   ``text`` の改行コードを ``crlf`` から ``lf`` に変換します。

   :param string text: 文字列
   :returns: string:変換後の文字列

正規表現
~~~~~~~~~~~~~~
utf-8を正しく扱える正規表現関連APIです。正規表現フラグは :lua:attr:`Regex.S` や :lua:attr:`Regex.I` の論理和( :lua:func:`icebergsupport.bor` が利用できます) として表現されます。無指定の場合は :lua:attr:`Regex.NONE` を指定します。

.. lua:function:: icebergsupport.regex_match(pattern, flags, string[, startpos, endpos])

   ``string`` から ``pattern`` を検索します。(完全一致)

   :param string pattern: 正規表現
   :param number flags: 正規表現フラグ
   :param string string: 文字列
   :param number startpos: 検索開始位置
   :param number endpos: 検索終了位置
   :returns: [bool:見つかればtrue,でなければfalse, Regex:Regexオブジェクト]

.. lua:function:: icebergsupport.regex_search(pattern, flags, string[, startpos, endpos])

   ``string`` から ``pattern`` を検索します。(部分一致)

   :param string pattern: 正規表現
   :param number flags: 正規表現フラグ
   :param string string: 文字列
   :param number startpos: 検索開始位置
   :param number endpos: 検索終了位置
   :returns: [bool:見つかればtrue,でなければfalse, Regex:Regexオブジェクト]

.. lua:function:: icebergsupport.regex_split(pattern, flags, string)

   ``string`` を ``pattern`` で分割します。

   :param string pattern: 正規表現
   :param number flags: 正規表現フラグ
   :param string string: 文字列
   :returns: [string]:文字列のリスト

.. lua:function:: icebergsupport.regex_gsub(pattern, flags, string, repl)

   ``string`` から ``pattern`` を検索し ``repl`` で置換します。
   ``repl`` には後方参照( ``%1, %2 ...`` )が使用できます。以下に例を示します。

   .. code-block:: lua

       icebergsupport.regex_gsub("ABC([A-Z]+)", Regex.NONE, "ABCDEFG", "REPLACED")

       # -> "REPLACED"

       icebergsupport.regex_gsub("ABC([A-Z]+)", Regex.NONE, "ABCDEFG", function(re)
        return re:_1()
       end))

       # -> "DEFG"

   :param string pattern: 正規表現
   :param number flags: 正規表現フラグ
   :param string string: 置換対象文字列
   :param callback repl: コールバック関数もしくは文字列
   :returns: string:置換後文字列



その他
~~~~~~~~~~~~~~

.. lua:function:: icebergsupport.dump_lua_object(object, indent, isarrayval)

   ``object`` をLuaの文法で解釈できる文字列に変換します。

   :param object object: Luaのオブジェクト
   :param number indent: インデント、呼び出し時は0とする
   :param bool isarrayval: ``object`` が配列の場合trueとする
   :returns: string:変換後の文字列

.. lua:function:: icebergsupport.load_lua_object(text)

   ``text`` で示されるLuaオブジェクトを返します。

   :param string text: Luaのオブジェクトを示す文字列
   :returns: [bool:成功ならtrue,でなければfalse, object or string: 成功ならLuaオブジェクト,でなければエラーメッセージ]

.. lua:function:: icebergsupport.grep(text, pattern [, flags])

   ``text`` のうち 正規表現 ``pattern`` にマッチする行のみを返します。

   :param string text: 文字列
   :param string pattern: 正規表現
   :param number flags: 正規表現のフラグ
   :returns: string: 文字列

.. lua:function:: icebergsupport.is_array(table)

   ``table`` が配列かどうか判定します。すべての添え字が数値の場合配列と判定されます。

   :param table table: table
   :returns: 配列であればtrue, でなければfalse

.. lua:function:: icebergsupport.merge_table(table, [obj, obj ...])

   第一引数の ``table`` と第二引数以降をマージします。第一引数の ``table`` が配列の場合、末尾に要素が追加されます。そうでない場合、第二引数以降の ``table`` のキーを第一引数の ``table`` に上書きします。

   :param table table: table

.. lua:function:: icebergsupport.table_find(table, obj)

   配列 ``table`` 内に ``obj`` が含まれるか検索します。

   :param table table: table
   :returns: 見つかった場合その位置(1はじまり)、そうでない場合は ``0``

補完・オプション解析
~~~~~~~~~~~~~~~~~~~~~

.. lua:function:: icebergsupport.getopts(args, option, [option, option ...])

   コマンド引数の配列からオプションの解析を行います。

   :param table args: コマンド引数の配列(たとえば ``{"-a", "-b", "action"}`` のような)
   :param string option: 解析する引数名。たとえば ``-a`` のような。引数が値を受け取るときは ``-a:`` のように ``:`` を末尾に付与する。
   :returns: [table:解析できたoption, table:その他の値の配列]

.. lua:function:: icebergsupport.comp_state(values, pos, option, [option, option ...])

   補完候補リストの生成と補完状態の解析を実施します。

   :param table values: コマンド引数の配列(たとえば ``{"-a", "-b", "action"}`` のような)
   :param number pos: 現在カーソルがさしている ``values`` における位置
   :param table option: オプション定義
   :returns: [string:補完状態を示す文字列, table:オプションの補完候補リスト]


これらの関数は補完関数の実装や、Luaコマンド実行時のオプション解析に使用します。
補完関数内で以下のように :lua:func:`icebergsupport.comp_state` を使用します。

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

オプション定義は以下より構成されます。

:opt:
    オプション名です。かならず ``-`` ではじめなければいけません。
:description:
    補完候補として表示する際に利用されるオプションの説明です。
:state:
    その引数が指定されている時の状態名です。たとえば ``-b -a A`` と入力されており今 ``A`` にカーソルがある場合上記の例だと ``aaa`` という状態になります。
:exclude:
    同時に指定できないオプションです。上記の例では ``-c`` オプションは ``-a`` が指定されている場合には補完候補に含まれなくなります。

また、たとえば ``value -`` のように入力されており、今 ``-`` の後ろにカーソルがある場合(つまり ``-`` を入力した直後)は ``"opt"`` という状態になります。

コマンドの実行時などのオプション解析は以下のように :lua:func:`icebergsupport.getopts` を使用します。

    .. code-block:: lua
    
        function(args)
          local opts, args = ibs.getopts(args, "-a:", "-b", "-c")
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

``opts`` には引数で渡されたオプション名から ``-`` を取り除いた名前で値が登録された ``table`` です。 オプション名の末尾に ``:`` を付与するとそのオプションは次の文字列を値として読み込みます。未知のオプションは位置パラメータは ``args`` 配列に格納されます。

たとえば上記の例で ``{"-a", "file1", "-b", "action", "parameter"}`` の場合、 ``opts.a = "file1"; opts.b = true; args = {"action", "parameter"}`` という結果になります。

iceberg操作
~~~~~~~~~~~~~~

.. lua:function:: icebergsupport.version()

   icebergのバージョン文字列を返します。

   :returns: string:バージョン文字列

.. lua:function:: icebergsupport.hide_application()

   icebergを非表示にします。

.. lua:function:: icebergsupport.show_application()

   icebergを表示します。

.. lua:function:: icebergsupport.do_autocomplete()

   オートコンプリートを実行します。

.. lua:function:: icebergsupport.get_cwd()

   icebergのカレントディレクトリを返します。

   :returns: string:カレントディレクトリのパス

.. lua:function:: icebergsupport.set_cwd(path)

   icebergのカレントディレクトリを変更します。

   :param string path: ディレクトリパス
   :returns: [bool:成功ならtrue,でなければfalse, string:エラーメッセージ]

.. lua:function:: icebergsupport.set_result_text(text)

   icebergの入力欄にメッセージを設定します。

   :param string text: 設定したいテキスト

.. lua:function:: icebergsupport.find_command(name)

   コマンドを検索します。

   :param string name: コマンド名
   :returns:
       [bool:見つかればtrue,でなければfalse, table or string:見つかった場合はコマンド情報を含むtableでなければエラーメッセージ]

       tableは以下の内容を含みます。

       :name: コマンド名
       :path: コマンドパス
       :cmdpath: コマンドパスから引数を除いたもの
       :workdir: 実行ディレクトリ
       :description: 説明
       :icon: アイコンパス
       :terminal: ターミナルで実行するか(yes:する, no:しない, auto:自動)
       :history: ヒストリへ追加するか
       
.. lua:function:: icebergsupport.to_path(text)

   ``text`` をパスに変換します。 ``text`` がicebergにコマンドとして登録されている場合、そのパスを返します。また、 ``text`` がパスである場合入力をそのまま返します。

   :param string text: 文字列
   :returns: [bool:成功ならtrue,でなければfalse, string:成功ならパス、でなければエラーメッセージ]

.. lua:function:: icebergsupport.to_directory_path(text)

   ``text`` をディレクトリパスに変換します。 ``text`` がicebergにコマンドとして登録されている場合、そのパスを返します。また、 ``text`` がパスである場合入力をそのまま返します。

   :param string text: 文字列
   :returns: [bool:成功ならtrue,でなければfalse, string:成功ならパス、でなければエラーメッセージ]

.. lua:function:: icebergsupport.message(text)

   ポップアップメッセージを表示します。

   :param string text: 表示したいテキスト

.. lua:function:: icebergsupport.event_key()

   現在のイベントに対するキーを返します。

   :returns: number:キー

.. lua:function:: icebergsupport.event_state()

   修飾キーの押下状況を示すビット列(number)を返します。この配列は :lua:data:`icebergsupport.EVENT_STATE_*` とのビット演算に使用できます。

   :returns: number:ビット配列

.. lua:function:: icebergsupport.matches_key(key)

   ``key`` で示されるキーが現在のイベントで発生しているかを返します。 ``key`` は ``ctrl-a`` や ``ctrl-alt-space`` のようにすべて小文字で記載し、修飾キーは ``-`` で繋ぎます。

   :param string key: キーを示す文字列
   :returns: bool:イベントが発生している場合true,でない場合false

.. lua:function:: icebergsupport.exit_application()

   icebergを終了します。

.. lua:function:: icebergsupport.reboot_application()

   icebergを再起動します。

.. lua:function:: icebergsupport.scan_search_path(category)

   ``category`` で指定されるsearch pathを再スキャンします。

   :param string category: search pathのカテゴリ

.. lua:function:: icebergsupport.get_input_text()

   現在入力されている文字列を返します。

   :returns: string:入力されている文字列

.. lua:function:: icebergsupport.set_input_text(text)

   入力欄に ``text`` を設定します。

   :param string text: 設定する文字列
   
.. lua:function:: icebergsupport.get_input_text_values()

   現在入力されている文字列をパースし、文字列のリストを返します。この関数は入力欄に入力されている文字列を解析します。そのため、オートコンプリートを利用しており補完内容も含めて解析したい場合、事前に :lua:func:`icebergsupport.do_autocomplete` を実行する必要があります。

   :returns: table:文字列のリスト

.. lua:function:: icebergsupport.get_clipboard()

   クリップボード上の文字列を返します。

   :returns: string:クリップボード上の文字列

.. lua:function:: icebergsupport.set_clipboard(text)

   クリップボードに ``text`` を設定します。

   :param string text: 設定する文字列

.. lua:function:: icebergsupport.get_clipboard_histories()

   クリップボードの履歴文字列のリストを返します(この関数はWindowsでのみサポートされます)。

   :returns: [string:クリップボード文字列]

.. lua:function:: icebergsupport.selected_index()

   選択されている補完候補のインデックスを返します。インデックスは1からはじまります。選択されていない場合0を返します。

   :returns: number:インデックス

.. lua:function:: icebergsupport.command_execute(name [, args])

   icebergに ``name`` で登録されているコマンドを実行します。

   :param string name: 実行するコマンドのname
   :param [string] args: コマンドに対する引数の配列
   :returns: [bool:成功ならtrueでなければfalse, string:エラーメッセージ]

.. lua:function:: icebergsupport.default_after_command_action(success, message)

   icebergのコマンド実行後のデフォルトアクションを実行します。 ``success`` が ``true`` の場合は入力テキストをクリアし、ウインドウを隠します。 ``false`` の場合は ``message`` を表示します。典型的には ``icebergsupport.default_after_command_action(icebergsupport.command_execute("cmd", {"arg0", "arg1"}))`` のように :lua:func:`icebergsupport.command_execute` と組み合わせて使用します。

   :param bool success: コマンドが成功したかどうか
   :param string message: コマンド失敗時のメッセージ

.. lua:function:: icebergsupport.add_history(input [, name])

   ``input`` をヒストリに追加します。登録済みコマンドのヒストリを追加する場合は ``name`` にコマンド名を渡します。 それ以外の場合は ``input`` のみで構いません。

   :param string input: ヒストリに追加する文字列
   :param string name: ``input`` が登録済みコマンドの場合、コマンド名

.. lua:function:: icebergsupport.open_dir(path)

   ``path`` でしめされるディレクトリを ``system.file_browser`` で指定されているアプリケーションで開きます。

   :param string path: ディレクトリパス
   :returns: [bool:成功ならtrueでなければfalse, string:エラーメッセージ]

.. lua:function:: icebergsupport.group_command(command[, commmand, command ...])

   以下のように使用することで、複数のコマンドを順次実行するコマンドを作成することができます。各コマンドは :lua:func:`icebergsupport.command_execute` を通じて実行されるので、該当のコマンドが登録済みである必要があります。

   .. code-block:: lua

       group_sample = { path = ibs.group_command({"userdir", {}}, {"np", {}}), description = "runs a group of commands"},

.. lua:function:: icebergsupport.bind_key(key, func)

   ``key`` で示されるキーを押下じに ``func`` を実行します。この関数は ``on_key_down`` 内で利用されます。

   :param string key: ``ctrl-m`` のようなキーを示す文字列
   :param function func: 実行する関数

.. lua:function:: icebergsupport.is_modifier_pressed(keycode)

   :param number keycode: :lua:data:`icebergsupport.EVENT_STATE_*` 定数
   :returns: bool:キーが押下されていればtrue,でなければfalse

Windows補助
---------------------------------

.. lua:function:: winsupport.foreground_explorer()

   最前面で表示されているエクスプローラの情報を返します。

   :returns: table:次の要をを含む。{path=エクスプローラのパス, selected={選択されているファイル名の配列}}

.. lua:function:: winsupport.foreground_explorer_path()

   最前面で表示されているエクスプローラのパスを返します。

   :returns: string:エクスプローラで開いているパス、該当がない場合は空文字列


クラス
---------------------------------

.. lua:class:: Regex.new(pattern, flags)

   utf8を正しく解釈可能な正規表現を示すクラスです。
   ``icebergsupport.regex_*`` 関数で利用されます。

   :param string pattern: 正規表現
   :param number flags:   正規表現フラグ

.. lua:attribute:: Regex.NONE

   無指定を示す正規表現フラグです。

.. lua:attribute:: Regex.S

   perlのsフラグと同一です。

.. lua:attribute:: Regex.M

   perlのmフラグと同一です。

.. lua:attribute:: Regex.I

   perlのiフラグと同一です。

.. lua:function:: Regex.escape(text)

   正規表現をエスケープした文字列を返します。

   :param string text: 文字列
   :returns: string:エスケープ後の文字列

.. lua:function:: Regex:_1()

   マッチしたグループの文字列を返します。
   ``Regex:_1()`` , ``Regex:_2()`` ... ``Regex:_9()`` まで定義されています。

   :returns: string:マッチした部分文字列

.. lua:function:: Regex:group(group)

   ``group`` 番目のグループの文字列を返します。

   :returns: string:マッチした部分文字列

.. lua:function:: Regex:startpos(group)

   ``group`` 番目のグループの開始位置を返します。

   :returns: number:開始位置

.. lua:function:: Regex:endpos(group)

   ``group`` 番目のグループの終了位置を返します。

   :returns: number:終了位置
