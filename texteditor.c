// texteditor.c
// Text Editor with PS/2 Keyboard for PIC32MX150F128B / PIC32MX250F128B by K.Tanaka

// 利用システム
// ps2keyboard.a : PS/2キーボード入力システムライブラリ
// lib_colortext32.a : カラービデオ信号出力システムライブラリ（30×27テキスト版）
// libsdfsio.a ： SDカードアクセス用ライブラリ

#include <plib.h>
#include "colortext32.h"
#include "ps2keyboard.h"
#include "keyinput.h"
#include "SDFSIO.h"

//外付けクリスタル with PLL (16倍)
#pragma config PMDL1WAY = OFF, IOL1WAY = OFF
#pragma config FPLLIDIV = DIV_1, FPLLMUL = MUL_16, FPLLODIV = DIV_1
#pragma config FNOSC = PRIPLL, FSOSCEN = OFF, POSCMOD = XT, OSCIOFNC = OFF
#pragma config FPBDIV = DIV_1, FWDTEN = OFF, JTAGEN = OFF, ICESEL = ICS_PGx1

#define TBUFMAXLINE 101 //テキストバッファ数
#define TBUFSIZE 200 //テキストバッファ1つのサイズ
#define TBUFMAXSIZE (TBUFSIZE*(TBUFMAXLINE-1)) //最大バッファ容量（バッファ1行分空ける）
#define EDITWIDTHX 30 //エディタ画面横幅
#define EDITWIDTHY 26 //エディタ画面縦幅
#define COLOR_NORMALTEXT 7 //通常テキスト色
#define COLOR_ERRORTEXT 4 //エラーメッセージテキスト色
#define COLOR_AREASELECTTEXT 4 //範囲選択テキスト色
#define COLOR_BOTTOMLINE 5 //画面最下行の色
#define FILEBUFSIZE 256 //ファイルアクセス用バッファサイズ
#define MAXFILENUM 50 //利用可能ファイル最大数

#define ERR_FILETOOBIG -1
#define ERR_CANTFILEOPEN -2
#define ERR_CANTWRITEFILE -3


struct _TBUF{
//リンク付きのテキストバッファ
	struct _TBUF *prev;//前方へのリンク。NULLの場合先頭または空き
	struct _TBUF *next;//後方へのリンク。NULLの場合最後
	unsigned short n;//現在の使用バイト数
	unsigned char Buf[TBUFSIZE];//バッファ
} ;
typedef struct _TBUF _tbuf;

_tbuf TextBuffer[TBUFMAXLINE]; //テキストバッファの実体
_tbuf *TBufstart; //テキストバッファの先頭位置
_tbuf *cursorbp; //現在のカーソル位置のテキストバッファ
unsigned short cursorix; //現在のカーソル位置のテキストバッファ先頭からの位置
_tbuf *disptopbp; //現在表示中画面左上のテキストバッファ
unsigned short disptopix; //現在表示中画面左上のテキストバッファ先頭からの位置
int num; //現在バッファ内に格納されている文字数

int cx,cy; //カーソル座標
int cx2; //上下移動時の仮カーソルX座標

_tbuf *cursorbp1; //範囲選択時のカーソルスタート位置のテキストバッファ、範囲選択モードでない場合NULL
unsigned short cursorix1; //範囲選択時のカーソルスタート位置のテキストバッファ先頭からの位置
int cx1,cy1; //範囲選択時のカーソルスタート座標

// カーソル関連位置の一時避難用
_tbuf *cursorbp_t;
unsigned short cursorix_t;
_tbuf *disptopbp_t;
unsigned short disptopix_t;
int cx_t,cy_t;
unsigned char clipboard[EDITWIDTHX*EDITWIDTHY]; //クリップボード、最大サイズは編集画面領域と同じ
int clipsize; //現在クリップボードに格納されている文字数

int edited; //保存後に変更されたかを表すフラグ

unsigned char filebuf[FILEBUFSIZE]; //ファイルアクセス用バッファ
unsigned char currentfile[8+1+3+1],tempfile[8+1+3+1]; //編集中のファイル名、一時ファイル名
unsigned char filenames[MAXFILENUM][13];

void wait60thsec(unsigned short n){
	// 60分のn秒ウェイト（ビデオ画面の最下行信号出力終了まで待つ）
	drawcount=0;
	while(drawcount<n) asm("wait");
}

_tbuf * newTBuf(_tbuf *prev){
// 新しいテキストバッファ1行を生成
// prev:挿入先の行（prevの後ろに追加）
// 戻り値　生成したバッファへのポインタ、生成できない場合NULL
	_tbuf *bp,*next;

	//バッファの先頭から空きをサーチ
	bp=TextBuffer;
	while(1){
		if(bp->prev==NULL && bp!=TBufstart) break;
		bp++;
		if(bp>=TextBuffer+TBUFMAXLINE) return NULL;//最後まで空きなし
	}
	next=prev->next;
	//行挿入
	bp->prev=prev;
	bp->next=next;
	prev->next=bp;
	if(next!=NULL) next->prev=bp;
	bp->n=0;
	return bp;
}

_tbuf * deleteTBuf(_tbuf *bp){
// テキストバッファの削除
// bp:削除する行のポインタ
// 戻り値　削除前の次のバッファへのポインタ、ない場合NULL
	unsigned short a,b;
	_tbuf *prev,*next;
	prev=bp->prev;
	next=bp->next;
	if(prev==NULL){
		//先頭行の場合
		if(next==NULL) return next; //最後の1行の場合は削除しない
		TBufstart=next; //次の行を先頭行設定
	}
	else prev->next=next; //前を次にリンク（最終行ならNULLがコピーされる）
	if(next!=NULL) next->prev=prev; //次があれば次を前にリンク
	bp->prev=NULL; //空きフラグ設定
	return next;
}

int insertchar(_tbuf *bp,unsigned int ix,unsigned char c){
//テキストバッファbpの先頭からixバイトの位置にcを挿入
//戻り値　成功：0、不正または容量オーバー：-1、空きがあるはずなのに失敗：1
	unsigned char *p;

	if(ix > bp->n) return -1; //不正指定
	if(num >= TBUFMAXSIZE) return -1; //バッファ容量オーバー
	if(bp->n < TBUFSIZE){
		//ライン内だけで1バイト挿入可能//
		for(p=bp->Buf + bp->n ; p > bp->Buf+ix ; p--) *p=*(p-1);
		*p=c;
		bp->n++;
		num++; //バッファ使用量
//		if(bp->n >= TBUFSIZE && bp->next==NULL) newTBuf(bp); //バッファがいっぱいになったら新たにバッファ生成
		return 0;
	}
	//ラインがあふれる場合
	if(bp->next==NULL || bp->next->n >=TBUFSIZE){
		// 最終行または次のラインバッファがいっぱいだったら一行挿入
		if(newTBuf(bp)==NULL){
			// ラインバッファ挿入不可
			return 1;
		}
	}
	if(ix==TBUFSIZE){
		insertchar(bp->next,0,c);
		return 0;
	}
	p=bp->Buf + TBUFSIZE-1;
	insertchar(bp->next,0,*p); //次の行の先頭に1文字挿入（必ず空きあり）
	for( ; p > bp->Buf+ix ; p--) *p=*(p-1);
	*p=c;
	return 0;
}

