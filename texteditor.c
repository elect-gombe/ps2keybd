// texteditor.c
// Text Editor with PS/2 Keyboard for PIC32MX150F128B / PIC32MX250F128B by K.Tanaka

// ���p�V�X�e��
// ps2keyboard.a : PS/2�L�[�{�[�h���̓V�X�e�����C�u����
// lib_colortext32.a : �J���[�r�f�I�M���o�̓V�X�e�����C�u�����i30�~27�e�L�X�g�Łj
// libsdfsio.a �F SD�J�[�h�A�N�Z�X�p���C�u����

#include <plib.h>
#include "colortext32.h"
#include "ps2keyboard.h"
#include "keyinput.h"
#include "SDFSIO.h"

//�O�t���N���X�^�� with PLL (16�{)
#pragma config PMDL1WAY = OFF, IOL1WAY = OFF
#pragma config FPLLIDIV = DIV_1, FPLLMUL = MUL_16, FPLLODIV = DIV_1
#pragma config FNOSC = PRIPLL, FSOSCEN = OFF, POSCMOD = XT, OSCIOFNC = OFF
#pragma config FPBDIV = DIV_1, FWDTEN = OFF, JTAGEN = OFF, ICESEL = ICS_PGx1

#define TBUFMAXLINE 101 //�e�L�X�g�o�b�t�@��
#define TBUFSIZE 200 //�e�L�X�g�o�b�t�@1�̃T�C�Y
#define TBUFMAXSIZE (TBUFSIZE*(TBUFMAXLINE-1)) //�ő�o�b�t�@�e�ʁi�o�b�t�@1�s���󂯂�j
#define EDITWIDTHX 30 //�G�f�B�^��ʉ���
#define EDITWIDTHY 26 //�G�f�B�^��ʏc��
#define COLOR_NORMALTEXT 7 //�ʏ�e�L�X�g�F
#define COLOR_ERRORTEXT 4 //�G���[���b�Z�[�W�e�L�X�g�F
#define COLOR_AREASELECTTEXT 4 //�͈͑I���e�L�X�g�F
#define COLOR_BOTTOMLINE 5 //��ʍŉ��s�̐F
#define FILEBUFSIZE 256 //�t�@�C���A�N�Z�X�p�o�b�t�@�T�C�Y
#define MAXFILENUM 50 //���p�\�t�@�C���ő吔

#define ERR_FILETOOBIG -1
#define ERR_CANTFILEOPEN -2
#define ERR_CANTWRITEFILE -3


struct _TBUF{
//�����N�t���̃e�L�X�g�o�b�t�@
	struct _TBUF *prev;//�O���ւ̃����N�BNULL�̏ꍇ�擪�܂��͋�
	struct _TBUF *next;//����ւ̃����N�BNULL�̏ꍇ�Ō�
	unsigned short n;//���݂̎g�p�o�C�g��
	unsigned char Buf[TBUFSIZE];//�o�b�t�@
} ;
typedef struct _TBUF _tbuf;

_tbuf TextBuffer[TBUFMAXLINE]; //�e�L�X�g�o�b�t�@�̎���
_tbuf *TBufstart; //�e�L�X�g�o�b�t�@�̐擪�ʒu
_tbuf *cursorbp; //���݂̃J�[�\���ʒu�̃e�L�X�g�o�b�t�@
unsigned short cursorix; //���݂̃J�[�\���ʒu�̃e�L�X�g�o�b�t�@�擪����̈ʒu
_tbuf *disptopbp; //���ݕ\������ʍ���̃e�L�X�g�o�b�t�@
unsigned short disptopix; //���ݕ\������ʍ���̃e�L�X�g�o�b�t�@�擪����̈ʒu
int num; //���݃o�b�t�@���Ɋi�[����Ă��镶����

int cx,cy; //�J�[�\�����W
int cx2; //�㉺�ړ����̉��J�[�\��X���W

_tbuf *cursorbp1; //�͈͑I�����̃J�[�\���X�^�[�g�ʒu�̃e�L�X�g�o�b�t�@�A�͈͑I�����[�h�łȂ��ꍇNULL
unsigned short cursorix1; //�͈͑I�����̃J�[�\���X�^�[�g�ʒu�̃e�L�X�g�o�b�t�@�擪����̈ʒu
int cx1,cy1; //�͈͑I�����̃J�[�\���X�^�[�g���W

// �J�[�\���֘A�ʒu�̈ꎞ���p
_tbuf *cursorbp_t;
unsigned short cursorix_t;
_tbuf *disptopbp_t;
unsigned short disptopix_t;
int cx_t,cy_t;
unsigned char clipboard[EDITWIDTHX*EDITWIDTHY]; //�N���b�v�{�[�h�A�ő�T�C�Y�͕ҏW��ʗ̈�Ɠ���
int clipsize; //���݃N���b�v�{�[�h�Ɋi�[����Ă��镶����

int edited; //�ۑ���ɕύX���ꂽ����\���t���O

unsigned char filebuf[FILEBUFSIZE]; //�t�@�C���A�N�Z�X�p�o�b�t�@
unsigned char currentfile[8+1+3+1],tempfile[8+1+3+1]; //�ҏW���̃t�@�C�����A�ꎞ�t�@�C����
unsigned char filenames[MAXFILENUM][13];

void wait60thsec(unsigned short n){
	// 60����n�b�E�F�C�g�i�r�f�I��ʂ̍ŉ��s�M���o�͏I���܂ő҂j
	drawcount=0;
	while(drawcount<n) asm("wait");
}

_tbuf * newTBuf(_tbuf *prev){
// �V�����e�L�X�g�o�b�t�@1�s�𐶐�
// prev:�}����̍s�iprev�̌��ɒǉ��j
// �߂�l�@���������o�b�t�@�ւ̃|�C���^�A�����ł��Ȃ��ꍇNULL
	_tbuf *bp,*next;

	//�o�b�t�@�̐擪����󂫂��T�[�`
	bp=TextBuffer;
	while(1){
		if(bp->prev==NULL && bp!=TBufstart) break;
		bp++;
		if(bp>=TextBuffer+TBUFMAXLINE) return NULL;//�Ō�܂ŋ󂫂Ȃ�
	}
	next=prev->next;
	//�s�}��
	bp->prev=prev;
	bp->next=next;
	prev->next=bp;
	if(next!=NULL) next->prev=bp;
	bp->n=0;
	return bp;
}

_tbuf * deleteTBuf(_tbuf *bp){
// �e�L�X�g�o�b�t�@�̍폜
// bp:�폜����s�̃|�C���^
// �߂�l�@�폜�O�̎��̃o�b�t�@�ւ̃|�C���^�A�Ȃ��ꍇNULL
	unsigned short a,b;
	_tbuf *prev,*next;
	prev=bp->prev;
	next=bp->next;
	if(prev==NULL){
		//�擪�s�̏ꍇ
		if(next==NULL) return next; //�Ō��1�s�̏ꍇ�͍폜���Ȃ�
		TBufstart=next; //���̍s��擪�s�ݒ�
	}
	else prev->next=next; //�O�����Ƀ����N�i�ŏI�s�Ȃ�NULL���R�s�[�����j
	if(next!=NULL) next->prev=prev; //��������Ύ���O�Ƀ����N
	bp->prev=NULL; //�󂫃t���O�ݒ�
	return next;
}

