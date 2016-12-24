// �e�L�X�gVRAM�R���|�W�b�g�J���[�o�̓v���O���� PIC32MX120F032B�p�@by K.Tanaka Rev.3 �iPS/2�L�[�{�[�h�Ή��j
// �o�� PORTB
// ��30�~�c27�L�����N�^�[�A256�F�J���[�p���b�g
// �N���b�N���g��3.579545MHz�~16�{
//�@�J���[�M���̓J���[�T�u�L�����A��90�x���Ƃɏo�́A270�x��1�h�b�g�i4����3�����j

#include "ps2keyboard.h"
#include "colortext32.h"
#include <plib.h>

// 5bit DA�M���萔�f�[�^
#define C_SYN	0
#define C_BLK	6
#define C_BST1	6
#define C_BST2	3
#define C_BST3	6
#define C_BST4	9

// �p���X���萔
#define V_NTSC		262					// 262�{/���
#define V_SYNC		10					// ���������{��
#define V_PREEQ		26					// �u�����L���O��ԏ㑤�iV_SYNC�{V_PREEQ�͋����Ƃ��邱�Ɓj
#define V_LINE		(WIDTH_Y*8)			// �摜�`����
#define H_NTSC		3632				// ��63.5��sec
#define H_SYNC		269					// �����������A��4.7��sec
#define H_CBST		304					// �J���[�o�[�X�g�J�n�ʒu�i�������������肩��j
#define H_BACK		339					// ���X�y�[�X�i�������������オ�肩��j

// �O���[�o���ϐ���`
unsigned int ClTable[256];	//�J���[�p���b�g�M���e�[�u���A�e�F32bit�����ʂ���8bit�����ɏo��
unsigned char TVRAM[WIDTH_X*WIDTH_Y*2+1] __attribute__ ((aligned (4)));
unsigned char *VRAMp; //VRAM�Ə�����VRAM�A�h���X�BVRAM��1�o�C�g�̃_�~�[�K�v

unsigned int bgcolor;		// �o�b�N�O�����h�J���[�g�`�f�[�^�A32bit�����ʂ���8bit�����ɏo��
volatile unsigned short LineCount;	// �������̍s
volatile unsigned short drawcount;	//�@1��ʕ\���I�����Ƃ�1�����B�A�v������0�ɂ���B
							// �Œ�1��͉�ʕ\���������Ƃ̃`�F�b�N�ƁA�A�v���̏���������ʊ��ԕK�v���̊m�F�ɗ��p�B
volatile char drawing;		//�@�f����ԏ�������-1�A���̑���0

/**********************
*  Timer2 ���荞�ݏ����֐�
***********************/
void __ISR(8, ipl5) T2Handler(void)
{
	asm volatile("#":::"a0");
	asm volatile("#":::"v0");

	//TMR2�̒l�Ń^�C�~���O�̂����␳
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
	asm volatile("sb	$a0,0($v0)");// �����M������������B��������ɑS�Ă̐M���o�͂̃^�C�~���O�𒲐�����

	if(LineCount<V_SYNC){
		// ������������
		OC3R = H_NTSC-H_SYNC-3;	// �؂荞�݃p���X���ݒ�
		OC3CON = 0x8001;
	}
	else{
		OC1R = H_SYNC-3;		// �����p���X��4.7usec
		OC1CON = 0x8001;		// �^�C�}2�I�������V���b�g
        if(LineCount == V_SYNC+V_PREEQ-1){
            mCNBClearIntFlag();
            // CNB���荞�ݖ�����
            IEC1CLR = _IEC1_CNBIE_MASK;
            mTRISPS2CLKOUT();//CLK���o�͂ɐݒ�
            mPS2CLKCLR();
        }
		if(LineCount>=V_SYNC+V_PREEQ && LineCount<V_SYNC+V_PREEQ+V_LINE){
			// �摜�`����
			OC2R = H_SYNC+H_BACK-3-38;// �摜�M���J�n�̃^�C�~���O
			OC2CON = 0x8001;		// �^�C�}2�I�������V���b�g
			if(LineCount==V_SYNC+V_PREEQ){
				VRAMp=TVRAM;
				drawing=-1;
			}
            
			else{
                if((LineCount-(V_SYNC+V_PREEQ))%8==0) VRAMp+=WIDTH_X;// 1�L�����N�^�c8�h�b�g
			}
		}
		else if(LineCount==V_SYNC+V_PREEQ+V_LINE){ // 1��ʍŌ�̕`��I��
			drawing=0;
			drawcount++;
            mCNBIntEnable(1); // CNB���荞�ݗL����
            mTRISPS2CLKIN();//CLK����͂ɐݒ�
		}
	}
	LineCount++;
	if(LineCount>=V_NTSC) LineCount=0;
	IFS0bits.T2IF = 0;			// T2���荞�݃t���O�N���A
}