int overwritechar(_tbuf *bp,unsigned int ix,unsigned char c){
//テキストバッファbpの先頭からixバイトの位置をcで上書き
//戻り値　成功：0、不正または容量オーバー：-1、空きがあるはずなのに失敗：1

	//現在のバッファ位置の文字が終端または改行の場合、挿入モード
	if(ix > bp->n) return -1; //不正指定
	while(ix >= bp->n){
		if(bp->next==NULL){
			//テキスト全体最後尾の場合は挿入
			return insertchar(bp,ix,c);
		}
		bp=bp->next;
		ix=0;
	}
	if(bp->Buf[ix]=='\n') return insertchar(bp,ix,c);
	else bp->Buf[ix]=c;
	return 0;
}

void deletechar(_tbuf *bp,unsigned int ix){
//テキストバッファbpの先頭からkバイトの位置の1バイト削除
	unsigned char *p;

	if(ix > bp->n) return; //不正指定
	if(ix !=bp->n){
		//バッファの最後の文字より後ろでない場合
		for(p=bp->Buf+ix ; p< bp->Buf + bp->n-1 ; p++) *p=*(p+1);
		bp->n--;
		num--; //バッファ使用量
		return;
	}
	//行バッファの現在の最後の場合（削除する文字がない場合）
	if(bp->next==NULL) return; //全体の最後の場合、何もしない
	deletechar(bp->next,0); //次の行の先頭文字を削除
}
int gabagecollect1(void){
//断片化されたテキストバッファの隙間を埋めるガベージコレクション
//カーソルの前と後ろそれぞれ探索して最初の1バイト分のみ実施
//戻り値 1バイトでも移動した場合：1、なかった場合：0

	_tbuf *bp;
	int f=0;
	unsigned char *p,*p2;

	//カーソルがバッファの先頭にある場合、前のバッファの最後尾に変更
	//（ただし前に空きがない場合と先頭バッファの場合を除く）
	while(cursorix==0 && cursorbp->prev!=NULL && cursorbp->prev->n <TBUFSIZE){
		cursorbp=cursorbp->prev;
		cursorix=cursorbp->n;
	}
	//画面左上位置がバッファの先頭にある場合、前のバッファの最後尾に変更
	//（ただし先頭バッファの場合を除く）
	while(disptopix==0 && disptopbp->prev!=NULL){
		disptopbp=disptopbp->prev;
		disptopix=disptopbp->n;
	}
	//カーソルのあるバッファ以外の空バッファを全て削除
	bp=TBufstart;
	while(bp!=NULL){
		if(bp->n == 0 && bp!=cursorbp) bp=deleteTBuf(bp); //空きバッファ削除
		else bp=bp->next;
	}

	//カーソル位置より前の埋まっていないバッファを先頭からサーチ
	bp=TBufstart;
	while(bp->n >= TBUFSIZE){
		if(bp==cursorbp) break;
		bp=bp->next;
	}
	if(bp!=cursorbp){
		//最初に見つけた空き場所に次のバッファから1バイト移動
		bp->Buf[bp->n++] = bp->next->Buf[0];
		bp=bp->next;
		p=bp->Buf;
		p2=p+bp->n-1;
		for( ; p<p2 ; p++) *p=*(p+1);
		bp->n--;
		f=1;
		if(bp == disptopbp) disptopix--;
		if(bp == cursorbp) cursorix--;
//		else if(bp->n == 0) deleteTBuf(bp);
	}
	if(cursorbp->next ==NULL) return f; //カーソル位置が最終バッファなら終了
	//カーソル位置の次のバッファから埋まっていないバッファをサーチ
	bp=cursorbp;
	do{
		bp=bp->next;
		if(bp->next ==NULL) return f; //最終バッファに到達なら終了
	} while(bp->n >=TBUFSIZE);

	//最初に見つけた空き場所に次のバッファから1バイト移動
	bp->Buf[bp->n++] = bp->next->Buf[0];
	bp=bp->next;
	p=bp->Buf;
	p2=p+bp->n-1;
	for( ; p<p2 ; p++) *p=*(p+1);
	bp->n--;
	f=1;
	if(bp->n == 0) deleteTBuf(bp);
	return f;
}
void gabagecollect2(void){
// 変化がなくなるまで1バイト分のガベージコレクションを呼び出し
	while(gabagecollect1()) ;
}
void inittextbuf(void){
// テキストバッファの初期化
	_tbuf *bp;
	for(bp=TextBuffer;bp<TextBuffer+TBUFMAXLINE;bp++) bp->prev=NULL; //未使用バッファ化
	TBufstart=TextBuffer; //リンクの先頭設定
	TBufstart->next=NULL;
	TBufstart->n=0;
	num=0; //バッファ使用量
	edited=0; //編集済みフラグクリア
}
void redraw(){
//画面の再描画
	unsigned char *vp;
	_tbuf *bp,*bp1,*bp2;
	int ix,ix1,ix2;
	int x,y;
	unsigned char ch,cl;

	vp=TVRAM;
	bp=disptopbp;
	ix=disptopix;
	cl=COLOR_NORMALTEXT;
	if(cursorbp1==NULL){
		//範囲選択モードでない場合
		bp1=NULL;
		bp2=NULL;
	}
	else{
		//範囲選択モードの場合、開始位置と終了の前後判断して
		//bp1,ix1を開始位置、bp2,ix2を終了位置に設定
		if(cy<cy1 || (cy==cy1 && cx<cx1)){
			bp1=cursorbp;
			ix1=cursorix;
			bp2=cursorbp1;
			ix2=cursorix1;
		}
		else{
			bp1=cursorbp1;
			ix1=cursorix1;
			bp2=cursorbp;
			ix2=cursorix;
		}
	}
	for(y=0;y<EDITWIDTHY;y++){
		if(bp==NULL) break;
		for(x=0;x<EDITWIDTHX;x++){
			//文字がある位置までサーチ
			while(ix>=bp->n){
				if(bp==bp1 && ix==ix1) cl=COLOR_AREASELECTTEXT;
				if(bp==bp2 && ix==ix2) cl=COLOR_NORMALTEXT;
				bp=bp->next;
				ix=0;
				if(bp==NULL) break;
			}
			if(bp==NULL) break; //バッファ最終
			if(bp==bp1 && ix==ix1) cl=COLOR_AREASELECTTEXT;
			if(bp==bp2 && ix==ix2) cl=COLOR_NORMALTEXT;
			ch=bp->Buf[ix++];
			if(ch=='\n') break;
			*(vp+ATTROFFSET)=cl;
			*vp++=ch;
		}
		//改行およびバッファ最終以降の右側表示消去
		for(;x<EDITWIDTHX;x++){
			*(vp+ATTROFFSET)=0;
			*vp++=0;
		}
	}
	//バッファ最終以降の下側表示消去
	for(;y<EDITWIDTHY;y++){
		for(x=0;x<EDITWIDTHX;x++){
			*(vp+ATTROFFSET)=0;
			*vp++=0;
		}
	}
}

