// テキストVRAMコンポジットカラー出力プログラム PIC32MX120F032B用　by K.Tanaka Rev.3 （PS/2キーボード対応）
// 出力 PORTB
// 横30×縦27キャラクター、256色カラーパレット
// クロック周波数3.579545MHz×16倍
//　カラー信号はカラーサブキャリアの90度ごとに出力、270度で1ドット（4分の3周期）

#include "ps2keyboard.h"
#include "colortext32.h"
#include <plib.h>

// 5bit DA信号定数データ
#define C_SYN	0
#define C_BLK	6
#define C_BST1	6
#define C_BST2	3
#define C_BST3	6
#define C_BST4	9

// パルス幅定数
#define V_NTSC		262					// 262本/画面
#define V_SYNC		10					// 垂直同期本数
#define V_PREEQ		26					// ブランキング区間上側（V_SYNC＋V_PREEQは偶数とすること）
#define V_LINE		(WIDTH_Y*8)			// 画像描画区間
#define H_NTSC		3632				// 約63.5μsec
#define H_SYNC		269					// 水平同期幅、約4.7μsec
#define H_CBST		304					// カラーバースト開始位置（水平同期立下りから）
#define H_BACK		339					// 左スペース（水平同期立ち上がりから）

// グローバル変数定義
unsigned int ClTable[256];	//カラーパレット信号テーブル、各色32bitを下位から8bitずつ順に出力
unsigned char TVRAM[WIDTH_X*WIDTH_Y*2+1] __attribute__ ((aligned (4)));
unsigned char *VRAMp; //VRAMと処理中VRAMアドレス。VRAMは1バイトのダミー必要

unsigned int bgcolor;		// バックグランドカラー波形データ、32bitを下位から8bitずつ順に出力
volatile unsigned short LineCount;	// 処理中の行
volatile unsigned short drawcount;	//　1画面表示終了ごとに1足す。アプリ側で0にする。
							// 最低1回は画面表示したことのチェックと、アプリの処理が何画面期間必要かの確認に利用。
volatile char drawing;		//　映像区間処理中は-1、その他は0

/**********************
*  Timer2 割り込み処理関数
***********************/
void __ISR(8, ipl5) T2Handler(void)
{
	asm volatile("#":::"a0");
	asm volatile("#":::"v0");

	//TMR2の値でタイミングのずれを補正
	asm volatile("la	$v0,%0"::"i"(&TMR2));
	asm volatile("lhu	$a0,0($v0)");
	asm volatile("addiu	$a0,$a0,-22");
	asm volatile("bltz	$a0,label1_2");
	asm volatile("addiu	$v0,$a0,-10");
	asm volatile("bgtz	$v0,label1_2");
	asm volatile("sll	$a0,$a0,2");
	asm volatile("la	$v0,label1");
	asm volatile("addu	$a0,$v0");
	asm volatile("jr	$a0");

asm volatile("label1:");
	asm volatile("nop");asm volatile("nop");asm volatile("nop");asm volatile("nop");
	asm volatile("nop");asm volatile("nop");asm volatile("nop");asm volatile("nop");
	asm volatile("nop");asm volatile("nop");

asm volatile("label1_2:");
	//LATB=C_SYN;
	asm volatile("addiu	$a0,$zero,%0"::"n"(C_SYN));
	asm volatile("la	$v0,%0"::"i"(&LATB));
	asm volatile("sb	$a0,0($v0)");// 同期信号立ち下がり。ここを基準に全ての信号出力のタイミングを調整する

	if(LineCount<V_SYNC){
		// 垂直同期期間
		OC3R = H_NTSC-H_SYNC-3;	// 切り込みパルス幅設定
		OC3CON = 0x8001;
	}
	else{
		OC1R = H_SYNC-3;		// 同期パルス幅4.7usec
		OC1CON = 0x8001;		// タイマ2選択ワンショット
        if(LineCount == V_SYNC+V_PREEQ-1){
            mCNBClearIntFlag();
            // CNB割り込み無効化
            IEC1CLR = _IEC1_CNBIE_MASK;
            mTRISPS2CLKOUT();//CLKを出力に設定
            mPS2CLKCLR();
        }
		if(LineCount>=V_SYNC+V_PREEQ && LineCount<V_SYNC+V_PREEQ+V_LINE){
			// 画像描画区間
			OC2R = H_SYNC+H_BACK-3-38;// 画像信号開始のタイミング
			OC2CON = 0x8001;		// タイマ2選択ワンショット
			if(LineCount==V_SYNC+V_PREEQ){
				VRAMp=TVRAM;
				drawing=-1;
			}
            
			else{
                if((LineCount-(V_SYNC+V_PREEQ))%8==0) VRAMp+=WIDTH_X;// 1キャラクタ縦8ドット
			}
		}
		else if(LineCount==V_SYNC+V_PREEQ+V_LINE){ // 1画面最後の描画終了
			drawing=0;
			drawcount++;
            mCNBIntEnable(1); // CNB割り込み有効化
            mTRISPS2CLKIN();//CLKを入力に設定
		}
	}
	LineCount++;
	if(LineCount>=V_NTSC) LineCount=0;
	IFS0bits.T2IF = 0;			// T2割り込みフラグクリア
}