/*********************
*  OC3���荞�ݏ����֐� ���������؂荞�݃p���X
*********************/
void __ISR(14, ipl5) OC3Handler(void)
{
	asm volatile("#":::"v0");
	asm volatile("#":::"v1");
	asm volatile("#":::"a0");

	//TMR2�̒l�Ń^�C�~���O�̂����␳
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
	// �����M���̃��Z�b�g
	//	LATB=C_BLK;
	asm volatile("addiu	$v1,$zero,%0"::"n"(C_BLK));
	asm volatile("la	$v0,%0"::"i"(&LATB));
	asm volatile("sb	$v1,0($v0)");	// �����M�����Z�b�g�B�����M�����������肩��3153�T�C�N��

	IFS0bits.OC3IF = 0;			// OC3���荞�݃t���O�N���A
}

/*********************
*  OC1���荞�ݏ����֐� �������������オ��?�J���[�o�[�X�g
*********************/
void __ISR(6, ipl5) OC1Handler(void)
{
    
	asm volatile("#":::"v0");
	asm volatile("#":::"v1");
	asm volatile("#":::"a0");

	//TMR2�̒l�Ń^�C�~���O�̂����␳
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
	// �����M���̃��Z�b�g
	//	LATB=C_BLK;
	asm volatile("addiu	$v1,$zero,%0"::"n"(C_BLK));
	asm volatile("la	$v0,%0"::"i"(&LATB));
	asm volatile("sb	$v1,0($v0)");	// �����M�����Z�b�g�B�����������������肩��269�T�C�N��

	// 28�N���b�N�E�F�C�g
	asm volatile("addiu	$a0,$zero,9");
asm volatile("loop2:");
	asm volatile("addiu	$a0,$a0,-1");
	asm volatile("nop");
	asm volatile("bnez	$a0,loop2");

	// �J���[�o�[�X�g�M�� 9�����o��
	asm volatile("addiu	$a0,$zero,4*9");
	asm volatile("la	$v0,%0"::"i"(&LATB));
	asm volatile("lui	$v1,%0"::"n"(C_BST3+(C_BST4<<8)));
	asm volatile("ori	$v1,$v1,%0"::"n"(C_BST1+(C_BST2<<8)));
asm volatile("loop3:");
	asm volatile("addiu	$a0,$a0,-1");	//���[�v�J�E���^
	asm volatile("sb	$v1,0($v0)");	// �J���[�o�[�X�g�J�n�B�����������������肩��304�T�C�N��
	asm volatile("rotr	$v1,$v1,8");
	asm volatile("bnez	$a0,loop3");

	asm volatile("addiu	$v1,$zero,%0"::"n"(C_BLK));
	asm volatile("sb	$v1,0($v0)");

	IFS0bits.OC1IF = 0;			// OC1���荞�݃t���O�N���A
}