void cursor_left(void){
//カーソルを1つ前に移動
//出力：下記変数を移動先の値に変更
//cursorbp,cursorix バッファ上のカーソル位置
//cx,cy 画面上のカーソル位置
//cx2 cxと同じ
//disptopbp,disptopix 画面左上のバッファ上の位置

	_tbuf *bp;
	int ix;
	int i;
	int x;

	//バッファ上のカーソル位置を1つ前に移動
	if(cursorix!=0) cursorix--;
	else while(1) {
		//1つ前のバッファの最後尾に移動、ただし空バッファは飛ばす
		if(cursorbp->prev==NULL) return; //テキスト全体先頭なので移動しない
		cursorbp=cursorbp->prev;
		if(cursorbp->n >0){
			cursorix=cursorbp->n-1;//バッファ最後尾
			break;
		}
	}

	//カーソルおよび画面左上位置の更新
	if(cx>0){
		//左端でなければカーソルを単純に1つ左に移動して終了
		cx--;
		cx2=cx;
		return;
	}
	if(cy>0){
		//左端だが上端ではない場合
		if(cursorbp->Buf[cursorix]!='\n'){
			// 移動先が改行コードでない場合、カーソルは1つ上の行の右端に移動
			cx=EDITWIDTHX-1;
			cx2=cx;
			cy--;
			return;
		}
		//画面左上位置から最後尾のX座標をサーチ
		bp=disptopbp;
		ix=disptopix;
		x=0;
		while(ix!=cursorix || bp!=cursorbp){
			if(bp->n==0){
				//空バッファの場合次へ
				bp=bp->next;
				ix=0;
				continue;
			}
			if(bp->Buf[ix++]=='\n' || x>=EDITWIDTHX-1) x=0;
			else x++;
			if(ix >= bp->n){
				bp=bp->next;
				ix=0;
			}
		}
		cx=x;
		cx2=cx;
		cy--;
		return;
	}

	//左端かつ上端の場合
	if(cursorbp->Buf[cursorix]!='\n'){
		// 移動先が改行コードでない場合、カーソルは右端に移動
		// 画面左上位置は画面横幅分前に移動
		cx=EDITWIDTHX-1;
		cx2=cx;
	}
	else{
		//移動先が改行コードの場合
		//行頭（改行の次の文字またはバッファ先頭）と現在位置の文字数差を
		//画面横幅で割った余りがカーソルX座標
		bp=cursorbp;
		ix=cursorix;
		i=0;
		while(1){
			if(ix==0){
				if(bp->prev==NULL) break;
				bp=bp->prev;
				ix=bp->n;
				continue;
			}
			ix--;
			if(bp->Buf[ix]=='\n') break;
			i++;
		}
		cx=i % EDITWIDTHX;
		cx2=cx;
	}
	//画面左上位置は現在位置からX座標分引いたところ
	bp=cursorbp;
	ix=cursorix;
	x=cx;
	while(x>0){
		if(ix==0){
			bp=bp->prev;
			ix=bp->n;
			continue;
		}
		ix--;
		x--;
	}
	disptopbp=bp;
	disptopix=ix;
}
void cursor_right(void){
//カーソルを1つ後ろに移動
//出力：下記変数を移動先の値に変更
//cursorbp,cursorix バッファ上のカーソル位置
//cx,cy 画面上のカーソル位置
//cx2 cxと同じ
//disptopbp,disptopix 画面左上のバッファ上の位置

	_tbuf *bp;
	int ix;
	int i;
	int x;
	unsigned char c;

	if(cursorix >= cursorbp->n){
		//バッファ最後尾の場合、次の先頭に移動
		bp=cursorbp;
		while(1) {
			//空バッファは飛ばす
			if(bp->next==NULL) return; //テキスト全体最後尾なので移動しない
			bp=bp->next;
			if(bp->n >0) break;
		}
		cursorbp=bp;
		cursorix=0;//バッファ先頭
	}
	c=cursorbp->Buf[cursorix++]; //バッファ上のカーソル位置のコードを読んで1つ後ろに移動
	if(c!='\n' && cx<EDITWIDTHX-1){
		//カーソル位置が改行でも右端でもない場合単純に1つ右に移動して終了
		cx++;
		cx2=cx;
		return;
	}
	cx=0; //カーソルを右端に移動
	cx2=cx;
	if(cy<EDITWIDTHY-1){
		//下端でなければカーソルを次行に移動して終了
		cy++;
		return;
	}
	//下端の場合
	//画面左上位置を更新
	//改行コードまたは画面横幅超えるまでサーチ
	bp=disptopbp;
	ix=disptopix;
	x=0;
	while(x<EDITWIDTHX){
		if(ix >= bp->n){
			bp=bp->next;
			ix=0;
			continue;
		}
		if(bp->Buf[ix++]=='\n') break;
		x++;
	}
	disptopbp=bp;
	disptopix=ix;
}
void cursor_up(void){
//カーソルを1つ上に移動
//出力：下記変数を移動先の値に変更
//cursorbp,cursorix バッファ上のカーソル位置
//cx,cy 画面上のカーソル位置
//cx2 移動前のcxと同じ
//disptopbp,disptopix 画面左上のバッファ上の位置

	_tbuf *bp;
	int ix;
	int i;
	int x;
	unsigned char c;

	//画面幅分前に戻ったところがバッファ上カーソルの移動先
	//途中で改行コードがあれば別の手段で検索
	bp=cursorbp;
	ix=cursorix;
	i=cx2-cx;
	while(i<EDITWIDTHX){
		if(ix==0){
			if(bp->prev==NULL) return; //バッファ先頭までサーチしたら移動なし
			bp=bp->prev;
			ix=bp->n;
			continue;
		}
		ix--;
		if(bp->Buf[ix]=='\n') break;
		i++;
	}
	cursorbp=bp;
	cursorix=ix;
	//画面幅の間に改行コードがなかった場合
	if(i==EDITWIDTHX){
		cx=cx2;
		//画面上端でなければカーソルを1つ上に移動して終了
		if(cy>0){
			cy--;
			return;
		}
		//画面上端の場合、カーソル位置からX座標分戻ったところが画面左上位置
		x=cx;
		while(x>0){
			if(ix==0){
				bp=bp->prev;
				ix=bp->n;
				continue;
			}
			ix--;
			x--;
		}
		disptopbp=bp;
		disptopix=ix;
		return;
	}
	//改行が見つかった場合
	//行頭（改行の次の文字またはバッファ先頭）と現在位置の文字数差を
	//画面横幅で割った余りを求める
	i=0;
	while(1){
		if(ix==0){
			if(bp->prev==NULL) break;
			bp=bp->prev;
			ix=bp->n;
			continue;
		}
		ix--;
		if(bp->Buf[ix]=='\n') break;
		i++;
	}
	x=i % EDITWIDTHX; //改行ブロックの最終行の右端
	bp=cursorbp;
	ix=cursorix;
	//バッファ上のカーソル位置は改行ブロックの最終行右端からカーソルX座標分戻る
	//最終行右端のほうが小さい場合、その場所をバッファ上のカーソル位置とする
	while(x>cx2){
		if(ix==0){
			bp=bp->prev;
			ix=bp->n;
			continue;
		}
		ix--;
		x--;
	}
	cursorbp=bp;
	cursorix=ix;
	cx=x; //cx2または改行ブロック最終行右端
	if(cy>0){
		//画面上端でなければカーソルを1つ上に移動して終了
		cy--;
		return;
	}
	//画面上端の場合
	//画面左上位置は現在位置からX座標分引いたところ
	while(x>0){
		if(ix==0){
			bp=bp->prev;
			ix=bp->n;
			continue;
		}
		ix--;
		x--;
	}
	disptopbp=bp;
	disptopix=ix;
}
void cursor_down(void){
//カーソルを1つ下に移動
//出力：下記変数を移動先の値に変更
//cursorbp,cursorix バッファ上のカーソル位置
//cx,cy 画面上のカーソル位置
//cx2 移動前のcxと同じ
//disptopbp,disptopix 画面左上のバッファ上の位置

	_tbuf *bp;
	int ix;
	int x;
	unsigned char c;

	//次行の先頭サーチ
	//カーソル位置から画面右端までの間に改行コードがあれば次の文字が先頭
	bp=cursorbp;
	ix=cursorix;
	x=cx;
	while(x<EDITWIDTHX){
		if(ix>=bp->n){
			if(bp->next==NULL) return; //バッファ最後までサーチしたら移動なし
			bp=bp->next;
			ix=0;
			continue;
		}
		c=bp->Buf[ix];
		ix++;
		x++;
		if(c=='\n') break;
	}
	//次行先頭からcx2文字数分後ろにサーチ
	x=0;
	while(x<cx2){
		if(ix>=bp->n){
			if(bp->next==NULL) break; //バッファ最後の場合そこに移動
			bp=bp->next;
			ix=0;
			continue;
		}
		if(bp->Buf[ix]=='\n') break; //改行コードの場合そこに移動
		ix++;
		x++;
	}
	cursorbp=bp;
	cursorix=ix;
	cx=x;
	//画面下端でなければカーソルを1つ下に移動して終了
	if(cy<EDITWIDTHY-1){
		cy++;
		return;
	}
	//下端の場合
	//画面左上位置を更新
	//改行コードまたは画面横幅超えるまでサーチ
	bp=disptopbp;
	ix=disptopix;
	x=0;
	while(x<EDITWIDTHX){
		if(ix >= bp->n){
			bp=bp->next;
			ix=0;
			continue;
		}
		if(bp->Buf[ix++]=='\n') break;
		x++;
	}
	disptopbp=bp;
	disptopix=ix;
}
void cursor_home(void){
//カーソルを行先頭に移動
//出力：下記変数を移動先の値に変更
//cursorbp,cursorix バッファ上のカーソル位置
//cx,cx2 0
//cy 変更なし
//disptopbp,disptopix 画面左上のバッファ上の位置（変更なし）

	//カーソルX座標分前に移動
	while(cx>0){
		if(cursorix==0){
			//空バッファは飛ばす
			cursorbp=cursorbp->prev;
			cursorix=cursorbp->n;
			continue;
		}
		cursorix--;
		cx--;
	}
	cx2=0;
}
void cursor_end(void){
//カーソルを行末に移動
//出力：下記変数を移動先の値に変更
//cursorbp,cursorix バッファ上のカーソル位置
//cx,cx2 行末
//cy 変更なし
//disptopbp,disptopix 画面左上のバッファ上の位置（変更なし）

	//カーソルX座標を画面幅分後ろに移動
	//改行コードまたはバッファ最終があればそこに移動
	while(cx<EDITWIDTHX-1){
		if(cursorix>=cursorbp->n){
			//空バッファは飛ばす
			if(cursorbp->next==NULL) break;
			cursorbp=cursorbp->next;
			cursorix=0;
			continue;
		}
		if(cursorbp->Buf[cursorix]=='\n') break;
		cursorix++;
		cx++;
	}
	cx2=cx;
}
void cursor_pageup(void){
//PageUpキー
//最上行が最下行になるまでスクロール
//出力：下記変数を移動先の値に変更
//cursorbp,cursorix バッファ上のカーソル位置
//cx,cx2
//cy
//disptopbp,disptopix 画面左上のバッファ上の位置

	_tbuf *bp;
	int ix;
	int i;
	int cy_old;

	cy_old=cy;
	while(cy>0) cursor_up(); // cy==0になるまでカーソルを上に移動
	for(i=0;i<EDITWIDTHY-1;i++){
		//画面行数-1行分カーソルを上に移動
		bp=disptopbp;
		ix=disptopix;
		cursor_up();
		if(bp==disptopbp && ix==disptopix) break; //最上行で移動できなかった場合抜ける
	}
	//元のY座標までカーソルを下に移動、1行も動かなかった場合は最上行に留まる
	if(i>0) while(cy<cy_old) cursor_down();
}
void cursor_pagedown(void){
//PageDownキー
//最下行が最上行になるまでスクロール
//出力：下記変数を移動先の値に変更
//cursorbp,cursorix バッファ上のカーソル位置
//cx,cx2
//cy
//disptopbp,disptopix 画面左上のバッファ上の位置

	_tbuf *bp;
	int ix;
	int i;
	int y;
	int cy_old;

	cy_old=cy;
	while(cy<EDITWIDTHY-1){
		// cy==EDITWIDTH-1になるまでカーソルを下に移動
		y=cy;
		cursor_down();
		if(y==cy) break;// バッファ最下行で移動できなかった場合抜ける
	}
	for(i=0;i<EDITWIDTHY-1;i++){
		//画面行数-1行分カーソルを下に移動
		bp=disptopbp;
		ix=disptopix;
		cursor_down();
		if(bp==disptopbp && ix==disptopix) break; //最下行で移動できなかった場合抜ける
	}
	//下端からさらに移動した行数分、カーソルを上に移動、1行も動かなかった場合は最下行に留まる
	if(i>0) while(cy>cy_old) cursor_up();
}
void cursor_top(void){
//カーソルをテキストバッファの先頭に移動
	cursorbp=TBufstart;
	cursorix=0;
	cursorbp1=NULL; //範囲選択モード解除
	disptopbp=cursorbp;
	disptopix=cursorix;
	cx=0;
	cx2=0;
	cy=0;
}