/*********************
*  OC3割り込み処理関数 垂直同期切り込みパルス
*********************/
void __ISR(14, ipl5) OC3Handler(void)
{
	asm volatile("#":::"v0");
	asm volatile("#":::"v1");
	asm volatile("#":::"a0");

	//TMR2の値でタイミングのずれを補正
	asm volatile("la	$v0,%0"::"i"(&TMR2));
	asm volatile("lhu	$a0,0($v0)");
	asm volatile("addiu	$a0,$a0,%0"::"n"(-(H_NTSC-H_SYNC+22)));
	asm volatile("bltz	$a0,label4_2");
	asm volatile("addiu	$v0,$a0,-10");
	asm volatile("bgtz	$v0,label4_2");
	asm volatile("sll	$a0,$a0,2");
	asm volatile("la	$v0,label4");
	asm volatile("addu	$a0,$v0");
	asm volatile("jr	$a0");

asm volatile("label4:");
	asm volatile("nop");asm volatile("nop");asm volatile("nop");asm volatile("nop");
	asm volatile("nop");asm volatile("nop");asm volatile("nop");asm volatile("nop");
	asm volatile("nop");asm volatile("nop");

asm volatile("label4_2:");
	// 同期信号のリセット
	//	LATB=C_BLK;
	asm volatile("addiu	$v1,$zero,%0"::"n"(C_BLK));
	asm volatile("la	$v0,%0"::"i"(&LATB));
	asm volatile("sb	$v1,0($v0)");	// 同期信号リセット。同期信号立ち下がりから3153サイクル

	IFS0bits.OC3IF = 0;			// OC3割り込みフラグクリア
}

/*********************
*  OC1割り込み処理関数 水平同期立ち上がり?カラーバースト
*********************/
void __ISR(6, ipl5) OC1Handler(void)
{
    
	asm volatile("#":::"v0");
	asm volatile("#":::"v1");
	asm volatile("#":::"a0");

	//TMR2の値でタイミングのずれを補正
	asm volatile("la	$v0,%0"::"i"(&TMR2));
	asm volatile("lhu	$a0,0($v0)");
	asm volatile("addiu	$a0,$a0,%0"::"n"(-(H_SYNC+22)));
	asm volatile("bltz	$a0,label2_2");
	asm volatile("addiu	$v0,$a0,-10");
	asm volatile("bgtz	$v0,label2_2");
	asm volatile("sll	$a0,$a0,2");
	asm volatile("la	$v0,label2");
	asm volatile("addu	$a0,$v0");
	asm volatile("jr	$a0");

asm volatile("label2:");
	asm volatile("nop");asm volatile("nop");asm volatile("nop");asm volatile("nop");
	asm volatile("nop");asm volatile("nop");asm volatile("nop");asm volatile("nop");
	asm volatile("nop");asm volatile("nop");

asm volatile("label2_2:");
	// 同期信号のリセット
	//	LATB=C_BLK;
	asm volatile("addiu	$v1,$zero,%0"::"n"(C_BLK));
	asm volatile("la	$v0,%0"::"i"(&LATB));
	asm volatile("sb	$v1,0($v0)");	// 同期信号リセット。水平同期立ち下がりから269サイクル

	// 28クロックウェイト
	asm volatile("addiu	$a0,$zero,9");
asm volatile("loop2:");
	asm volatile("addiu	$a0,$a0,-1");
	asm volatile("nop");
	asm volatile("bnez	$a0,loop2");

	// カラーバースト信号 9周期出力
	asm volatile("addiu	$a0,$zero,4*9");
	asm volatile("la	$v0,%0"::"i"(&LATB));
	asm volatile("lui	$v1,%0"::"n"(C_BST3+(C_BST4<<8)));
	asm volatile("ori	$v1,$v1,%0"::"n"(C_BST1+(C_BST2<<8)));
asm volatile("loop3:");
	asm volatile("addiu	$a0,$a0,-1");	//ループカウンタ
	asm volatile("sb	$v1,0($v0)");	// カラーバースト開始。水平同期立ち下がりから304サイクル
	asm volatile("rotr	$v1,$v1,8");
	asm volatile("bnez	$a0,loop3");

	asm volatile("addiu	$v1,$zero,%0"::"n"(C_BLK));
	asm volatile("sb	$v1,0($v0)");

	IFS0bits.OC1IF = 0;			// OC1割り込みフラグクリア
}

