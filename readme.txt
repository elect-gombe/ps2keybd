PIC32MX150F128B/PIC32MX250F128B用テキストエディタ　by K.Tanaka
Gombeによりちらつき防止コードを挿入

PS/2キーボード、SDカード対応

詳細はこちら
http://www.ze.em-net.ne.jp/~kenken/texteditor/index.html

＜同梱ファイル＞
texteditor.c
　テキストエディター本体

keyinput.c
　キー入力やカーソル点滅関連部品

keyinput.h
　keyinput.c用ヘッダーファイル

ps2keyboard.X.a
　PS/2キーボードを使用するためのライブラリ

ps2keyboard.h
　ps2keyboard.X.a用ヘッダーファイル

lib_colortext32.a
　カラービデオ出力するためのライブラリ

colortext32.h
　lib_colortext32.a用ヘッダーファイル

libsdfsio.a
　SDカードにアクセスするためのライブラリ

SDFSIO.h
　libsdfsio.a用ヘッダーファイル

App_32MX250F128B.ld
　SDカードブートローダAP用リンカースクリプト

textedit.hex
　SDカードブートローダ用にビルド済みのHEXファイル