int countarea(void){
//テキストバッファの指定範囲の文字数をカウント
//範囲は(cursorbp,cursorix)と(cursorbp1,cursorix1)で指定
//後ろ側の一つ前の文字までをカウント
	_tbuf *bp1,*bp2;
	int ix1,ix2;
	int n;

	//範囲選択モードの場合、開始位置と終了の前後判断して
	//bp1,ix1を開始位置、bp2,ix2を終了位置に設定
	if(cy<cy1 || (cy==cy1 && cx<cx1)){
		bp1=cursorbp;
		ix1=cursorix;
		bp2=cursorbp1;
		ix2=cursorix1;
	}
	else{
		bp1=cursorbp1;
		ix1=cursorix1;
		bp2=cursorbp;
		ix2=cursorix;
	}
	n=0;
	while(1){
		if(bp1==bp2 && ix1==ix2) return n;
		if(ix1 < bp1->n){
			n++;
			ix1++;
		}
		else{
			bp1=bp1->next;
			ix1=0;
		}
	}
}
void deletearea(void){
//テキストバッファの指定範囲を削除
//範囲は(cursorbp,cursorix)と(cursorbp1,cursorix1)で指定
//後ろ側の一つ前の文字までを削除
//削除後のカーソル位置は選択範囲の先頭にし、範囲選択モード解除する

	_tbuf *bp;
	int ix;
	int n;

	n=countarea(); //選択範囲の文字数カウント

	//範囲選択の開始位置と終了位置の前後を判断してカーソルを開始位置に設定
	if(cy>cy1 || (cy==cy1 && cx>cx1)){
		cursorbp=cursorbp1;
		cursorix=cursorix1;
		cx=cx1;
		cy=cy1;
	}
	cx2=cx;
	cursorbp1=NULL; //範囲選択モード解除

	//bp,ixを開始位置に設定
	bp=cursorbp;
	ix=cursorix;

	//選択範囲が最初のバッファの最後まである場合
	if(n>=(bp->n - ix)){
		n -= bp->n - ix; //削除文字数減
		num-=bp->n - ix; //バッファ使用量を減数
		bp->n=ix; //ix以降を削除
		bp=bp->next;
		if(bp==NULL) return;
		ix=0;
	}
	//次のバッファ以降、選択範囲の終了位置が含まれないバッファは削除
	while(n>=bp->n){
		n-=bp->n; //削除文字数減
		num-=bp->n; //バッファ使用量を減数
		bp=deleteTBuf(bp); //バッファ削除して次のバッファに進む
		if(bp==NULL) return;
	}
	//選択範囲の終了位置を含む場合、1文字ずつ削除
	while(n>0){
		deletechar(bp,ix); //バッファから1文字削除（numは関数内で1減される）
		n--;
	}
}
void clipcopy(void){
// 選択範囲をクリップボードにコピー
	_tbuf *bp1,*bp2;
	int ix1,ix2;
	char *ps,*pd;

	//範囲選択モードの場合、開始位置と終了の前後判断して
	//bp1,ix1を開始位置、bp2,ix2を終了位置に設定
	if(cy<cy1 || (cy==cy1 && cx<cx1)){
		bp1=cursorbp;
		ix1=cursorix;
		bp2=cursorbp1;
		ix2=cursorix1;
	}
	else{
		bp1=cursorbp1;
		ix1=cursorix1;
		bp2=cursorbp;
		ix2=cursorix;
	}
	ps=bp1->Buf+ix1;
	pd=clipboard;
	clipsize=0;
	while(bp1!=bp2 || ix1!=ix2){
		if(ix1 < bp1->n){
			*pd++=*ps++;
			clipsize++;
			ix1++;
		}
		else{
			bp1=bp1->next;
			ps=bp1->Buf;
			ix1=0;
		}
	}
}
void clippaste(void){
// クリップボードから貼り付け
	int n,i;
	unsigned char *p;

	p=clipboard;
	for(n=clipsize;n>0;n--){
		i=insertchar(cursorbp,cursorix,*p);
		if(i>0){
			//バッファ空きがあるのに挿入失敗の場合
			gabagecollect2(); //全体ガベージコレクション
			i=insertchar(cursorbp,cursorix,*p);//テキストバッファに１文字挿入
		}
		if(i!=0) return;//挿入失敗
		cursor_right();//画面上、バッファ上のカーソル位置を1つ後ろに移動
		p++;
	}
}
void set_areamode(){
//範囲選択モード開始時のカーソル開始位置グローバル変数設定
	cursorbp1=cursorbp;
	cursorix1=cursorix;
	cx1=cx;
	cy1=cy;
}
void save_cursor(void){
//カーソル関連グローバル変数を一時避難
	cursorbp_t=cursorbp;
	cursorix_t=cursorix;
	disptopbp_t=disptopbp;
	disptopix_t=disptopix;
	cx_t=cx;
	cy_t=cy;
}
void restore_cursor(void){
//カーソル関連グローバル変数を一時避難場所から戻す
	cursorbp=cursorbp_t;
	cursorix=cursorix_t;
	disptopbp=disptopbp_t;
	disptopix=disptopix_t;
	cx=cx_t;
	cy=cy_t;
}