int insertchar(_tbuf *bp,unsigned int ix,unsigned char c){
//�e�L�X�g�o�b�t�@bp�̐擪����ix�o�C�g�̈ʒu��c��}��
//�߂�l�@�����F0�A�s���܂��͗e�ʃI�[�o�[�F-1�A�󂫂�����͂��Ȃ̂Ɏ��s�F1
	unsigned char *p;

	if(ix > bp->n) return -1; //�s���w��
	if(num >= TBUFMAXSIZE) return -1; //�o�b�t�@�e�ʃI�[�o�[
	if(bp->n < TBUFSIZE){
		//���C����������1�o�C�g�}���\//
		for(p=bp->Buf + bp->n ; p > bp->Buf+ix ; p--) *p=*(p-1);
		*p=c;
		bp->n++;
		num++; //�o�b�t�@�g�p��
//		if(bp->n >= TBUFSIZE && bp->next==NULL) newTBuf(bp); //�o�b�t�@�������ς��ɂȂ�����V���Ƀo�b�t�@����
		return 0;
	}
	//���C�������ӂ��ꍇ
	if(bp->next==NULL || bp->next->n >=TBUFSIZE){
		// �ŏI�s�܂��͎��̃��C���o�b�t�@�������ς����������s�}��
		if(newTBuf(bp)==NULL){
			// ���C���o�b�t�@�}���s��
			return 1;
		}
	}
	if(ix==TBUFSIZE){
		insertchar(bp->next,0,c);
		return 0;
	}
	p=bp->Buf + TBUFSIZE-1;
	insertchar(bp->next,0,*p); //���̍s�̐擪��1�����}���i�K���󂫂���j
	for( ; p > bp->Buf+ix ; p--) *p=*(p-1);
	*p=c;
	return 0;
}