/***********************
*  OC2割り込み処理関数　映像信号出力
***********************/
void __ISR(10, ipl5) OC2Handler(void)
{
// 映像信号出力
	//インラインアセンブラでの破壊レジスタを宣言（スタック退避させるため）
	asm volatile("#":::"v0");
	asm volatile("#":::"v1");
	asm volatile("#":::"a0");
	asm volatile("#":::"a1");
	asm volatile("#":::"a2");
	asm volatile("#":::"t0");
	asm volatile("#":::"t1");
	asm volatile("#":::"t2");
	asm volatile("#":::"t3");
	asm volatile("#":::"t4");
	asm volatile("#":::"t5");
	asm volatile("#":::"t6");

	//TMR2の値でタイミングのずれを補正
	asm volatile("la	$v0,%0"::"i"(&TMR2));
	asm volatile("lhu	$a0,0($v0)");
	asm volatile("addiu	$a0,$a0,%0"::"n"(-(H_SYNC+H_BACK+31-38)));
	asm volatile("bltz	$a0,label3_2");
	asm volatile("addiu	$v0,$a0,-10");
	asm volatile("bgtz	$v0,label3_2");
	asm volatile("sll	$a0,$a0,2");
	asm volatile("la	$v0,label3");
	asm volatile("addu	$a0,$v0");
	asm volatile("jr	$a0");

asm volatile("label3:");
	asm volatile("nop");asm volatile("nop");asm volatile("nop");asm volatile("nop");
	asm volatile("nop");asm volatile("nop");asm volatile("nop");asm volatile("nop");
	asm volatile("nop");asm volatile("nop");

asm volatile("label3_2:");
	//	drawing=-1;
	asm volatile("addiu	$t1,$zero,-1");
	asm volatile("la	$v0,%0"::"i"(&drawing));
	asm volatile("sb	$t1,0($v0)");
	//	a0=VRAMp;
	asm volatile("la	$v0,%0"::"i"(&VRAMp));
	asm volatile("lw	$a0,0($v0)");
	//	a1=ClTable;
	asm volatile("la	$a1,%0"::"i"(ClTable));
	//	a2=FontData+描画中の行%8
	asm volatile("la	$v0,%0"::"i"(&LineCount));
	asm volatile("lhu	$v1,0($v0)");
	asm volatile("la	$a2,%0"::"i"(FontData));
	asm volatile("addiu	$v1,$v1,%0"::"n"(-V_SYNC-V_PREEQ-1));
	asm volatile("andi	$v1,$v1,7");
	asm volatile("addu	$a2,$a2,$v1");
	//	v1=bgcolor波形データ
	asm volatile("la	$v0,%0"::"i"(&bgcolor));
	asm volatile("lw	$v1,0($v0)");
	//	v0=&LATB;
	asm volatile("la	$v0,%0"::"i"(&LATB));

	asm volatile("addiu	$t6,$zero,%0"::"n"(WIDTH_X));//ループカウンタ

// ここから出力波形データ生成
	asm volatile("lbu	$t0,0($a0)");
	asm volatile("lbu	$t1,%0($a0)"::"n"(ATTROFFSET));
	asm volatile("sll	$t0,$t0,3");
	asm volatile("addu	$t0,$t0,$a2");
	asm volatile("lbu	$t0,0($t0)");
	asm volatile("sll	$t1,$t1,2");
	asm volatile("addu	$t1,$t1,$a1");
	asm volatile("lw	$t1,0($t1)");
	asm volatile("addiu	$a0,$a0,1");//ここで水平クロック立ち下がりから604クロック

asm volatile("loop1:");
	asm volatile("addu	$t2,$zero,$v1");
	asm volatile("ext	$t3,$t0,7,1");
	asm volatile("movn	$t2,$t1,$t3");
	asm volatile("sb	$t2,0($v0)"); // ここで最初の信号出力
	asm volatile("rotr	$t2,$t2,8");
	asm volatile("rotr	$v1,$v1,24");
	asm volatile("rotr	$t1,$t1,24");
	asm volatile("sb	$t2,0($v0)");
	asm volatile("rotr	$t2,$t2,8");
			asm volatile("lbu	$t4,0($a0)"); //空いている時間に次の文字出力の準備開始
			asm volatile("lbu	$t5,%0($a0)"::"n"(ATTROFFSET));
	asm volatile("sb	$t2,0($v0)");
	
	asm volatile("addu	$t2,$zero,$v1");
	asm volatile("ext	$t3,$t0,6,1");
	asm volatile("movn	$t2,$t1,$t3");
	asm volatile("sb	$t2,0($v0)");
	asm volatile("rotr	$t2,$t2,8");
	asm volatile("rotr	$v1,$v1,24");
	asm volatile("rotr	$t1,$t1,24");
	asm volatile("sb	$t2,0($v0)");
	asm volatile("rotr	$t2,$t2,8");
			asm volatile("sll	$t4,$t4,3");
			asm volatile("addu	$t4,$t4,$a2");
	asm volatile("sb	$t2,0($v0)");
	
	asm volatile("addu	$t2,$zero,$v1");
	asm volatile("ext	$t3,$t0,5,1");
	asm volatile("movn	$t2,$t1,$t3");
	asm volatile("sb	$t2,0($v0)");
	asm volatile("rotr	$t2,$t2,8");
	asm volatile("rotr	$v1,$v1,24");
	asm volatile("rotr	$t1,$t1,24");
	asm volatile("sb	$t2,0($v0)");
	asm volatile("rotr	$t2,$t2,8");
			asm volatile("sll	$t5,$t5,2");
			asm volatile("addu	$t5,$t5,$a1");
	asm volatile("sb	$t2,0($v0)");
	
	asm volatile("addu	$t2,$zero,$v1");
	asm volatile("ext	$t3,$t0,4,1");
	asm volatile("movn	$t2,$t1,$t3");
	asm volatile("sb	$t2,0($v0)");
	asm volatile("rotr	$t2,$t2,8");
	asm volatile("rotr	$v1,$v1,24");
	asm volatile("rotr	$t1,$t1,24");
	asm volatile("sb	$t2,0($v0)");
	asm volatile("rotr	$t2,$t2,8");
			asm volatile("lw	$t5,0($t5)");
			asm volatile("addiu	$a0,$a0,1");
	asm volatile("sb	$t2,0($v0)");
	
	asm volatile("addu	$t2,$zero,$v1");
	asm volatile("ext	$t3,$t0,3,1");
	asm volatile("movn	$t2,$t1,$t3");
	asm volatile("sb	$t2,0($v0)");
	asm volatile("rotr	$t2,$t2,8");
	asm volatile("rotr	$v1,$v1,24");
	asm volatile("rotr	$t1,$t1,24");
	asm volatile("sb	$t2,0($v0)");
	asm volatile("rotr	$t2,$t2,8");
asm volatile("nop");
asm volatile("nop");
	asm volatile("sb	$t2,0($v0)");
	
	asm volatile("addu	$t2,$zero,$v1");
	asm volatile("ext	$t3,$t0,2,1");
	asm volatile("movn	$t2,$t1,$t3");
	asm volatile("sb	$t2,0($v0)");
	asm volatile("rotr	$t2,$t2,8");
	asm volatile("rotr	$v1,$v1,24");
	asm volatile("rotr	$t1,$t1,24");
	asm volatile("sb	$t2,0($v0)");
	asm volatile("rotr	$t2,$t2,8");
					asm volatile("addiu	$t6,$t6,-1");//ループカウンタ
asm volatile("nop");
	asm volatile("sb	$t2,0($v0)");
	
	asm volatile("addu	$t2,$zero,$v1");
	asm volatile("ext	$t3,$t0,1,1");
	asm volatile("movn	$t2,$t1,$t3");
	asm volatile("sb	$t2,0($v0)");
	asm volatile("rotr	$t2,$t2,8");
	asm volatile("rotr	$v1,$v1,24");
	asm volatile("rotr	$t1,$t1,24");
	asm volatile("sb	$t2,0($v0)");
	asm volatile("rotr	$t2,$t2,8");
			asm volatile("lbu	$t4,0($t4)");
	asm volatile("sb	$t2,0($v0)");
	
	asm volatile("addu	$t2,$zero,$v1");
	asm volatile("ext	$t3,$t0,0,1");
	asm volatile("movn	$t2,$t1,$t3");
	asm volatile("sb	$t2,0($v0)");
	asm volatile("rotr	$t2,$t2,8");
	asm volatile("rotr	$v1,$v1,24");
			asm volatile("addu	$t1,$zero,$t5");
	asm volatile("sb	$t2,0($v0)");
	asm volatile("rotr	$t2,$t2,8");
			asm volatile("addu	$t0,$zero,$t4");
	asm volatile("sb	$t2,0($v0)");//遅延スロットのため1クロック遅れる
	asm volatile("bnez	$t6,loop1");

	asm volatile("nop");
	asm volatile("nop");
	//	LATB=C_BLK;
	asm volatile("addiu	$t0,$zero,%0"::"n"(C_BLK));
	asm volatile("sb	$t0,0($v0)");
/*
	if((LineCount-(V_SYNC+V_PREEQ))%8==0) VRAMp+=WIDTH_X;// 1キャラクタ縦8ドット(LineCountは1足されている)
	if(LineCount==V_SYNC+V_PREEQ+V_LINE){ // 1画面最後の描画終了
			drawing=0;
			drawcount++;
			VRAMp=TVRAM;
	}
*/
 	IFS0bits.OC2IF = 0;			// OC2割り込みフラグクリア
}

