変更履歴
=======================
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