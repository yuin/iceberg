変更履歴
=======================
0.9.10 (2016-08-30)
-----------------------
- IMPROVED: WindowsでDirectWriteを実装。

0.9.9からのアップグレード方法
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- 0.9.10をダウンロードしてください。
- 0.9.10から ``iceberg.exe`` をコピーして既存の0.9.9側に上書きしてください。
- ``config.lua`` の ``system`` グローバル変数に ``disable_direct_write=true``  を追加してください。DirectWrite機能を利用する場合は ``false`` をセットしてください(ただし、現段階では少々描画が遅いです)。

0.9.9 (2016-01-31)
-----------------------
- IMPROVED: 補完関数の第二引数に現在の引数の位置を渡すようになった。
- NEW: ``icebergsupport.table_find`` , ``icebergsupport.getopts`` , ``icebergsupport.comp_state`` を追加
- FIXED: アイコンロード時にデッドロックする問題を修正
- FIXED: キー入力時に稀にクラッシュする問題を修正

0.9.8からのアップグレード方法
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- 0.9.9をダウンロードしてください。
- 0.9.9から ``iceberg.exe`` をコピーして既存の0.9.8側に上書きしてください。
- 0.9.9から ``luamodule/icebergsupport.lua`` をコピーして既存の0.9.8側に上書きしてください。

0.9.8 (2015-12-27)
-----------------------
- NEW: コマンド定義に ``terminal`` 属性を追加。
- IMPROVED: Windowsにおいてシステム環境変数が変更された場合、icebergを再起動無しに以後起動したコマンドに変更された環境変数が反映されるようになった。
- IMPROVED: Pluginシステムを標準化した。
- IMPROVED: Windowsでも ``config.d`` ディレクトリ配下を読むようになった。
- FIXED: GIFアイコンとXPMアイコンを表示した時にクラッシュする問題を修正。

0.9.7からのアップグレード方法
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- 0.9.8をダウンロードしてください。
- 0.9.8から ``iceberg.exe`` をコピーして既存の0.9.7側に上書きしてください。
- 0.9.8から ``luamodule/icebergsupport.lua`` をコピーして既存の0.9.7側に上書きしてください。
- ``commands.cache`` と  ``icons.cache`` を削除してください。
- icebergディレクトリに ``plugins`` ディレクトリを作成してください。
- Windowsの場合、icebergディレクトリに ``config.d`` ディレクトリを作成してください。
- Windowsの場合、 ``config.lua`` の ``system`` グローバル変数に以下のように ``terminal``  を追加してください。

    .. code-block:: lua

        system = {
          (他の設定)
          terminal = "cmd /k ${1}",
          (他の設定)
        }

- ``config.lua`` の末尾に以下を追加してください。Linuxの場合、 末尾の ``load_config_d()`` は削除した上で追加してください(関数定義も削除して問題ありません)。

    .. code-block:: lua

        ibs.load_lua_files(ibs.join_path(script_path, "config.d"))
        ibs.load_plugins()


0.9.7 (2015-11-19)
-----------------------
- NEW: Linux上での動作をサポートした。
- NEW: アイコン画像にsvgを利用できるようになった。
- CHANGED: fltkのバージョンを1.3.3にアップグレード。
- CHANGED: 鬼車のバージョンを5.9.6にアップグレード。

0.9.6からのアップグレード方法
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- 0.9.7をダウンロードしてください。
- 0.9.7から ``iceberg.exe`` をコピーして既存の0.9.6側に上書きしてください。
- ``commands.cache`` と  ``icons.cache`` を削除してください。

0.9.6 (2014-10-20)
-----------------------
- FIXED: 一部のパスでアイコンが正しく読み込まれない問題を修正
- NEW: パス補完とオプション補完でオートコンプリートを有効にする ``path_autocomplete`` と ``option_autocomplete`` を追加
- FIXED: 一部のパスを読み込んだ際にクラッシュする問題を修正
- CHANGED: 使用するコンパイラをMinGW-W64 4.9.1にアップグレード
- IMPROVED: 単一キーをホットキーに割り当てられるようになった

0.9.5からのアップグレード方法
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
- 0.9.6をダウンロードしてください。
- 0.9.6から ``iceberg.exe`` をコピーして既存の0.9.5側に上書きしてください。
- アイコン周りの修正が入っているので ``icons.cache`` を削除することをお勧めします。
- ``config.lua`` の ``system`` グローバル変数に以下のように ``path_autocomplete`` と ``option_autocomplete`` を追加してください。

    .. code-block:: lua

        system = {
          (他の設定)
          -- パス補完時にオートコンプリートを行うか
          path_autocomplete = true,
          -- オプション補完時にオートコンプリートを行うか
          option_autocomplete = true,
          (他の設定)
        }


0.9.5 (2014-03-04)
-----------------------
- FIXED: 補完候補ウインドウに ``'&'`` が描画できなかった問題を修正
- FIXED: 一部のキー組み合わせで動作しなかった問題を修正( ``shift-tab`` など )

0.9.4 (2013-11-05)
-----------------------
- NEW: 補完候補の属性に ``always_match`` を追加
- NEW: ``icebergsupport.selected_index`` , ``icebergsupport.brshift`` , ``icebergsupport.blshift`` を追加

0.9.3 (2013-11-01)
-----------------------
- FIXED: shift+矢印キーが動作しない問題を修正
- FIEED: 特定のアイテム上でコンテキストメニューを開こうとした時に落ちる問題を修正
- FIXED: 相対パスを絶対パスに変換する処理のバグを修正
- FIXED: ``server_port`` オプションを有効にしている状態で再起動に失敗する問題を修正
- CHANGED: コマンドの設定で ``history = false`` としてる場合もコマンド名のみのヒストリは取得するように変更
- IMPROVED: モーダルウインドウの扱いを改善(Enterキー押下でクローズできるようになった)

0.9.2 (2013-09-07)
-----------------------
- FIXED: スレッドセーフではない関数がマルチスレッド下で保護されていない問題を修正
- IMPROVED: コマンド定義でも補完関数を定義できるようになった
- NEW: ``icebergsupport.get_clipboard_histories`` を追加
    - 関連して ``system.max_clipboard_histories`` 設定値を追加
- NEW: ``clipboard`` コマンドをデフォルトコマンドに追加
- NEW: ``icebergsupport.add_history`` を追加
- NEW: ``-m activate`` 起動オプションを追加

0.9.1 (2013-08-24)
-----------------------
- FIXED: ショートカット実行時、入力欄に ``"`` で囲われた空白を含む値が合った場合の問題を修正
- FIXED: 補完関数がテーブルを返した場合、補完リストが入力に従いフィルタリングされなかった問題を修正
- IMPROVED: 相対パスの扱いを改善
- IMPROVED: 補完関数でjpegファイルをアイコンに使用できるようにした
- IMPROVED: アイコンキャッシュをより効率的な実装にした
- NEW: ``icebergsupport.unquote_path`` を追加
- NEW: ``alttab`` コマンドをデフォルトコマンドに追加

0.9.0 (2013-08-15)
-----------------------
- 公開開始