int savetextfile(char *filename){
// テキストバッファをテキストファイルに書き込み
// 書き込み成功で0、失敗でエラーコード（負数）を返す
	FSFILE *fp;
	_tbuf *bp;
	int ix,n,i,er;
	unsigned char *ps,*pd;
	er=0;//エラーコード
	i=-1;
	fp=FSfopen(filename,"WRITE");
	if(fp==NULL) return ERR_CANTFILEOPEN;
	bp=TBufstart;
	ix=0;
	ps=bp->Buf;
	do{
		pd=filebuf;
		n=0;
		while(n<FILEBUFSIZE-1){
		//改行コードが2バイトになることを考慮してバッファサイズ-1までとする
			while(ix>=bp->n){
				bp=bp->next;
				if(bp==NULL){
					break;
				}
				ix=0;
				ps=bp->Buf;
			}
			if(bp==NULL) break;
			if(*ps=='\n'){
				*pd++='\r'; //改行コード0A→0D 0Aにする
				n++;
			}
			*pd++=*ps++;
			ix++;
			n++;
		}
		if(n>0) i=FSfwrite(filebuf,1,n,fp);
		if(i!=n) er=ERR_CANTWRITEFILE;
	} while(bp!=NULL && er==0);
	FSfclose(fp);
	return er;
}
int loadtextfile(char *filename){
// テキストファイルをテキストバッファに読み込み
// 読み込み成功で0、失敗でエラーコード（負数）を返す
	FSFILE *fp;
	_tbuf *bp;
	int ix,n,i,er;
	unsigned char *ps,*pd;
	er=0;//エラーコード
	fp=FSfopen(filename,"READ");
	if(fp==NULL) return ERR_CANTFILEOPEN;
	inittextbuf();
	bp=TextBuffer;
	ix=0;
	pd=bp->Buf;
	do{
		n=FSfread(filebuf,1,FILEBUFSIZE,fp);
		ps=filebuf;
		for(i=0;i<n;i++){
			if(ix>=TBUFSIZE){
				bp->n=TBUFSIZE;
				bp=newTBuf(bp);
				if(bp==NULL){
					er=ERR_FILETOOBIG;
					break;
				}
				ix=0;
				pd=bp->Buf;
			}
			if(*ps=='\r') ps++; //改行コード0D 0A→0Aにする（単純に0D無視）
			else{
				*pd++=*ps++;
				ix++;
				num++;//バッファ総文字数
				if(num>TBUFMAXSIZE){
					er=ERR_FILETOOBIG;
					break;
				}
			}
		}
	} while(n==FILEBUFSIZE && er==0);
	if(bp!=NULL) bp->n=ix;//最後のバッファの文字数
	FSfclose(fp);
	if(er) inittextbuf();//エラー発生の場合バッファクリア
	return er;
}
void save_as(void){
// 現在のテキストバッファの内容をファイル名を付けてSDカードに保存
// ファイル名はグローバル変数currentfile[]
// ファイル名はキーボードから変更可能
// 成功した場合currentfileを更新

	int er;
	unsigned char vk;
	unsigned char *ps,*pd;
	cls();
	setcursor(0,0,COLOR_NORMALTEXT);
	printstr("Save To SD Card\n");

	//currentfileからtempfileにコピー
	ps=currentfile;
	pd=tempfile;
	while(*ps!=0) *pd++=*ps++;
	*pd=0;

	while(1){
		printstr("File Name + [Enter] / [ESC]\n");
		if(lineinput(tempfile,8+1+3)<0) return; //ESCキーが押された
		if(tempfile[0]==0) continue; //NULL文字列の場合
		printstr("Writing...\n");
		er=savetextfile(tempfile); //ファイル保存、er:エラーコード
		if(er==0){
			printstr("OK");
			//tempfileからcurrentfileにコピーして終了
			ps=tempfile;
			pd=currentfile;
			while(*ps!=0) *pd++=*ps++;
			*pd=0;
			edited=0; //編集済みフラグクリア
			wait60thsec(60);//1秒待ち
			return;
		}
		setcursorcolor(COLOR_ERRORTEXT);
		if(er==ERR_CANTFILEOPEN) printstr("Bad File Name or File Error\n");
		else printstr("Cannot Write\n");
		setcursorcolor(COLOR_NORMALTEXT);
		printstr("Retry:[Enter] / Quit:[ESC]\n");
		while(1){
			inputchar(); //1文字入力待ち
			vk=vkey & 0xff;
			if(vk==VK_RETURN || vk==VK_SEPARATOR) break;
			if(vk==VK_ESCAPE) return;
		}
	}
}