// 画面クリア
void clearscreen(void)
{
	unsigned int *vp;
	int i;
	vp=(unsigned int *)TVRAM;
	for(i=0;i<WIDTH_X*WIDTH_Y*2/4;i++) *vp++=0;
	cursor=TVRAM;
}

void set_palette(unsigned char n,unsigned char b,unsigned char r,unsigned char g)
{
	// カラーパレット設定（5ビットDA、電源3.3V、90度単位）
	// n:パレット番号、r,g,b:0?255
	int y,s0,s1,s2,s3;
	y=(150*g+29*b+77*r+128)/256;
	s0=(3525*y+3093*((int)r-y)+1440*256+32768)/65536;
	s1=(3525*y+1735*((int)b-y)+1440*256+32768)/65536;
	s2=(3525*y-3093*((int)r-y)+1440*256+32768)/65536;
	s3=(3525*y-1735*((int)b-y)+1440*256+32768)/65536;
	ClTable[n]=s0+(s1<<8)+(s2<<16)+(s3<<24);
}
void set_bgcolor(unsigned char b,unsigned char r,unsigned char g)
{
	// バックグランドカラー設定 r,g,b:0?255
	int y,s0,s1,s2,s3;
	y=(150*g+29*b+77*r+128)/256;
	s0=(3525*y+3093*((int)r-y)+1440*256+32768)/65536;
	s1=(3525*y+1735*((int)b-y)+1440*256+32768)/65536;
	s2=(3525*y-3093*((int)r-y)+1440*256+32768)/65536;
	s3=(3525*y-1735*((int)b-y)+1440*256+32768)/65536;
	bgcolor=s0+(s1<<8)+(s2<<16)+(s3<<24);
}

