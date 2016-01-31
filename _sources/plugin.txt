プラグイン
=================================
概要
--------------------
icebergには柔軟に機能を追加できるプラグイン機能があります。ここではプラグインの使用方法と開発方法を記載します。

プラグインの使用方法
----------------------
配布されているプラグインを icebergディレクトリ配下の ``plugins`` ディレクトリに配置します。配置されたプラグインはiceberg起動時に自動的にロードされます。

また、プラグインのディレクトリ名を ``_`` ではじまるようにリネームするとプラグインを無効にする事ができます。

プラグインの開発方法
---------------------
プラグインの構造
~~~~~~~~~~~~~~~~~~~~~
プラグインは ``main.lua`` が配置された単純なディレクトリです。典型的には以下のような構造です。
    
    .. code-block:: lua

        plugin-name          - プラグイン名のディレクトリ
          |
          +---- main.lua     - プラグインのメインスクリプト
          |
          +---- icon.svg     - アイコン

main.luaテンプレート
~~~~~~~~~~~~~~~~~~~~~
以下にプラグインのテンプレートを示します。

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

- 04行目 : 設定をローカル変数に保持します。
- 08行目 : ``plugin_プラグイン名`` というグローバル変数で設定変更が可能なようにします。具体的には ``config`` ローカル変数を ``plugin_プラグイン名`` グローバル変数で上書きします。
- 09行目 : コマンドを登録します。コマンドの定義方法は :doc:`config` を参照してください。

プラグイン
---------------------
いくつかプラグインを紹介します。

- `iceberg-ip <https://github.com/yuin/iceberg-ip>`_ : ローカルマシンのIPアドレスを表示します。
- `iceberg-worldtime <https://github.com/yuin/iceberg-worldtime>`_ : 世界の都市の時間を表示します。