int selectfile(void){
// SDカードからファイルを選択して読み込み
// currenfile[]にファイル名を記憶
// 戻り値　0：読み込みを行った　-1：読み込みなし
	int filenum,i,er;
	unsigned char *ps,*pd;
	unsigned char x,y;
	unsigned char vk;
	SearchRec sr;

	//ファイルの一覧をSDカードから読み出し
	cls();
	if(edited){
		//最終保存後に編集済みの場合、保存の確認
		setcursorcolor(COLOR_NORMALTEXT);
		printstr("Save Editing File?\n");
		printstr("Save:[Enter] / Not Save:[ESC]\n");
		while(1){
			inputchar(); //1文字キー入力待ち
			vk=vkey & 0xff;
			if(vk==VK_RETURN || vk==VK_SEPARATOR){
				save_as();
				break;
			}
			else if(vk==VK_ESCAPE) break;
		}
	}
	while(1){
		if(FindFirst("*.*",ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_ARCHIVE,&sr)==0) break;
		setcursorcolor(COLOR_ERRORTEXT);
		printstr("No File Found\n");
		printstr("Retry:[Enter] / Quit:[ESC]\n");
		while(1){
			inputchar(); //1文字キー入力待ち
			vk=vkey & 0xff;
			if(vk==VK_RETURN || vk==VK_SEPARATOR) break;
			if(vk==VK_ESCAPE) return -1;
		}
	}
	filenum=0;
	do{
		//filenames[]にファイル名の一覧を読み込み
		ps=sr.filename;
		pd=filenames[filenum];
		while(*ps!=0) *pd++=*ps++;
		*pd=0;
		filenum++;
	}
	while(!FindNext(&sr) && filenum<MAXFILENUM);

	//ファイル一覧を画面に表示
	cls();
	setcursor(0,0,4);
	printstr("Select File + [Enter] / [ESC]\n");
	for(i=0;i<filenum;i++){
		x=(i&1)*15+1;
		y=i/2+1;
		setcursor(x,y,COLOR_NORMALTEXT);
		printstr(filenames[i]);
	}

	//ファイルの選択
	i=0;
	setcursor(0,1,5);
	printchar(0x1c); // Right Arrow
	cursor--;
	while(1){
		inputchar();
		vk=vkey & 0xff;
		if(vk==0) continue;
		printchar(' ');
		setcursor(0,WIDTH_Y-1,COLOR_NORMALTEXT);
		for(x=0;x<WIDTH_X-1;x++) printchar(' '); //最下行のステータス表示を消去
		switch(vk){
			case VK_UP:
			case VK_NUMPAD8:
				//上矢印キー
				if(i>=2) i-=2;
				break;
			case VK_DOWN:
			case VK_NUMPAD2:
				//下矢印キー
				if(i+2<filenum) i+=2;
				break;
			case VK_LEFT:
			case VK_NUMPAD4:
				//左矢印キー
				if(i>0) i--;
				break;
			case VK_RIGHT:
			case VK_NUMPAD6:
				//右矢印キー
				if(i+1<filenum) i++;
				break;
			case VK_RETURN: //Enterキー
			case VK_SEPARATOR: //テンキーのEnter
				//ファイル名決定。読み込んで終了
				er=loadtextfile(filenames[i]); //テキストバッファにファイル読み込み
				if(er==0){
					//currenfile[]変数にファイル名をコピーして終了
					ps=filenames[i];
					pd=currentfile;
					while(*ps!=0) *pd++=*ps++;
					*pd=0;
					return 0;
				}
				setcursor(0,WIDTH_Y-1,COLOR_ERRORTEXT);
				if(er==ERR_CANTFILEOPEN) printstr("Cannot Open File");
				else if(er=ERR_FILETOOBIG) printstr("File Too Big");
				break;
			case VK_ESCAPE:
				//ESCキー、ファイル読み込みせず終了
				return -1;
		}
		setcursor((i&1)*15,i/2+1,5);
		printchar(0x1c);// Right Arrow
		cursor--;
	}
}
void newtext(void){
// 新規テキスト作成
	unsigned char vk;
	if(edited){
		//最終保存後に編集済みの場合、保存の確認
		cls();
		setcursorcolor(COLOR_NORMALTEXT);
		printstr("Save Editing File?\n");
		printstr("Save:[Enter] / Not Save:[ESC]\n");
		while(1){
			inputchar(); //1文字キー入力待ち
			vk=vkey & 0xff;
			if(vk==VK_RETURN || vk==VK_SEPARATOR){
				save_as();
				break;
			}
			else if(vk==VK_ESCAPE) break;
		}
	}
	inittextbuf(); //テキストバッファ初期化
	cursor_top(); //カーソルをテキストバッファの先頭に設定
	currentfile[0]=0; //作業中ファイル名クリア
}
void displaybottomline(void){
//エディター画面最下行の表示
	setcursor(0,WIDTH_Y-1,COLOR_BOTTOMLINE);
	printstr("F1:LOAD F2:SAVE   F4:NEW ");
	printnum2(TBUFMAXSIZE-num,5);
}
void normal_code_process(unsigned char k){
// 通常文字入力処理
// k:入力された文字コード
	int i;

	edited=1; //編集済みフラグ
	if(insertmode || k=='\n' || cursorbp1!=NULL){ //挿入モード
		if(cursorbp1!=NULL) deletearea();//選択範囲を削除
		i=insertchar(cursorbp,cursorix,k);//テキストバッファに１文字挿入
		if(i>0){
			//バッファ空きがあるのに挿入失敗の場合
			gabagecollect2(); //全体ガベージコレクション
			i=insertchar(cursorbp,cursorix,k);//テキストバッファに１文字挿入
		}
		if(i==0) cursor_right();//画面上、バッファ上のカーソル位置を1つ後ろに移動
	}
	else{ //上書きモード
		i=overwritechar(cursorbp,cursorix,k);//テキストバッファに１文字上書き
		if(i>0){
			//バッファ空きがあるのに上書き（挿入）失敗の場合
			//（行末やバッファ最後尾では挿入）
			gabagecollect2(); //全体ガベージコレクション
			i=overwritechar(cursorbp,cursorix,k);//テキストバッファに１文字上書き
		}
		if(i==0) cursor_right();//画面上、バッファ上のカーソル位置を1つ後ろに移動
	}
}
void control_code_process(unsigned char k,unsigned char sh){
// 制御文字入力処理
// k:制御文字の仮想キーコード
// sh:シフト関連キー状態

	save_cursor(); //カーソル関連変数退避（カーソル移動できなかった場合戻すため）
	switch(k){
		case VK_LEFT:
		case VK_NUMPAD4:
			 //シフトキー押下していなければ範囲選択モード解除（NumLock＋シフト＋テンキーでも解除）
			if((sh & CHK_SHIFT)==0 || (k==VK_NUMPAD4) && (sh & CHK_NUMLK)) cursorbp1=NULL;
			else if(cursorbp1==NULL) set_areamode(); //範囲選択モードでなければ範囲選択モード開始
			if(sh & CHK_CTRL){
				//CTRL＋左矢印でHome
				cursor_home();
				break;
			}
			cursor_left();
			if(cursorbp1!=NULL && (disptopbp!=disptopbp_t || disptopix!=disptopix_t)){
				//範囲選択モードで画面スクロールがあった場合
				if(cy1<EDITWIDTHY-1) cy1++; //範囲スタート位置もスクロール
				else restore_cursor(); //カーソル位置を戻す（画面範囲外の範囲選択禁止）
			}
			break;
		case VK_RIGHT:
		case VK_NUMPAD6:
			 //シフトキー押下していなければ範囲選択モード解除（NumLock＋シフト＋テンキーでも解除）
			if((sh & CHK_SHIFT)==0 || (k==VK_NUMPAD6) && (sh & CHK_NUMLK)) cursorbp1=NULL;
			else if(cursorbp1==NULL) set_areamode(); //範囲選択モードでなければ範囲選択モード開始
			if(sh & CHK_CTRL){
				//CTRL＋右矢印でEnd
				cursor_end();
				break;
			}
			cursor_right();
			if(cursorbp1!=NULL && (disptopbp!=disptopbp_t || disptopix!=disptopix_t)){
				//範囲選択モードで画面スクロールがあった場合
				if(cy1>0) cy1--; //範囲スタート位置もスクロール
				else restore_cursor(); //カーソル位置を戻す（画面範囲外の範囲選択禁止）
			}
			break;
		case VK_UP:
		case VK_NUMPAD8:
			 //シフトキー押下していなければ範囲選択モード解除（NumLock＋シフト＋テンキーでも解除）
			if((sh & CHK_SHIFT)==0 || (k==VK_NUMPAD8) && (sh & CHK_NUMLK)) cursorbp1=NULL;
			else if(cursorbp1==NULL) set_areamode(); //範囲選択モードでなければ範囲選択モード開始
			cursor_up();
			if(cursorbp1!=NULL && (disptopbp!=disptopbp_t || disptopix!=disptopix_t)){
				//範囲選択モードで画面スクロールがあった場合
				if(cy1<EDITWIDTHY-1) cy1++; //範囲スタート位置もスクロール
				else restore_cursor(); //カーソル位置を戻す（画面範囲外の範囲選択禁止）
			}
			break;
		case VK_DOWN:
		case VK_NUMPAD2:
			 //シフトキー押下していなければ範囲選択モード解除（NumLock＋シフト＋テンキーでも解除）
			if((sh & CHK_SHIFT)==0 || (k==VK_NUMPAD2) && (sh & CHK_NUMLK)) cursorbp1=NULL;
			else if(cursorbp1==NULL) set_areamode(); //範囲選択モードでなければ範囲選択モード開始
			cursor_down();
			if(cursorbp1!=NULL && (disptopbp!=disptopbp_t || disptopix!=disptopix_t)){
				//範囲選択モードで画面スクロールがあった場合
				if(cy1>0) cy1--; //範囲スタート位置もスクロール
				else restore_cursor(); //カーソル位置を戻す（画面範囲外の範囲選択禁止）
			}
			break;
		case VK_HOME:
		case VK_NUMPAD7:
			 //シフトキー押下していなければ範囲選択モード解除（NumLock＋シフト＋テンキーでも解除）
			if((sh & CHK_SHIFT)==0 || (k==VK_NUMPAD7) && (sh & CHK_NUMLK)) cursorbp1=NULL;
			else if(cursorbp1==NULL) set_areamode(); //範囲選択モードでなければ範囲選択モード開始
			cursor_home();
			break;
		case VK_END:
		case VK_NUMPAD1:
			 //シフトキー押下していなければ範囲選択モード解除（NumLock＋シフト＋テンキーでも解除）
			if((sh & CHK_SHIFT)==0 || (k==VK_NUMPAD1) && (sh & CHK_NUMLK)) cursorbp1=NULL;
			else if(cursorbp1==NULL) set_areamode(); //範囲選択モードでなければ範囲選択モード開始
			cursor_end();
			break;
		case VK_PRIOR: // PageUpキー
		case VK_NUMPAD9:
			 //シフト＋PageUpは無効（NumLock＋シフト＋「9」除く）
			if((sh & CHK_SHIFT) && ((k!=VK_NUMPAD9) || ((sh & CHK_NUMLK)==0))) break;
			cursorbp1=NULL; //範囲選択モード解除
			cursor_pageup();
			break;
		case VK_NEXT: // PageDownキー
		case VK_NUMPAD3:
			 //シフト＋PageDownは無効（NumLock＋シフト＋「3」除く）
			if((sh & CHK_SHIFT) && ((k!=VK_NUMPAD3) || ((sh & CHK_NUMLK)==0))) break;
			cursorbp1=NULL; //範囲選択モード解除
			cursor_pagedown();
			break;
		case VK_DELETE: //Deleteキー
		case VK_DECIMAL: //テンキーの「.」
			edited=1; //編集済みフラグ
			if(cursorbp1!=NULL) deletearea();//選択範囲を削除
			else deletechar(cursorbp,cursorix);
			break;
		case VK_BACK: //BackSpaceキー
			edited=1; //編集済みフラグ
			if(cursorbp1!=NULL){
				deletearea();//選択範囲を削除
				break;
			}
			if(cursorix==0 && cursorbp->prev==NULL) break; //バッファ先頭では無視
			cursor_left();
			deletechar(cursorbp,cursorix);
			break;
		case VK_INSERT:
		case VK_NUMPAD0:
			insertmode^=1; //挿入モード、上書きモードを切り替え
			break;
		case 'C':
			//CTRL+C、クリップボードにコピー
			if(cursorbp1!=NULL && (sh & CHK_CTRL)) clipcopy();
			break;
		case 'X':
			//CTRL+X、クリップボードに切り取り
			if(cursorbp1!=NULL && (sh & CHK_CTRL)){
				clipcopy();
				deletearea(); //選択範囲の削除
				edited=1; //編集済みフラグ
			}
			break;
		case 'V':
			//CTRL+V、クリップボードから貼り付け
			if((sh & CHK_CTRL)==0) break;
			if(clipsize==0) break;
			edited=1; //編集済みフラグ
			if(cursorbp1!=NULL){
				//範囲選択している時は削除してから貼り付け
				if(num-countarea()+clipsize<=TBUFMAXSIZE){ //バッファ空き容量チェック
					deletearea();//選択範囲を削除
					clippaste();//クリップボード貼り付け
				}
			}
			else{
				if(num+clipsize<=TBUFMAXSIZE){ //バッファ空き容量チェック
					clippaste();//クリップボード貼り付け
				}
			}
			break;
		case 'S':
			//CTRL+S、SDカードに保存
			if((sh & CHK_CTRL)==0) break;
		case VK_F2: //F2キー
			save_as(); //ファイル名を付けて保存
			break;
		case 'O':
			//CTRL+O、ファイル読み込み
			if((sh & CHK_CTRL)==0) break;
		case VK_F1: //F1キー
			//F1キー、ファイル読み込み
			if(selectfile()==0){ //ファイルを選択して読み込み
				//読み込みを行った場合、カーソル位置を先頭に
				cursor_top();
			}
			break;
		case 'N':
			//CTRL+N、新規作成
			if((sh & CHK_CTRL)==0) break;
		case VK_F4: //F4キー
			newtext(); //新規作成
			break;
	}
}
void texteditor(void){
//テキストエディター本体
	unsigned char k1,k2,sh;

	inittextbuf(); //テキストバッファ初期化
	currentfile[0]=0; //作業中ファイル名クリア
	cursor_top(); //カーソルをテキストバッファの先頭に移動
	insertmode=1; //0:上書き、1:挿入
	clipsize=0; //クリップボードクリア
	blinktimer=0; //カーソル点滅タイマークリア

	while(1){
		redraw();//画面再描画
		displaybottomline(); //画面最下行にファンクションキー機能表示
		setcursor(cx,cy,COLOR_NORMALTEXT);
		getcursorchar(); //カーソル位置の文字を退避（カーソル点滅用）
		while(1){
			//キー入力待ちループ
			wait60thsec(1);  //60分の1秒ウェイト
			blinkcursorchar(); //カーソル点滅させる
			k1=ps2readkey(); //キーバッファから読み込み、k1:通常文字入力の場合ASCIIコード
			if(vkey) break;  //キーが押された場合ループから抜ける
			if(cursorbp1==NULL) gabagecollect1(); //1バイトガベージコレクション（範囲選択時はしない）
		}
		resetcursorchar(); //カーソルを元の文字表示に戻す
		k2=(unsigned char)vkey; //k2:仮想キーコード
		sh=vkey>>8;             //sh:シフト関連キー状態
		if(k2==VK_RETURN || k2==VK_SEPARATOR) k1='\n'; //Enter押下は単純に改行文字を入力とする
		if(k1) normal_code_process(k1); //通常文字が入力された場合
		else control_code_process(k2,sh); //制御文字が入力された場合
		if(cursorbp1!=NULL && cx==cx1 && cy==cy1) cursorbp1=NULL;//選択範囲の開始と終了が重なったら範囲選択モード解除
 	}
}
int main(void){
	/* ポートの初期設定 */
	TRISA = 0x0000; // PORTA全て出力
	ANSELA = 0x0000; // 全てデジタル
	TRISB = KEYSTART | KEYFIRE | KEYUP | KEYDOWN | KEYLEFT | KEYRIGHT;// ボタン接続ポート入力設定
	ANSELB = 0x0000; // 全てデジタル
	CNPUBSET=KEYSTART | KEYFIRE | KEYUP | KEYDOWN | KEYLEFT | KEYRIGHT;// プルアップ設定
	ODCB = 0x0300;	//RB8,RB9はオープンドレイン

    // 周辺機能ピン割り当て
	SDI2R=2; //RPA4:SDI2
	RPB5R=4; //RPB5:SDO2

	ps2mode(); //RA1オン（PS/2有効化マクロ）
	init_composite(); // ビデオメモリクリア、割り込み初期化、カラービデオ出力開始
	setcursor(0,0,COLOR_NORMALTEXT);

	printstr("Init PS/2...");
	if(ps2init()){ //PS/2初期化
		//エラーの場合ストップ
		setcursorcolor(COLOR_ERRORTEXT);
		printstr("\nPS/2 Error\n");
		printstr("Power Off and Check Keyboard\n");
//		while(1) asm("wait"); //無限ループ
	}
    
	printstr("OK\n");
        IEC1bits.CNBIE=0;
//    while(1){
//        int i;
//        clearscreen();
//        setcursor(0,0,COLOR_NORMALTEXT);
//        for(i=0;i<256;i++){
//            if(ps2keystatus[i]){
//                printnum(i);
//                printstr("\n");
//            }
//        }
//        while(!drawing);
//        while(drawing);
//    }

	//SDカードファイルシステム初期化
	while(1){
		setcursorcolor(COLOR_NORMALTEXT);
		printstr("Init File System...");
		// Initialize the File System
		if(FSInit()) break; //ファイルシステム初期化、OKなら抜ける
		//エラーの場合やり直し
		setcursorcolor(COLOR_ERRORTEXT);
		printstr("\nFile System Error\n");
		printstr("Insert Correct Card\n");
		printstr("And Hit Any Key\n");
		inputchar(); //1文字キー入力待ち
	}
	printstr("OK\n");
	wait60thsec(60); //1秒待ち

	texteditor(); //テキストエディター本体呼び出し
}
