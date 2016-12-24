#define WIDTH_X	30 // ������������
#define WIDTH_Y	27 // �c����������
#define ATTROFFSET	(WIDTH_X*WIDTH_Y) // VRAM��̃J���[�p���b�g�i�[�ʒu

// ���̓{�^���̃|�[�g�A�r�b�g��`
#define KEYPORT PORTB
#define KEYUP 0x0400
#define KEYDOWN 0x0080
#define KEYLEFT 0x0100
#define KEYRIGHT 0x0200
#define KEYSTART 0x0800
#define KEYFIRE 0x4000


// �p���X���萔
#define V_NTSC		262					// 262�{/���
#define V_SYNC		10					// ���������{��
#define V_PREEQ		26					// �u�����L���O��ԏ㑤�iV_SYNC�{V_PREEQ�͋����Ƃ��邱�Ɓj
#define V_LINE		(WIDTH_Y*8)			// �摜�`����
#define H_NTSC		3632				// ��63.5��sec
#define H_SYNC		269					// �����������A��4.7��sec
#define H_CBST		304					// �J���[�o�[�X�g�J�n�ʒu�i�������������肩��j
#define H_BACK		339					// ���X�y�[�X�i�������������オ�肩��j


extern volatile char drawing;		//�@�\�����Ԓ���-1
extern volatile unsigned short drawcount;		//�@1��ʕ\���I�����Ƃ�1�����B�A�v������0�ɂ���B
							// �Œ�1��͉�ʕ\���������Ƃ̃`�F�b�N�ƁA�A�v���̏���������ʊ��ԕK�v���̊m�F�ɗ��p�B
extern volatile unsigned short LineCount;	// �������̍s

extern unsigned char TVRAM[]; //�e�L�X�g�r�f�I������

extern const unsigned char FontData[]; //�t�H���g�p�^�[����`
extern unsigned char *cursor;
extern unsigned char cursorcolor;

void start_composite(void); //�J���[�R���|�W�b�g�o�͊J�n
void stop_composite(void); //�J���[�R���|�W�b�g�o�͒�~
void init_composite(void); //�J���[�R���|�W�b�g�o�͏�����
void clearscreen(void); //��ʃN���A
void set_palette(unsigned char n,unsigned char b,unsigned char r,unsigned char g); //�p���b�g�ݒ�
void set_bgcolor(unsigned char b,unsigned char r,unsigned char g); //�o�b�N�O�����h�J���[�ݒ�

void vramscroll(void);
	//1�s�X�N���[��
void setcursor(unsigned char x,unsigned char y,unsigned char c);
	//�J�[�\���ʒu�ƃJ���[��ݒ�
void setcursorcolor(unsigned char c);
	//�J�[�\���ʒu���̂܂܂ŃJ���[�ԍ���c�ɐݒ�
void printchar(unsigned char n);
	//�J�[�\���ʒu�Ƀe�L�X�g�R�[�hn��1�����\�����A�J�[�\����1�����i�߂�
void printstr(unsigned char *s);
	//�J�[�\���ʒu�ɕ�����s��\��
void printnum(unsigned int n);
	//�J�[�\���ʒu�ɕ����Ȃ�����n��10�i���\��
void printnum2(unsigned int n,unsigned char e);
	//�J�[�\���ʒu�ɕ����Ȃ�����n��e����10�i���\���i�O�̋󂫌������̓X�y�[�X�Ŗ��߂�j
void cls(void);
	//��ʏ������A�J�[�\����擪�Ɉړ