void start_composite(void)
{
	// 変数初期設定
	LineCount=0;				// 処理中ラインカウンター
	drawing=0;
	VRAMp=TVRAM;

	PR2 = H_NTSC -1; 			// 約63.5usecに設定
	T2CONSET=0x8000;			// タイマ2スタート
}
void stop_composite(void)
{
	T2CONCLR = 0x8000;			// タイマ2停止
}

// カラーコンポジット出力初期化
void init_composite(void)
{
	int i;
	clearscreen();

	//カラー番号0?7のパレット初期化
	for(i=0;i<8;i++){
		set_palette(i,255*(i&1),255*((i>>1)&1),255*(i>>2));
	}
	for(;i<256;i++){
		set_palette(i,255,255,255); //8以上は全て白に初期化
	}
	set_bgcolor(0,0,0); //バックグランドカラーは黒
	setcursorcolor(7);

	// タイマ2の初期設定,内部クロックで63.5usec周期、1:1
	T2CON = 0x0000;				// タイマ2停止状態
	IPC2bits.T2IP = 5;			// 割り込みレベル5
	IFS0bits.T2IF = 0;
	IEC0bits.T2IE = 1;			// タイマ2割り込み有効化

	// OC1の割り込み有効化
	IPC1bits.OC1IP = 5;			// 割り込みレベル5
	IFS0bits.OC1IF = 0;
	IEC0bits.OC1IE = 1;			// OC1割り込み有効化

	// OC2の割り込み有効化
	IPC2bits.OC2IP = 5;			// 割り込みレベル5
	IFS0bits.OC2IF = 0;
	IEC0bits.OC2IE = 1;			// OC2割り込み有効化

	// OC3の割り込み有効化
	IPC3bits.OC3IP = 5;			// 割り込みレベル5
	IFS0bits.OC3IF = 0;
	IEC0bits.OC3IE = 1;			// OC3割り込み有効化

	OSCCONCLR=0x10; // WAIT命令はアイドルモード
	BMXCONCLR=0x40;	// RAMアクセスウェイト0
	INTEnableSystemMultiVectoredInt();
	start_composite();
}