int overwritechar(_tbuf *bp,unsigned int ix,unsigned char c){
//�e�L�X�g�o�b�t�@bp�̐擪����ix�o�C�g�̈ʒu��c�ŏ㏑��
//�߂�l�@�����F0�A�s���܂��͗e�ʃI�[�o�[�F-1�A�󂫂�����͂��Ȃ̂Ɏ��s�F1

	//���݂̃o�b�t�@�ʒu�̕������I�[�܂��͉��s�̏ꍇ�A�}�����[�h
	if(ix > bp->n) return -1; //�s���w��
	while(ix >= bp->n){
		if(bp->next==NULL){
			//�e�L�X�g�S�̍Ō���̏ꍇ�͑}��
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
//�e�L�X�g�o�b�t�@bp�̐擪����k�o�C�g�̈ʒu��1�o�C�g�폜
	unsigned char *p;

	if(ix > bp->n) return; //�s���w��
	if(ix !=bp->n){
		//�o�b�t�@�̍Ō�̕��������łȂ��ꍇ
		for(p=bp->Buf+ix ; p< bp->Buf + bp->n-1 ; p++) *p=*(p+1);
		bp->n--;
		num--; //�o�b�t�@�g�p��
		return;
	}
	//�s�o�b�t�@�̌��݂̍Ō�̏ꍇ�i�폜���镶�����Ȃ��ꍇ�j
	if(bp->next==NULL) return; //�S�̂̍Ō�̏ꍇ�A�������Ȃ�
	deletechar(bp->next,0); //���̍s�̐擪�������폜
}
int gabagecollect1(void){
//�f�Љ����ꂽ�e�L�X�g�o�b�t�@�̌��Ԃ𖄂߂�K�x�[�W�R���N�V����
//�J�[�\���̑O�ƌ�낻�ꂼ��T�����čŏ���1�o�C�g���̂ݎ��{
//�߂�l 1�o�C�g�ł��ړ������ꍇ�F1�A�Ȃ������ꍇ�F0

	_tbuf *bp;
	int f=0;
	unsigned char *p,*p2;

	//�J�[�\�����o�b�t�@�̐擪�ɂ���ꍇ�A�O�̃o�b�t�@�̍Ō���ɕύX
	//�i�������O�ɋ󂫂��Ȃ��ꍇ�Ɛ擪�o�b�t�@�̏ꍇ�������j
	while(cursorix==0 && cursorbp->prev!=NULL && cursorbp->prev->n <TBUFSIZE){
		cursorbp=cursorbp->prev;
		cursorix=cursorbp->n;
	}
	//��ʍ���ʒu���o�b�t�@�̐擪�ɂ���ꍇ�A�O�̃o�b�t�@�̍Ō���ɕύX
	//�i�������擪�o�b�t�@�̏ꍇ�������j
	while(disptopix==0 && disptopbp->prev!=NULL){
		disptopbp=disptopbp->prev;
		disptopix=disptopbp->n;
	}
	//�J�[�\���̂���o�b�t�@�ȊO�̋�o�b�t�@��S�č폜
	bp=TBufstart;
	while(bp!=NULL){
		if(bp->n == 0 && bp!=cursorbp) bp=deleteTBuf(bp); //�󂫃o�b�t�@�폜
		else bp=bp->next;
	}

	//�J�[�\���ʒu���O�̖��܂��Ă��Ȃ��o�b�t�@��擪����T�[�`
	bp=TBufstart;
	while(bp->n >= TBUFSIZE){
		if(bp==cursorbp) break;
		bp=bp->next;
	}
	if(bp!=cursorbp){
		//�ŏ��Ɍ������󂫏ꏊ�Ɏ��̃o�b�t�@����1�o�C�g�ړ�
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
	if(cursorbp->next ==NULL) return f; //�J�[�\���ʒu���ŏI�o�b�t�@�Ȃ�I��
	//�J�[�\���ʒu�̎��̃o�b�t�@���疄�܂��Ă��Ȃ��o�b�t�@���T�[�`
	bp=cursorbp;
	do{
		bp=bp->next;
		if(bp->next ==NULL) return f; //�ŏI�o�b�t�@�ɓ��B�Ȃ�I��
	} while(bp->n >=TBUFSIZE);

	//�ŏ��Ɍ������󂫏ꏊ�Ɏ��̃o�b�t�@����1�o�C�g�ړ�
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
// �ω����Ȃ��Ȃ�܂�1�o�C�g���̃K�x�[�W�R���N�V�������Ăяo��
	while(gabagecollect1()) ;
}
void inittextbuf(void){
// �e�L�X�g�o�b�t�@�̏�����
	_tbuf *bp;
	for(bp=TextBuffer;bp<TextBuffer+TBUFMAXLINE;bp++) bp->prev=NULL; //���g�p�o�b�t�@��
	TBufstart=TextBuffer; //�����N�̐擪�ݒ�
	TBufstart->next=NULL;
	TBufstart->n=0;
	num=0; //�o�b�t�@�g�p��
	edited=0; //�ҏW�ς݃t���O�N���A
}
void redraw(){
//��ʂ̍ĕ`��
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
		//�͈͑I�����[�h�łȂ��ꍇ
		bp1=NULL;
		bp2=NULL;
	}
	else{
		//�͈͑I�����[�h�̏ꍇ�A�J�n�ʒu�ƏI���̑O�㔻�f����
		//bp1,ix1���J�n�ʒu�Abp2,ix2���I���ʒu�ɐݒ�
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
			//����������ʒu�܂ŃT�[�`
			while(ix>=bp->n){
				if(bp==bp1 && ix==ix1) cl=COLOR_AREASELECTTEXT;
				if(bp==bp2 && ix==ix2) cl=COLOR_NORMALTEXT;
				bp=bp->next;
				ix=0;
				if(bp==NULL) break;
			}
			if(bp==NULL) break; //�o�b�t�@�ŏI
			if(bp==bp1 && ix==ix1) cl=COLOR_AREASELECTTEXT;
			if(bp==bp2 && ix==ix2) cl=COLOR_NORMALTEXT;
			ch=bp->Buf[ix++];
			if(ch=='\n') break;
			*(vp+ATTROFFSET)=cl;
			*vp++=ch;
		}
		//���s����уo�b�t�@�ŏI�ȍ~�̉E���\������
		for(;x<EDITWIDTHX;x++){
			*(vp+ATTROFFSET)=0;
			*vp++=0;
		}
	}
	//�o�b�t�@�ŏI�ȍ~�̉����\������
	for(;y<EDITWIDTHY;y++){
		for(x=0;x<EDITWIDTHX;x++){
			*(vp+ATTROFFSET)=0;
			*vp++=0;
		}
	}
}

void cursor_left(void){
//�J�[�\����1�O�Ɉړ�
//�o�́F���L�ϐ����ړ���̒l�ɕύX
//cursorbp,cursorix �o�b�t�@��̃J�[�\���ʒu
//cx,cy ��ʏ�̃J�[�\���ʒu
//cx2 cx�Ɠ���
//disptopbp,disptopix ��ʍ���̃o�b�t�@��̈ʒu

	_tbuf *bp;
	int ix;
	int i;
	int x;

	//�o�b�t�@��̃J�[�\���ʒu��1�O�Ɉړ�
	if(cursorix!=0) cursorix--;
	else while(1) {
		//1�O�̃o�b�t�@�̍Ō���Ɉړ��A��������o�b�t�@�͔�΂�
		if(cursorbp->prev==NULL) return; //�e�L�X�g�S�̐擪�Ȃ̂ňړ����Ȃ�
		cursorbp=cursorbp->prev;
		if(cursorbp->n >0){
			cursorix=cursorbp->n-1;//�o�b�t�@�Ō��
			break;
		}
	}

	//�J�[�\������щ�ʍ���ʒu�̍X�V
	if(cx>0){
		//���[�łȂ���΃J�[�\����P����1���Ɉړ����ďI��
		cx--;
		cx2=cx;
		return;
	}
	if(cy>0){
		//���[������[�ł͂Ȃ��ꍇ
		if(cursorbp->Buf[cursorix]!='\n'){
			// �ړ��悪���s�R�[�h�łȂ��ꍇ�A�J�[�\����1��̍s�̉E�[�Ɉړ�
			cx=EDITWIDTHX-1;
			cx2=cx;
			cy--;
			return;
		}
		//��ʍ���ʒu����Ō����X���W���T�[�`
		bp=disptopbp;
		ix=disptopix;
		x=0;
		while(ix!=cursorix || bp!=cursorbp){
			if(bp->n==0){
				//��o�b�t�@�̏ꍇ����
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

	//���[����[�̏ꍇ
	if(cursorbp->Buf[cursorix]!='\n'){
		// �ړ��悪���s�R�[�h�łȂ��ꍇ�A�J�[�\���͉E�[�Ɉړ�
		// ��ʍ���ʒu�͉�ʉ������O�Ɉړ�
		cx=EDITWIDTHX-1;
		cx2=cx;
	}
	else{
		//�ړ��悪���s�R�[�h�̏ꍇ
		//�s���i���s�̎��̕����܂��̓o�b�t�@�擪�j�ƌ��݈ʒu�̕���������
		//��ʉ����Ŋ������]�肪�J�[�\��X���W
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
	//��ʍ���ʒu�͌��݈ʒu����X���W���������Ƃ���
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
//�J�[�\����1���Ɉړ�
//�o�́F���L�ϐ����ړ���̒l�ɕύX
//cursorbp,cursorix �o�b�t�@��̃J�[�\���ʒu
//cx,cy ��ʏ�̃J�[�\���ʒu
//cx2 cx�Ɠ���
//disptopbp,disptopix ��ʍ���̃o�b�t�@��̈ʒu

	_tbuf *bp;
	int ix;
	int i;
	int x;
	unsigned char c;

	if(cursorix >= cursorbp->n){
		//�o�b�t�@�Ō���̏ꍇ�A���̐擪�Ɉړ�
		bp=cursorbp;
		while(1) {
			//��o�b�t�@�͔�΂�
			if(bp->next==NULL) return; //�e�L�X�g�S�̍Ō���Ȃ̂ňړ����Ȃ�
			bp=bp->next;
			if(bp->n >0) break;
		}
		cursorbp=bp;
		cursorix=0;//�o�b�t�@�擪
	}
	c=cursorbp->Buf[cursorix++]; //�o�b�t�@��̃J�[�\���ʒu�̃R�[�h��ǂ��1���Ɉړ�
	if(c!='\n' && cx<EDITWIDTHX-1){
		//�J�[�\���ʒu�����s�ł��E�[�ł��Ȃ��ꍇ�P����1�E�Ɉړ����ďI��
		cx++;
		cx2=cx;
		return;
	}
	cx=0; //�J�[�\�����E�[�Ɉړ�
	cx2=cx;
	if(cy<EDITWIDTHY-1){
		//���[�łȂ���΃J�[�\�������s�Ɉړ����ďI��
		cy++;
		return;
	}
	//���[�̏ꍇ
	//��ʍ���ʒu���X�V
	//���s�R�[�h�܂��͉�ʉ���������܂ŃT�[�`
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
//�J�[�\����1��Ɉړ�
//�o�́F���L�ϐ����ړ���̒l�ɕύX
//cursorbp,cursorix �o�b�t�@��̃J�[�\���ʒu
//cx,cy ��ʏ�̃J�[�\���ʒu
//cx2 �ړ��O��cx�Ɠ���
//disptopbp,disptopix ��ʍ���̃o�b�t�@��̈ʒu

	_tbuf *bp;
	int ix;
	int i;
	int x;
	unsigned char c;

	//��ʕ����O�ɖ߂����Ƃ��낪�o�b�t�@��J�[�\���̈ړ���
	//�r���ŉ��s�R�[�h������Εʂ̎�i�Ō���
	bp=cursorbp;
	ix=cursorix;
	i=cx2-cx;
	while(i<EDITWIDTHX){
		if(ix==0){
			if(bp->prev==NULL) return; //�o�b�t�@�擪�܂ŃT�[�`������ړ��Ȃ�
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
	//��ʕ��̊Ԃɉ��s�R�[�h���Ȃ������ꍇ
	if(i==EDITWIDTHX){
		cx=cx2;
		//��ʏ�[�łȂ���΃J�[�\����1��Ɉړ����ďI��
		if(cy>0){
			cy--;
			return;
		}
		//��ʏ�[�̏ꍇ�A�J�[�\���ʒu����X���W���߂����Ƃ��낪��ʍ���ʒu
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
	//���s�����������ꍇ
	//�s���i���s�̎��̕����܂��̓o�b�t�@�擪�j�ƌ��݈ʒu�̕���������
	//��ʉ����Ŋ������]������߂�
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
	x=i % EDITWIDTHX; //���s�u���b�N�̍ŏI�s�̉E�[
	bp=cursorbp;
	ix=cursorix;
	//�o�b�t�@��̃J�[�\���ʒu�͉��s�u���b�N�̍ŏI�s�E�[����J�[�\��X���W���߂�
	//�ŏI�s�E�[�̂ق����������ꍇ�A���̏ꏊ���o�b�t�@��̃J�[�\���ʒu�Ƃ���
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
	cx=x; //cx2�܂��͉��s�u���b�N�ŏI�s�E�[
	if(cy>0){
		//��ʏ�[�łȂ���΃J�[�\����1��Ɉړ����ďI��
		cy--;
		return;
	}
	//��ʏ�[�̏ꍇ
	//��ʍ���ʒu�͌��݈ʒu����X���W���������Ƃ���
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
//�J�[�\����1���Ɉړ�
//�o�́F���L�ϐ����ړ���̒l�ɕύX
//cursorbp,cursorix �o�b�t�@��̃J�[�\���ʒu
//cx,cy ��ʏ�̃J�[�\���ʒu
//cx2 �ړ��O��cx�Ɠ���
//disptopbp,disptopix ��ʍ���̃o�b�t�@��̈ʒu

	_tbuf *bp;
	int ix;
	int x;
	unsigned char c;

	//���s�̐擪�T�[�`
	//�J�[�\���ʒu�����ʉE�[�܂ł̊Ԃɉ��s�R�[�h������Ύ��̕������擪
	bp=cursorbp;
	ix=cursorix;
	x=cx;
	while(x<EDITWIDTHX){
		if(ix>=bp->n){
			if(bp->next==NULL) return; //�o�b�t�@�Ō�܂ŃT�[�`������ړ��Ȃ�
			bp=bp->next;
			ix=0;
			continue;
		}
		c=bp->Buf[ix];
		ix++;
		x++;
		if(c=='\n') break;
	}
	//���s�擪����cx2�����������ɃT�[�`
	x=0;
	while(x<cx2){
		if(ix>=bp->n){
			if(bp->next==NULL) break; //�o�b�t�@�Ō�̏ꍇ�����Ɉړ�
			bp=bp->next;
			ix=0;
			continue;
		}
		if(bp->Buf[ix]=='\n') break; //���s�R�[�h�̏ꍇ�����Ɉړ�
		ix++;
		x++;
	}
	cursorbp=bp;
	cursorix=ix;
	cx=x;
	//��ʉ��[�łȂ���΃J�[�\����1���Ɉړ����ďI��
	if(cy<EDITWIDTHY-1){
		cy++;
		return;
	}
	//���[�̏ꍇ
	//��ʍ���ʒu���X�V
	//���s�R�[�h�܂��͉�ʉ���������܂ŃT�[�`
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
//�J�[�\�����s�擪�Ɉړ�
//�o�́F���L�ϐ����ړ���̒l�ɕύX
//cursorbp,cursorix �o�b�t�@��̃J�[�\���ʒu
//cx,cx2 0
//cy �ύX�Ȃ�
//disptopbp,disptopix ��ʍ���̃o�b�t�@��̈ʒu�i�ύX�Ȃ��j

	//�J�[�\��X���W���O�Ɉړ�
	while(cx>0){
		if(cursorix==0){
			//��o�b�t�@�͔�΂�
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
//�J�[�\�����s���Ɉړ�
//�o�́F���L�ϐ����ړ���̒l�ɕύX
//cursorbp,cursorix �o�b�t�@��̃J�[�\���ʒu
//cx,cx2 �s��
//cy �ύX�Ȃ�
//disptopbp,disptopix ��ʍ���̃o�b�t�@��̈ʒu�i�ύX�Ȃ��j

	//�J�[�\��X���W����ʕ������Ɉړ�
	//���s�R�[�h�܂��̓o�b�t�@�ŏI������΂����Ɉړ�
	while(cx<EDITWIDTHX-1){
		if(cursorix>=cursorbp->n){
			//��o�b�t�@�͔�΂�
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
//PageUp�L�[
//�ŏ�s���ŉ��s�ɂȂ�܂ŃX�N���[��
//�o�́F���L�ϐ����ړ���̒l�ɕύX
//cursorbp,cursorix �o�b�t�@��̃J�[�\���ʒu
//cx,cx2
//cy
//disptopbp,disptopix ��ʍ���̃o�b�t�@��̈ʒu

	_tbuf *bp;
	int ix;
	int i;
	int cy_old;

	cy_old=cy;
	while(cy>0) cursor_up(); // cy==0�ɂȂ�܂ŃJ�[�\������Ɉړ�
	for(i=0;i<EDITWIDTHY-1;i++){
		//��ʍs��-1�s���J�[�\������Ɉړ�
		bp=disptopbp;
		ix=disptopix;
		cursor_up();
		if(bp==disptopbp && ix==disptopix) break; //�ŏ�s�ňړ��ł��Ȃ������ꍇ������
	}
	//����Y���W�܂ŃJ�[�\�������Ɉړ��A1�s�������Ȃ������ꍇ�͍ŏ�s�ɗ��܂�
	if(i>0) while(cy<cy_old) cursor_down();
}
void cursor_pagedown(void){
//PageDown�L�[
//�ŉ��s���ŏ�s�ɂȂ�܂ŃX�N���[��
//�o�́F���L�ϐ����ړ���̒l�ɕύX
//cursorbp,cursorix �o�b�t�@��̃J�[�\���ʒu
//cx,cx2
//cy
//disptopbp,disptopix ��ʍ���̃o�b�t�@��̈ʒu

	_tbuf *bp;
	int ix;
	int i;
	int y;
	int cy_old;

	cy_old=cy;
	while(cy<EDITWIDTHY-1){
		// cy==EDITWIDTH-1�ɂȂ�܂ŃJ�[�\�������Ɉړ�
		y=cy;
		cursor_down();
		if(y==cy) break;// �o�b�t�@�ŉ��s�ňړ��ł��Ȃ������ꍇ������
	}
	for(i=0;i<EDITWIDTHY-1;i++){
		//��ʍs��-1�s���J�[�\�������Ɉړ�
		bp=disptopbp;
		ix=disptopix;
		cursor_down();
		if(bp==disptopbp && ix==disptopix) break; //�ŉ��s�ňړ��ł��Ȃ������ꍇ������
	}
	//���[���炳��Ɉړ������s�����A�J�[�\������Ɉړ��A1�s�������Ȃ������ꍇ�͍ŉ��s�ɗ��܂�
	if(i>0) while(cy>cy_old) cursor_up();
}
void cursor_top(void){
//�J�[�\�����e�L�X�g�o�b�t�@�̐擪�Ɉړ�
	cursorbp=TBufstart;
	cursorix=0;
	cursorbp1=NULL; //�͈͑I�����[�h����
	disptopbp=cursorbp;
	disptopix=cursorix;
	cx=0;
	cx2=0;
	cy=0;
}

int countarea(void){
//�e�L�X�g�o�b�t�@�̎w��͈͂̕��������J�E���g
//�͈͂�(cursorbp,cursorix)��(cursorbp1,cursorix1)�Ŏw��
//��둤�̈�O�̕����܂ł��J�E���g
	_tbuf *bp1,*bp2;
	int ix1,ix2;
	int n;

	//�͈͑I�����[�h�̏ꍇ�A�J�n�ʒu�ƏI���̑O�㔻�f����
	//bp1,ix1���J�n�ʒu�Abp2,ix2���I���ʒu�ɐݒ�
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
//�e�L�X�g�o�b�t�@�̎w��͈͂��폜
//�͈͂�(cursorbp,cursorix)��(cursorbp1,cursorix1)�Ŏw��
//��둤�̈�O�̕����܂ł��폜
//�폜��̃J�[�\���ʒu�͑I��͈͂̐擪�ɂ��A�͈͑I�����[�h��������

	_tbuf *bp;
	int ix;
	int n;

	n=countarea(); //�I��͈͂̕������J�E���g

	//�͈͑I���̊J�n�ʒu�ƏI���ʒu�̑O��𔻒f���ăJ�[�\�����J�n�ʒu�ɐݒ�
	if(cy>cy1 || (cy==cy1 && cx>cx1)){
		cursorbp=cursorbp1;
		cursorix=cursorix1;
		cx=cx1;
		cy=cy1;
	}
	cx2=cx;
	cursorbp1=NULL; //�͈͑I�����[�h����

	//bp,ix���J�n�ʒu�ɐݒ�
	bp=cursorbp;
	ix=cursorix;

	//�I��͈͂��ŏ��̃o�b�t�@�̍Ō�܂ł���ꍇ
	if(n>=(bp->n - ix)){
		n -= bp->n - ix; //�폜��������
		num-=bp->n - ix; //�o�b�t�@�g�p�ʂ�����
		bp->n=ix; //ix�ȍ~���폜
		bp=bp->next;
		if(bp==NULL) return;
		ix=0;
	}
	//���̃o�b�t�@�ȍ~�A�I��͈͂̏I���ʒu���܂܂�Ȃ��o�b�t�@�͍폜
	while(n>=bp->n){
		n-=bp->n; //�폜��������
		num-=bp->n; //�o�b�t�@�g�p�ʂ�����
		bp=deleteTBuf(bp); //�o�b�t�@�폜���Ď��̃o�b�t�@�ɐi��
		if(bp==NULL) return;
	}
	//�I��͈͂̏I���ʒu���܂ޏꍇ�A1�������폜
	while(n>0){
		deletechar(bp,ix); //�o�b�t�@����1�����폜�inum�͊֐�����1�������j
		n--;
	}
}
void clipcopy(void){
// �I��͈͂��N���b�v�{�[�h�ɃR�s�[
	_tbuf *bp1,*bp2;
	int ix1,ix2;
	char *ps,*pd;

	//�͈͑I�����[�h�̏ꍇ�A�J�n�ʒu�ƏI���̑O�㔻�f����
	//bp1,ix1���J�n�ʒu�Abp2,ix2���I���ʒu�ɐݒ�
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
// �N���b�v�{�[�h����\��t��
	int n,i;
	unsigned char *p;

	p=clipboard;
	for(n=clipsize;n>0;n--){
		i=insertchar(cursorbp,cursorix,*p);
		if(i>0){
			//�o�b�t�@�󂫂�����̂ɑ}�����s�̏ꍇ
			gabagecollect2(); //�S�̃K�x�[�W�R���N�V����
			i=insertchar(cursorbp,cursorix,*p);//�e�L�X�g�o�b�t�@�ɂP�����}��
		}
		if(i!=0) return;//�}�����s
		cursor_right();//��ʏ�A�o�b�t�@��̃J�[�\���ʒu��1���Ɉړ�
		p++;
	}
}
void set_areamode(){
//�͈͑I�����[�h�J�n���̃J�[�\���J�n�ʒu�O���[�o���ϐ��ݒ�
	cursorbp1=cursorbp;
	cursorix1=cursorix;
	cx1=cx;
	cy1=cy;
}
void save_cursor(void){
//�J�[�\���֘A�O���[�o���ϐ����ꎞ���
	cursorbp_t=cursorbp;
	cursorix_t=cursorix;
	disptopbp_t=disptopbp;
	disptopix_t=disptopix;
	cx_t=cx;
	cy_t=cy;
}
void restore_cursor(void){
//�J�[�\���֘A�O���[�o���ϐ����ꎞ���ꏊ����߂�
	cursorbp=cursorbp_t;
	cursorix=cursorix_t;
	disptopbp=disptopbp_t;
	disptopix=disptopix_t;
	cx=cx_t;
	cy=cy_t;
}

int savetextfile(char *filename){
// �e�L�X�g�o�b�t�@���e�L�X�g�t�@�C���ɏ�������
// �������ݐ�����0�A���s�ŃG���[�R�[�h�i�����j��Ԃ�
	FSFILE *fp;
	_tbuf *bp;
	int ix,n,i,er;
	unsigned char *ps,*pd;
	er=0;//�G���[�R�[�h
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
		//���s�R�[�h��2�o�C�g�ɂȂ邱�Ƃ��l�����ăo�b�t�@�T�C�Y-1�܂łƂ���
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
				*pd++='\r'; //���s�R�[�h0A��0D 0A�ɂ���
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
// �e�L�X�g�t�@�C�����e�L�X�g�o�b�t�@�ɓǂݍ���
// �ǂݍ��ݐ�����0�A���s�ŃG���[�R�[�h�i�����j��Ԃ�
	FSFILE *fp;
	_tbuf *bp;
	int ix,n,i,er;
	unsigned char *ps,*pd;
	er=0;//�G���[�R�[�h
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
			if(*ps=='\r') ps++; //���s�R�[�h0D 0A��0A�ɂ���i�P����0D�����j
			else{
				*pd++=*ps++;
				ix++;
				num++;//�o�b�t�@��������
				if(num>TBUFMAXSIZE){
					er=ERR_FILETOOBIG;
					break;
				}
			}
		}
	} while(n==FILEBUFSIZE && er==0);
	if(bp!=NULL) bp->n=ix;//�Ō�̃o�b�t�@�̕�����
	FSfclose(fp);
	if(er) inittextbuf();//�G���[�����̏ꍇ�o�b�t�@�N���A
	return er;
}
void save_as(void){
// ���݂̃e�L�X�g�o�b�t�@�̓��e���t�@�C������t����SD�J�[�h�ɕۑ�
// �t�@�C�����̓O���[�o���ϐ�currentfile[]
// �t�@�C�����̓L�[�{�[�h����ύX�\
// ���������ꍇcurrentfile���X�V

	int er;
	unsigned char vk;
	unsigned char *ps,*pd;
	cls();
	setcursor(0,0,COLOR_NORMALTEXT);
	printstr("Save To SD Card\n");

	//currentfile����tempfile�ɃR�s�[
	ps=currentfile;
	pd=tempfile;
	while(*ps!=0) *pd++=*ps++;
	*pd=0;

	while(1){
		printstr("File Name + [Enter] / [ESC]\n");
		if(lineinput(tempfile,8+1+3)<0) return; //ESC�L�[�������ꂽ
		if(tempfile[0]==0) continue; //NULL������̏ꍇ
		printstr("Writing...\n");
		er=savetextfile(tempfile); //�t�@�C���ۑ��Aer:�G���[�R�[�h
		if(er==0){
			printstr("OK");
			//tempfile����currentfile�ɃR�s�[���ďI��
			ps=tempfile;
			pd=currentfile;
			while(*ps!=0) *pd++=*ps++;
			*pd=0;
			edited=0; //�ҏW�ς݃t���O�N���A
			wait60thsec(60);//1�b�҂�
			return;
		}
		setcursorcolor(COLOR_ERRORTEXT);
		if(er==ERR_CANTFILEOPEN) printstr("Bad File Name or File Error\n");
		else printstr("Cannot Write\n");
		setcursorcolor(COLOR_NORMALTEXT);
		printstr("Retry:[Enter] / Quit:[ESC]\n");
		while(1){
			inputchar(); //1�������͑҂�
			vk=vkey & 0xff;
			if(vk==VK_RETURN || vk==VK_SEPARATOR) break;
			if(vk==VK_ESCAPE) return;
		}
	}
}

int selectfile(void){
// SD�J�[�h����t�@�C����I�����ēǂݍ���
// currenfile[]�Ƀt�@�C�������L��
// �߂�l�@0�F�ǂݍ��݂��s�����@-1�F�ǂݍ��݂Ȃ�
	int filenum,i,er;
	unsigned char *ps,*pd;
	unsigned char x,y;
	unsigned char vk;
	SearchRec sr;

	//�t�@�C���̈ꗗ��SD�J�[�h����ǂݏo��
	cls();
	if(edited){
		//�ŏI�ۑ���ɕҏW�ς݂̏ꍇ�A�ۑ��̊m�F
		setcursorcolor(COLOR_NORMALTEXT);
		printstr("Save Editing File?\n");
		printstr("Save:[Enter] / Not Save:[ESC]\n");
		while(1){
			inputchar(); //1�����L�[���͑҂�
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
			inputchar(); //1�����L�[���͑҂�
			vk=vkey & 0xff;
			if(vk==VK_RETURN || vk==VK_SEPARATOR) break;
			if(vk==VK_ESCAPE) return -1;
		}
	}
	filenum=0;
	do{
		//filenames[]�Ƀt�@�C�����̈ꗗ��ǂݍ���
		ps=sr.filename;
		pd=filenames[filenum];
		while(*ps!=0) *pd++=*ps++;
		*pd=0;
		filenum++;
	}
	while(!FindNext(&sr) && filenum<MAXFILENUM);

	//�t�@�C���ꗗ����ʂɕ\��
	cls();
	setcursor(0,0,4);
	printstr("Select File + [Enter] / [ESC]\n");
	for(i=0;i<filenum;i++){
		x=(i&1)*15+1;
		y=i/2+1;
		setcursor(x,y,COLOR_NORMALTEXT);
		printstr(filenames[i]);
	}

	//�t�@�C���̑I��
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
		for(x=0;x<WIDTH_X-1;x++) printchar(' '); //�ŉ��s�̃X�e�[�^�X�\��������
		switch(vk){
			case VK_UP:
			case VK_NUMPAD8:
				//����L�[
				if(i>=2) i-=2;
				break;
			case VK_DOWN:
			case VK_NUMPAD2:
				//�����L�[
				if(i+2<filenum) i+=2;
				break;
			case VK_LEFT:
			case VK_NUMPAD4:
				//�����L�[
				if(i>0) i--;
				break;
			case VK_RIGHT:
			case VK_NUMPAD6:
				//�E���L�[
				if(i+1<filenum) i++;
				break;
			case VK_RETURN: //Enter�L�[
			case VK_SEPARATOR: //�e���L�[��Enter
				//�t�@�C��������B�ǂݍ���ŏI��
				er=loadtextfile(filenames[i]); //�e�L�X�g�o�b�t�@�Ƀt�@�C���ǂݍ���
				if(er==0){
					//currenfile[]�ϐ��Ƀt�@�C�������R�s�[���ďI��
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
				//ESC�L�[�A�t�@�C���ǂݍ��݂����I��
				return -1;
		}
		setcursor((i&1)*15,i/2+1,5);
		printchar(0x1c);// Right Arrow
		cursor--;
	}
}
void newtext(void){
// �V�K�e�L�X�g�쐬
	unsigned char vk;
	if(edited){
		//�ŏI�ۑ���ɕҏW�ς݂̏ꍇ�A�ۑ��̊m�F
		cls();
		setcursorcolor(COLOR_NORMALTEXT);
		printstr("Save Editing File?\n");
		printstr("Save:[Enter] / Not Save:[ESC]\n");
		while(1){
			inputchar(); //1�����L�[���͑҂�
			vk=vkey & 0xff;
			if(vk==VK_RETURN || vk==VK_SEPARATOR){
				save_as();
				break;
			}
			else if(vk==VK_ESCAPE) break;
		}
	}
	inittextbuf(); //�e�L�X�g�o�b�t�@������
	cursor_top(); //�J�[�\�����e�L�X�g�o�b�t�@�̐擪�ɐݒ�
	currentfile[0]=0; //��ƒ��t�@�C�����N���A
}
void displaybottomline(void){
//�G�f�B�^�[��ʍŉ��s�̕\��
	setcursor(0,WIDTH_Y-1,COLOR_BOTTOMLINE);
	printstr("F1:LOAD F2:SAVE   F4:NEW ");
	printnum2(TBUFMAXSIZE-num,5);
}
void normal_code_process(unsigned char k){
// �ʏ핶�����͏���
// k:���͂��ꂽ�����R�[�h
	int i;

	edited=1; //�ҏW�ς݃t���O
	if(insertmode || k=='\n' || cursorbp1!=NULL){ //�}�����[�h
		if(cursorbp1!=NULL) deletearea();//�I��͈͂��폜
		i=insertchar(cursorbp,cursorix,k);//�e�L�X�g�o�b�t�@�ɂP�����}��
		if(i>0){
			//�o�b�t�@�󂫂�����̂ɑ}�����s�̏ꍇ
			gabagecollect2(); //�S�̃K�x�[�W�R���N�V����
			i=insertchar(cursorbp,cursorix,k);//�e�L�X�g�o�b�t�@�ɂP�����}��
		}
		if(i==0) cursor_right();//��ʏ�A�o�b�t�@��̃J�[�\���ʒu��1���Ɉړ�
	}
	else{ //�㏑�����[�h
		i=overwritechar(cursorbp,cursorix,k);//�e�L�X�g�o�b�t�@�ɂP�����㏑��
		if(i>0){
			//�o�b�t�@�󂫂�����̂ɏ㏑���i�}���j���s�̏ꍇ
			//�i�s����o�b�t�@�Ō���ł͑}���j
			gabagecollect2(); //�S�̃K�x�[�W�R���N�V����
			i=overwritechar(cursorbp,cursorix,k);//�e�L�X�g�o�b�t�@�ɂP�����㏑��
		}
		if(i==0) cursor_right();//��ʏ�A�o�b�t�@��̃J�[�\���ʒu��1���Ɉړ�
	}
}
void control_code_process(unsigned char k,unsigned char sh){
// ���䕶�����͏���
// k:���䕶���̉��z�L�[�R�[�h
// sh:�V�t�g�֘A�L�[���

	save_cursor(); //�J�[�\���֘A�ϐ��ޔ��i�J�[�\���ړ��ł��Ȃ������ꍇ�߂����߁j
	switch(k){
		case VK_LEFT:
		case VK_NUMPAD4:
			 //�V�t�g�L�[�������Ă��Ȃ���Δ͈͑I�����[�h�����iNumLock�{�V�t�g�{�e���L�[�ł������j
			if((sh & CHK_SHIFT)==0 || (k==VK_NUMPAD4) && (sh & CHK_NUMLK)) cursorbp1=NULL;
			else if(cursorbp1==NULL) set_areamode(); //�͈͑I�����[�h�łȂ���Δ͈͑I�����[�h�J�n
			if(sh & CHK_CTRL){
				//CTRL�{������Home
				cursor_home();
				break;
			}
			cursor_left();
			if(cursorbp1!=NULL && (disptopbp!=disptopbp_t || disptopix!=disptopix_t)){
				//�͈͑I�����[�h�ŉ�ʃX�N���[�����������ꍇ
				if(cy1<EDITWIDTHY-1) cy1++; //�͈̓X�^�[�g�ʒu���X�N���[��
				else restore_cursor(); //�J�[�\���ʒu��߂��i��ʔ͈͊O�͈̔͑I���֎~�j
			}
			break;
		case VK_RIGHT:
		case VK_NUMPAD6:
			 //�V�t�g�L�[�������Ă��Ȃ���Δ͈͑I�����[�h�����iNumLock�{�V�t�g�{�e���L�[�ł������j
			if((sh & CHK_SHIFT)==0 || (k==VK_NUMPAD6) && (sh & CHK_NUMLK)) cursorbp1=NULL;
			else if(cursorbp1==NULL) set_areamode(); //�͈͑I�����[�h�łȂ���Δ͈͑I�����[�h�J�n
			if(sh & CHK_CTRL){
				//CTRL�{�E����End
				cursor_end();
				break;
			}
			cursor_right();
			if(cursorbp1!=NULL && (disptopbp!=disptopbp_t || disptopix!=disptopix_t)){
				//�͈͑I�����[�h�ŉ�ʃX�N���[�����������ꍇ
				if(cy1>0) cy1--; //�͈̓X�^�[�g�ʒu���X�N���[��
				else restore_cursor(); //�J�[�\���ʒu��߂��i��ʔ͈͊O�͈̔͑I���֎~�j
			}
			break;
		case VK_UP:
		case VK_NUMPAD8:
			 //�V�t�g�L�[�������Ă��Ȃ���Δ͈͑I�����[�h�����iNumLock�{�V�t�g�{�e���L�[�ł������j
			if((sh & CHK_SHIFT)==0 || (k==VK_NUMPAD8) && (sh & CHK_NUMLK)) cursorbp1=NULL;
			else if(cursorbp1==NULL) set_areamode(); //�͈͑I�����[�h�łȂ���Δ͈͑I�����[�h�J�n
			cursor_up();
			if(cursorbp1!=NULL && (disptopbp!=disptopbp_t || disptopix!=disptopix_t)){
				//�͈͑I�����[�h�ŉ�ʃX�N���[�����������ꍇ
				if(cy1<EDITWIDTHY-1) cy1++; //�͈̓X�^�[�g�ʒu���X�N���[��
				else restore_cursor(); //�J�[�\���ʒu��߂��i��ʔ͈͊O�͈̔͑I���֎~�j
			}
			break;
		case VK_DOWN:
		case VK_NUMPAD2:
			 //�V�t�g�L�[�������Ă��Ȃ���Δ͈͑I�����[�h�����iNumLock�{�V�t�g�{�e���L�[�ł������j
			if((sh & CHK_SHIFT)==0 || (k==VK_NUMPAD2) && (sh & CHK_NUMLK)) cursorbp1=NULL;
			else if(cursorbp1==NULL) set_areamode(); //�͈͑I�����[�h�łȂ���Δ͈͑I�����[�h�J�n
			cursor_down();
			if(cursorbp1!=NULL && (disptopbp!=disptopbp_t || disptopix!=disptopix_t)){
				//�͈͑I�����[�h�ŉ�ʃX�N���[�����������ꍇ
				if(cy1>0) cy1--; //�͈̓X�^�[�g�ʒu���X�N���[��
				else restore_cursor(); //�J�[�\���ʒu��߂��i��ʔ͈͊O�͈̔͑I���֎~�j
			}
			break;
		case VK_HOME:
		case VK_NUMPAD7:
			 //�V�t�g�L�[�������Ă��Ȃ���Δ͈͑I�����[�h�����iNumLock�{�V�t�g�{�e���L�[�ł������j
			if((sh & CHK_SHIFT)==0 || (k==VK_NUMPAD7) && (sh & CHK_NUMLK)) cursorbp1=NULL;
			else if(cursorbp1==NULL) set_areamode(); //�͈͑I�����[�h�łȂ���Δ͈͑I�����[�h�J�n
			cursor_home();
			break;
		case VK_END:
		case VK_NUMPAD1:
			 //�V�t�g�L�[�������Ă��Ȃ���Δ͈͑I�����[�h�����iNumLock�{�V�t�g�{�e���L�[�ł������j
			if((sh & CHK_SHIFT)==0 || (k==VK_NUMPAD1) && (sh & CHK_NUMLK)) cursorbp1=NULL;
			else if(cursorbp1==NULL) set_areamode(); //�͈͑I�����[�h�łȂ���Δ͈͑I�����[�h�J�n
			cursor_end();
			break;
		case VK_PRIOR: // PageUp�L�[
		case VK_NUMPAD9:
			 //�V�t�g�{PageUp�͖����iNumLock�{�V�t�g�{�u9�v�����j
			if((sh & CHK_SHIFT) && ((k!=VK_NUMPAD9) || ((sh & CHK_NUMLK)==0))) break;
			cursorbp1=NULL; //�͈͑I�����[�h����
			cursor_pageup();
			break;
		case VK_NEXT: // PageDown�L�[
		case VK_NUMPAD3:
			 //�V�t�g�{PageDown�͖����iNumLock�{�V�t�g�{�u3�v�����j
			if((sh & CHK_SHIFT) && ((k!=VK_NUMPAD3) || ((sh & CHK_NUMLK)==0))) break;
			cursorbp1=NULL; //�͈͑I�����[�h����
			cursor_pagedown();
			break;
		case VK_DELETE: //Delete�L�[
		case VK_DECIMAL: //�e���L�[�́u.�v
			edited=1; //�ҏW�ς݃t���O
			if(cursorbp1!=NULL) deletearea();//�I��͈͂��폜
			else deletechar(cursorbp,cursorix);
			break;
		case VK_BACK: //BackSpace�L�[
			edited=1; //�ҏW�ς݃t���O
			if(cursorbp1!=NULL){
				deletearea();//�I��͈͂��폜
				break;
			}
			if(cursorix==0 && cursorbp->prev==NULL) break; //�o�b�t�@�擪�ł͖���
			cursor_left();
			deletechar(cursorbp,cursorix);
			break;
		case VK_INSERT:
		case VK_NUMPAD0:
			insertmode^=1; //�}�����[�h�A�㏑�����[�h��؂�ւ�
			break;
		case 'C':
			//CTRL+C�A�N���b�v�{�[�h�ɃR�s�[
			if(cursorbp1!=NULL && (sh & CHK_CTRL)) clipcopy();
			break;
		case 'X':
			//CTRL+X�A�N���b�v�{�[�h�ɐ؂���
			if(cursorbp1!=NULL && (sh & CHK_CTRL)){
				clipcopy();
				deletearea(); //�I��͈͂̍폜
				edited=1; //�ҏW�ς݃t���O
			}
			break;
		case 'V':
			//CTRL+V�A�N���b�v�{�[�h����\��t��
			if((sh & CHK_CTRL)==0) break;
			if(clipsize==0) break;
			edited=1; //�ҏW�ς݃t���O
			if(cursorbp1!=NULL){
				//�͈͑I�����Ă��鎞�͍폜���Ă���\��t��
				if(num-countarea()+clipsize<=TBUFMAXSIZE){ //�o�b�t�@�󂫗e�ʃ`�F�b�N
					deletearea();//�I��͈͂��폜
					clippaste();//�N���b�v�{�[�h�\��t��
				}
			}
			else{
				if(num+clipsize<=TBUFMAXSIZE){ //�o�b�t�@�󂫗e�ʃ`�F�b�N
					clippaste();//�N���b�v�{�[�h�\��t��
				}
			}
			break;
		case 'S':
			//CTRL+S�ASD�J�[�h�ɕۑ�
			if((sh & CHK_CTRL)==0) break;
		case VK_F2: //F2�L�[
			save_as(); //�t�@�C������t���ĕۑ�
			break;
		case 'O':
			//CTRL+O�A�t�@�C���ǂݍ���
			if((sh & CHK_CTRL)==0) break;
		case VK_F1: //F1�L�[
			//F1�L�[�A�t�@�C���ǂݍ���
			if(selectfile()==0){ //�t�@�C����I�����ēǂݍ���
				//�ǂݍ��݂��s�����ꍇ�A�J�[�\���ʒu��擪��
				cursor_top();
			}
			break;
		case 'N':
			//CTRL+N�A�V�K�쐬
			if((sh & CHK_CTRL)==0) break;
		case VK_F4: //F4�L�[
			newtext(); //�V�K�쐬
			break;
	}
}
void texteditor(void){
//�e�L�X�g�G�f�B�^�[�{��
	unsigned char k1,k2,sh;

	inittextbuf(); //�e�L�X�g�o�b�t�@������
	currentfile[0]=0; //��ƒ��t�@�C�����N���A
	cursor_top(); //�J�[�\�����e�L�X�g�o�b�t�@�̐擪�Ɉړ�
	insertmode=1; //0:�㏑���A1:�}��
	clipsize=0; //�N���b�v�{�[�h�N���A
	blinktimer=0; //�J�[�\���_�Ń^�C�}�[�N���A

	while(1){
		redraw();//��ʍĕ`��
		displaybottomline(); //��ʍŉ��s�Ƀt�@���N�V�����L�[�@�\�\��
		setcursor(cx,cy,COLOR_NORMALTEXT);
		getcursorchar(); //�J�[�\���ʒu�̕�����ޔ��i�J�[�\���_�ŗp�j
		while(1){
			//�L�[���͑҂����[�v
			wait60thsec(1);  //60����1�b�E�F�C�g
			blinkcursorchar(); //�J�[�\���_�ł�����
			k1=ps2readkey(); //�L�[�o�b�t�@����ǂݍ��݁Ak1:�ʏ핶�����͂̏ꍇASCII�R�[�h
			if(vkey) break;  //�L�[�������ꂽ�ꍇ���[�v���甲����
			if(cursorbp1==NULL) gabagecollect1(); //1�o�C�g�K�x�[�W�R���N�V�����i�͈͑I�����͂��Ȃ��j
		}
		resetcursorchar(); //�J�[�\�������̕����\���ɖ߂�
		k2=(unsigned char)vkey; //k2:���z�L�[�R�[�h
		sh=vkey>>8;             //sh:�V�t�g�֘A�L�[���
		if(k2==VK_RETURN || k2==VK_SEPARATOR) k1='\n'; //Enter�����͒P���ɉ��s��������͂Ƃ���
		if(k1) normal_code_process(k1); //�ʏ핶�������͂��ꂽ�ꍇ
		else control_code_process(k2,sh); //���䕶�������͂��ꂽ�ꍇ
		if(cursorbp1!=NULL && cx==cx1 && cy==cy1) cursorbp1=NULL;//�I��͈͂̊J�n�ƏI�����d�Ȃ�����͈͑I�����[�h����
 	}
}
int main(void){
	/* �|�[�g�̏����ݒ� */
	TRISA = 0x0000; // PORTA�S�ďo��
	ANSELA = 0x0000; // �S�ăf�W�^��
	TRISB = KEYSTART | KEYFIRE | KEYUP | KEYDOWN | KEYLEFT | KEYRIGHT;// �{�^���ڑ��|�[�g���͐ݒ�
	ANSELB = 0x0000; // �S�ăf�W�^��
	CNPUBSET=KEYSTART | KEYFIRE | KEYUP | KEYDOWN | KEYLEFT | KEYRIGHT;// �v���A�b�v�ݒ�
	ODCB = 0x0300;	//RB8,RB9�̓I�[�v���h���C��

    // ���Ӌ@�\�s�����蓖��
	SDI2R=2; //RPA4:SDI2
	RPB5R=4; //RPB5:SDO2

	ps2mode(); //RA1�I���iPS/2�L�����}�N���j
	init_composite(); // �r�f�I�������N���A�A���荞�ݏ������A�J���[�r�f�I�o�͊J�n
	setcursor(0,0,COLOR_NORMALTEXT);

	printstr("Init PS/2...");
	if(ps2init()){ //PS/2������
		//�G���[�̏ꍇ�X�g�b�v
		setcursorcolor(COLOR_ERRORTEXT);
		printstr("\nPS/2 Error\n");
		printstr("Power Off and Check Keyboard\n");
//		while(1) asm("wait"); //�������[�v
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

	//SD�J�[�h�t�@�C���V�X�e��������
	while(1){
		setcursorcolor(COLOR_NORMALTEXT);
		printstr("Init File System...");
		// Initialize the File System
		if(FSInit()) break; //�t�@�C���V�X�e���������AOK�Ȃ甲����
		//�G���[�̏ꍇ��蒼��
		setcursorcolor(COLOR_ERRORTEXT);
		printstr("\nFile System Error\n");
		printstr("Insert Correct Card\n");
		printstr("And Hit Any Key\n");
		inputchar(); //1�����L�[���͑҂�
	}
	printstr("OK\n");
	wait60thsec(60); //1�b�҂�

	texteditor(); //�e�L�X�g�G�f�B�^�[�{�̌Ăяo��
}