/***********************
*  OC2���荞�ݏ����֐��@�f���M���o��
***********************/
void __ISR(10, ipl5) OC2Handler(void)
{
// �f���M���o��
	//�C�����C���A�Z���u���ł̔j�󃌃W�X�^��錾�i�X�^�b�N�ޔ������邽�߁j
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

	//TMR2�̒l�Ń^�C�~���O�̂����␳
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
	//	a2=FontData+�`�撆�̍s%8
	asm volatile("la	$v0,%0"::"i"(&LineCount));
	asm volatile("lhu	$v1,0($v0)");
	asm volatile("la	$a2,%0"::"i"(FontData));
	asm volatile("addiu	$v1,$v1,%0"::"n"(-V_SYNC-V_PREEQ-1));
	asm volatile("andi	$v1,$v1,7");
	asm volatile("addu	$a2,$a2,$v1");
	//	v1=bgcolor�g�`�f�[�^
	asm volatile("la	$v0,%0"::"i"(&bgcolor));
	asm volatile("lw	$v1,0($v0)");
	//	v0=&LATB;
	asm volatile("la	$v0,%0"::"i"(&LATB));

	asm volatile("addiu	$t6,$zero,%0"::"n"(WIDTH_X));//���[�v�J�E���^

// ��������o�͔g�`�f�[�^����
	asm volatile("lbu	$t0,0($a0)");
	asm volatile("lbu	$t1,%0($a0)"::"n"(ATTROFFSET));
	asm volatile("sll	$t0,$t0,3");
	asm volatile("addu	$t0,$t0,$a2");
	asm volatile("lbu	$t0,0($t0)");
	asm volatile("sll	$t1,$t1,2");
	asm volatile("addu	$t1,$t1,$a1");
	asm volatile("lw	$t1,0($t1)");
	asm volatile("addiu	$a0,$a0,1");//�����Ő����N���b�N���������肩��604�N���b�N

asm volatile("loop1:");
	asm volatile("addu	$t2,$zero,$v1");
	asm volatile("ext	$t3,$t0,7,1");
	asm volatile("movn	$t2,$t1,$t3");
	asm volatile("sb	$t2,0($v0)"); // �����ōŏ��̐M���o��
	asm volatile("rotr	$t2,$t2,8");
	asm volatile("rotr	$v1,$v1,24");
	asm volatile("rotr	$t1,$t1,24");
	asm volatile("sb	$t2,0($v0)");
	asm volatile("rotr	$t2,$t2,8");
			asm volatile("lbu	$t4,0($a0)"); //�󂢂Ă��鎞�ԂɎ��̕����o�͂̏����J�n
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
					asm volatile("addiu	$t6,$t6,-1");//���[�v�J�E���^
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
	asm volatile("sb	$t2,0($v0)");//�x���X���b�g�̂���1�N���b�N�x���
	asm volatile("bnez	$t6,loop1");

	asm volatile("nop");
	asm volatile("nop");
	//	LATB=C_BLK;
	asm volatile("addiu	$t0,$zero,%0"::"n"(C_BLK));
	asm volatile("sb	$t0,0($v0)");
/*
	if((LineCount-(V_SYNC+V_PREEQ))%8==0) VRAMp+=WIDTH_X;// 1�L�����N�^�c8�h�b�g(LineCount��1������Ă���)
	if(LineCount==V_SYNC+V_PREEQ+V_LINE){ // 1��ʍŌ�̕`��I��
			drawing=0;
			drawcount++;
			VRAMp=TVRAM;
	}
*/
 	IFS0bits.OC2IF = 0;			// OC2���荞�݃t���O�N���A
}

// ��ʃN���A
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
	// �J���[�p���b�g�ݒ�i5�r�b�gDA�A�d��3.3V�A90�x�P�ʁj
	// n:�p���b�g�ԍ��Ar,g,b:0?255
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
	// �o�b�N�O�����h�J���[�ݒ� r,g,b:0?255
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
	// �ϐ������ݒ�
	LineCount=0;				// ���������C���J�E���^�[
	drawing=0;
	VRAMp=TVRAM;

	PR2 = H_NTSC -1; 			// ��63.5usec�ɐݒ�
	T2CONSET=0x8000;			// �^�C�}2�X�^�[�g
}
void stop_composite(void)
{
	T2CONCLR = 0x8000;			// �^�C�}2��~
}

// �J���[�R���|�W�b�g�o�͏�����
void init_composite(void)
{
	int i;
	clearscreen();

	//�J���[�ԍ�0?7�̃p���b�g������
	for(i=0;i<8;i++){
		set_palette(i,255*(i&1),255*((i>>1)&1),255*(i>>2));
	}
	for(;i<256;i++){
		set_palette(i,255,255,255); //8�ȏ�͑S�Ĕ��ɏ�����
	}
	set_bgcolor(0,0,0); //�o�b�N�O�����h�J���[�͍�
	setcursorcolor(7);

	// �^�C�}2�̏����ݒ�,�����N���b�N��63.5usec�����A1:1
	T2CON = 0x0000;				// �^�C�}2��~���
	IPC2bits.T2IP = 5;			// ���荞�݃��x��5
	IFS0bits.T2IF = 0;
	IEC0bits.T2IE = 1;			// �^�C�}2���荞�ݗL����

	// OC1�̊��荞�ݗL����
	IPC1bits.OC1IP = 5;			// ���荞�݃��x��5
	IFS0bits.OC1IF = 0;
	IEC0bits.OC1IE = 1;			// OC1���荞�ݗL����

	// OC2�̊��荞�ݗL����
	IPC2bits.OC2IP = 5;			// ���荞�݃��x��5
	IFS0bits.OC2IF = 0;
	IEC0bits.OC2IE = 1;			// OC2���荞�ݗL����

	// OC3�̊��荞�ݗL����
	IPC3bits.OC3IP = 5;			// ���荞�݃��x��5
	IFS0bits.OC3IF = 0;
	IEC0bits.OC3IE = 1;			// OC3���荞�ݗL����

	OSCCONCLR=0x10; // WAIT���߂̓A�C�h�����[�h
	BMXCONCLR=0x40;	// RAM�A�N�Z�X�E�F�C�g0
	INTEnableSystemMultiVectoredInt();
	start_composite();
}