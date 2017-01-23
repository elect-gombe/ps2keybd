// PS/2�L�[�{�[�h�ǂݍ��݃v���O���� for PIC32MX by K.Tanaka�@Rev1.4�@2016.5.2
//  Rev 1.2 104�p��L�[�{�[�h�T�|�[�g�APause/Break�L�[�T�|�[�g
//  Rev 1.3 ���������ɓ��{��/�p��L�[�{�[�h��ށALock�L�[��Ԏw��ɑΉ�
//  Rev 1.4 �R�}���h�^�C���A�E�g����
//
// PIC32MX1xx/2xx��  DAT:RB8�ACLK:RB9
// PIC32MX370F512H�� DAT:RD6�ACLK:RD7
//

#include "ps2keyboard.h"
#include <plib.h>

// PIC32MX1xx/2xx�ŗ��p����ꍇ�́A�ȉ��̍s���R�����g�A�E�g����
//#define PIC32MX370F

#ifdef PIC32MX370F
#define PS2DATBIT  0x40 // RD6
#define PS2CLKBIT  0x80 // RD7
#define PORTPS2DAT PORTDbits.RD6
#define PORTPS2CLK PORTDbits.RD7
#define mPS2DATSET() (LATDSET=PS2DATBIT)
#define mPS2DATCLR() (LATDCLR=PS2DATBIT)
#define mPS2CLKSET()  (LATDSET=PS2CLKBIT)
#define mPS2CLKCLR()  (LATDCLR=PS2CLKBIT)
#define mTRISPS2DATIN()  (TRISDSET=PS2DATBIT)
#define mTRISPS2DATOUT() (TRISDCLR=PS2DATBIT)
#define mTRISPS2CLKIN()  (TRISDSET=PS2CLKBIT)
#define mTRISPS2CLKOUT() (TRISDCLR=PS2CLKBIT)
#define TIMER5_100us 597

#else
#define PS2DATBIT  0x100 // RB8
#define PS2CLKBIT  0x200 // RB9
#define PORTPS2DAT PORTBbits.RB8
#define PORTPS2CLK PORTBbits.RB9
#define mPS2DATSET() (LATBSET=PS2DATBIT)
#define mPS2DATCLR() (LATBCLR=PS2DATBIT)
#define mPS2CLKSET()  (LATBSET=PS2CLKBIT)
#define mPS2CLKCLR()  (LATBCLR=PS2CLKBIT)
#define mTRISPS2DATIN()  (TRISBSET=PS2DATBIT)
#define mTRISPS2DATOUT() (TRISBCLR=PS2DATBIT)
#define mTRISPS2CLKIN()  (TRISBSET=PS2CLKBIT)
#define mTRISPS2CLKOUT() (TRISBCLR=PS2CLKBIT)
#define TIMER5_100us 358

#endif

#define SCANCODEBUFSIZE 16 //�X�L�����R�[�h�o�b�t�@�̃T�C�Y
#define KEYCODEBUFSIZE 16 //�L�[�R�[�h�o�b�t�@�̃T�C�Y

// ����M�G���[�R�[�h
#define PS2ERROR_PARITY 1
#define PS2ERROR_STARTBIT 2
#define PS2ERROR_STOPBIT 3
#define PS2ERROR_BUFFERFUL 4
#define PS2ERROR_TIMEOUT 5
#define PS2ERROR_TOOMANYRESENDREQ 6
#define PS2ERROR_NOACK 7
#define PS2ERROR_NOBAT 8

//�R�}���h���M�󋵂�\��ps2status�̒l
#define PS2STATUS_SENDSTART 1
#define PS2STATUS_WAIT100us 2
#define PS2STATUS_SENDING 3
#define PS2STATUS_WAITACK 4
#define PS2STATUS_WAITBAT 5
#define PS2STATUS_INIT 6

#define PS2TIME_CLKDOWN 2     //�R�}���h���M����CLK��L�ɂ��鎞�ԁA200us(PS2STATUS_WAIT100us�̎���)
#define PS2TIME_PRESEND 2     //�R�}���h���M�O�̑҂����ԁA200us
#define PS2TIME_CMDTIMEOUT 110 //�R�}���h���M�^�C���A�E�g���ԁA11ms
#define PS2TIME_ACKTIMEOUT 50 //ACK�����^�C���A�E�g���ԁA5000us
#define PS2TIME_BATTIMEOUT 5000 //BAT�����^�C���A�E�g���ԁA500ms
#define PS2TIME_INIT 5000    //�d���I����̋N�����ԁA500ms
#define PS2TIME_RECEIVETIMEOUT 20 //�f�[�^��M�^�C���A�E�g���ԁA2000us

// PS/2�R�}���h
#define PS2CMD_RESET 0xff //�L�[�{�[�h���Z�b�g
#define PS2CMD_SETLED 0xed //���b�NLED�ݒ�
#define PS2CMD_RESEND 0xfe //�đ��v��
#define PS2CMD_ACK 0xfa //ACK����
#define PS2CMD_BAT 0xaa //�e�X�g����I��
#define PS2CMD_ECHO 0xee //�G�R�[
#define PS2CMD_OVERRUN 0x00 //�o�b�t�@�I�[�o�[����
#define PS2CMD_BREAK 0xf0 //�u���C�N�v���t�B�b�N�X

// �O���[�o���ϐ���`
unsigned int ps2sendcomdata;//���M�R�}���h�o�b�t�@�i�ő�4�o�C�g�A���ʂ��珇�Ɂj
unsigned char ps2sendcombyte;//���M�R�}���h�̖����M�o�C�g��
unsigned char volatile ps2status;//���M�A��M�̃t�F�[�Y
unsigned int volatile ps2statuscount;//ps2status�̃J�E���^�[�BTimer5���荞�݂��ƂɃJ�E���g�_�E��
unsigned int volatile ps2receivetime;//��M���̃^�C�}�[
unsigned char receivecount;//��M���r�b�g�J�E���^
unsigned char receivedata;//��M���f�[�^
unsigned char sendcount;//���M���r�b�g�J�E���^
unsigned short senddata;//���M���f�[�^
unsigned char volatile ps2sending; //PIC����KB�ɑ��M��
unsigned char volatile ps2receiveError; //�ǂݍ��ݎ��G���[
unsigned char volatile ps2sendError; //�R�}���h���M���G���[
unsigned char volatile ps2receivecommand;//��M�R�}���h
unsigned char volatile ps2receivecommandflag;//�R�}���h��M����
unsigned short volatile ps2shiftkey_a; //�V�t�g�A�R���g���[���L�[���̏�ԁi���E�L�[��ʁj
unsigned char volatile ps2shiftkey; //�V�t�g�A�R���g���[���L�[���̏�ԁi���E�L�[��ʂȂ��j

unsigned char scancodebuf[SCANCODEBUFSIZE]; //�X�L�����R�[�h�o�b�t�@
unsigned char * volatile scancodebufp1; //�X�L�����R�[�h�������ݐ擪�|�C���^
unsigned char * volatile scancodebufp2; //�X�L�����R�[�h�ǂݏo���擪�|�C���^
unsigned short keycodebuf[KEYCODEBUFSIZE]; //�L�[�R�[�h�o�b�t�@
unsigned short * volatile keycodebufp1; //�L�[�R�[�h�������ݐ擪�|�C���^
unsigned short * volatile keycodebufp2; //�L�[�R�[�h�ǂݏo���擪�|�C���^

//���J�ϐ�
unsigned volatile char ps2keystatus[256]; // ���z�R�[�h�ɑ�������L�[�̏�ԁiOn�̎�1�j
unsigned volatile short vkey; // ps2readkey()�֐��ŃZ�b�g�����L�[�R�[�h�A���8�r�b�g�̓V�t�g�֘A�L�[
unsigned char lockkey; // ����������Lock�L�[�̏�Ԏw��B����3�r�b�g��<CAPSLK><NUMLK><SCRLK>
unsigned char keytype; // �L�[�{�[�h�̎�ށB0�F���{��109�L�[�A1�F�p��104�L�[

// ���{��109key�z��
const unsigned char ps2scan2vktable1_jp[]={
	// PS/2�X�L�����R�[�h���牼�z�L�[�R�[�h�ւ̕ϊ��e�[�u��
	// 0x00-0x83�܂�
	0,VK_F9,0,VK_F5,VK_F3,VK_F1,VK_F2,VK_F12,0,VK_F10,VK_F8,VK_F6,VK_F4,VK_TAB,VK_KANJI,0,
	0,VK_LMENU,VK_LSHIFT,VK_KANA,VK_LCONTROL,'Q','1',0,0,0,'Z','S','A','W','2',0,
	0,'C','X','D','E','4','3',0,0,VK_SPACE,'V','F','T','R','5',0,
	0,'N','B','H','G','Y','6',0,0,0,'M','J','U','7','8',0,
	0,VK_OEM_COMMA,'K','I','O','0','9',0,0,VK_OEM_PERIOD,VK_OEM_2,'L',VK_OEM_PLUS,'P',VK_OEM_MINUS,0,
	0,VK_OEM_102,VK_OEM_1,0,VK_OEM_3,VK_OEM_7,0,0,VK_CAPITAL,VK_RSHIFT,VK_RETURN,VK_OEM_4,0,VK_OEM_6,0,0,
	0,0,0,0,VK_CONVERT,0,VK_BACK,VK_NONCONVERT,0,VK_NUMPAD1,VK_OEM_5,VK_NUMPAD4,VK_NUMPAD7,0,0,0,
	VK_NUMPAD0,VK_DECIMAL,VK_NUMPAD2,VK_NUMPAD5,VK_NUMPAD6,VK_NUMPAD8,VK_ESCAPE,VK_NUMLOCK,VK_F11,VK_ADD,VK_NUMPAD3,VK_SUBTRACT,VK_MULTIPLY,VK_NUMPAD9,VK_SCROLL,0,
	0,0,0,VK_F7
};
const unsigned char ps2scan2vktable2_jp[]={
	// PS/2�X�L�����R�[�h���牼�z�L�[�R�[�h�ւ̕ϊ��e�[�u��(E0�v���t�B�b�N�X)
	// 0x00-0x83�܂�
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,VK_RMENU,/*VK_LSHIFT*/ 0,0,VK_RCONTROL,0,0,0,0,0,0,0,0,0,0,VK_LWIN,
	0,0,0,0,0,0,0,VK_RWIN,0,0,0,0,0,0,0,VK_APPS,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,VK_DIVIDE,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,VK_SEPARATOR,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,VK_END,0,VK_LEFT,VK_HOME,0,0,0,
	VK_INSERT,VK_DELETE,VK_DOWN,0,VK_RIGHT,VK_UP,0,0,0,0,VK_NEXT,0,VK_SNAPSHOT,VK_PRIOR,VK_CANCEL,0,
	0,0,0,0
};
const unsigned char vk2asc1_jp[]={
	// ���z�L�[�R�[�h����ASCII�R�[�h�ւ̕ϊ��e�[�u���iSHIFT�Ȃ��j
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	' ' ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'0' ,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,'7' ,'8' ,'9' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,'a' ,'b' ,'c' ,'d' ,'e' ,'f' ,'g' ,'h' ,'i' ,'j' ,'k' ,'l' ,'m' ,'n' ,'o' ,
	'p' ,'q' ,'r' ,'s' ,'t' ,'u' ,'v' ,'w' ,'x' ,'y' ,'z' ,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'*' ,'+' ,0x00,'-' ,0x00,'/' ,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,':' ,';' ,',' ,'-', '.' ,'/' ,
	'@' ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'[' ,'\\',']' ,'^' ,0x00,
	0x00,0x00,'\\',0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
const unsigned char vk2asc2_jp[]={
	// ���z�L�[�R�[�h����ASCII�R�[�h�ւ̕ϊ��e�[�u���iSHIFT����j
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	' ' ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,'!' ,0x22,'#' ,'$' ,'%' ,'&' ,0x27,'(' ,')' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,'A' ,'B' ,'C' ,'D' ,'E' ,'F' ,'G' ,'H' ,'I' ,'J' ,'K' ,'L' ,'M' ,'N' ,'O' ,
	'P' ,'Q' ,'R' ,'S' ,'T' ,'U' ,'V' ,'W' ,'X' ,'Y' ,'Z' ,0x00,0x00,0x00,0x00,0x00,
	'0' ,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,'7' ,'8' ,'9' ,'*' ,'+' ,0x00,'-' ,'.' ,'/' ,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'*' ,'+' ,'<' ,'=' ,'>' ,'?' ,
	'`' ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'{' ,'|' ,'}' ,'~' ,0x00,
	0x00,0x00,'_' ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
const unsigned char vk2kana1[]={
	// ���z�L�[�R�[�h����J�i�R�[�h�ւ̕ϊ��e�[�u���iSHIFT�Ȃ��j
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	' ' ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xdc,0xc7,0xcc,0xb1,0xb3,0xb4,0xb5,0xd4,0xd5,0xd6,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0xc1,0xba,0xbf,0xbc,0xb2,0xca,0xb7,0xb8,0xc6,0xcf,0xc9,0xd8,0xd3,0xd0,0xd7,
	0xbe,0xc0,0xbd,0xc4,0xb6,0xc5,0xcb,0xc3,0xbb,0xdd,0xc2,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'*' ,'+' ,0x00,'-' ,0x00,'/' ,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xb9,0xda,0xc8,0xce,0xd9,0xd2,
	0xde,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xdf,0xb0,0xd1,0xcd,0x00,
	0x00,0x00,0xdb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
const unsigned char vk2kana2[]={
	// ���z�L�[�R�[�h����J�i�R�[�h�ւ̕ϊ��e�[�u���iSHIFT����j
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	' ' ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0xa6,0xc7,0xcc,0xa7,0xa9,0xaa,0xab,0xac,0xad,0xae,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0xc1,0xba,0xbf,0xbc,0xa8,0xca,0xb7,0xb8,0xc6,0xcf,0xc9,0xd8,0xd3,0xd0,0xd7,
	0xbe,0xc0,0xbd,0xc4,0xb6,0xc5,0xcb,0xc3,0xbb,0xdd,0xaf,0x00,0x00,0x00,0x00,0x00,
	'0', '1' ,'2' ,'3' ,'4' ,'5' ,'6' ,'7' ,'8' ,'9' ,'*' ,'+' ,0x00,'-' ,'.' ,'/' ,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xb9,0xda,0xa4,0xce,0xa1,0xa5,
	0xde,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xa2,0xb0,0xa3,0xcd,0x00,
	0x00,0x00,0xdb,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

// �p��104key�z��
const unsigned char ps2scan2vktable1_en[]={
	// PS/2�X�L�����R�[�h���牼�z�L�[�R�[�h�ւ̕ϊ��e�[�u��
	// 0x00-0x83�܂�
	0,VK_F9,0,VK_F5,VK_F3,VK_F1,VK_F2,VK_F12,0,VK_F10,VK_F8,VK_F6,VK_F4,VK_TAB,VK_OEM_3,0,
	0,VK_LMENU,VK_LSHIFT,0,VK_LCONTROL,'Q','1',0,0,0,'Z','S','A','W','2',0,
	0,'C','X','D','E','4','3',0,0,VK_SPACE,'V','F','T','R','5',0,
	0,'N','B','H','G','Y','6',0,0,0,'M','J','U','7','8',0,
	0,VK_OEM_COMMA,'K','I','O','0','9',0,0,VK_OEM_PERIOD,VK_OEM_2,'L',VK_OEM_1,'P',VK_OEM_MINUS,0,
	0,0,VK_OEM_7,0,VK_OEM_4,VK_OEM_PLUS,0,0,VK_CAPITAL,VK_RSHIFT,VK_RETURN,VK_OEM_6,0,VK_OEM_5,0,0,
	0,0,0,0,0,0,VK_BACK,0,0,VK_NUMPAD1,0,VK_NUMPAD4,VK_NUMPAD7,0,0,0,
	VK_NUMPAD0,VK_DECIMAL,VK_NUMPAD2,VK_NUMPAD5,VK_NUMPAD6,VK_NUMPAD8,VK_ESCAPE,VK_NUMLOCK,VK_F11,VK_ADD,VK_NUMPAD3,VK_SUBTRACT,VK_MULTIPLY,VK_NUMPAD9,VK_SCROLL,0,
	0,0,0,VK_F7
};
const unsigned char ps2scan2vktable2_en[]={
	// PS/2�X�L�����R�[�h���牼�z�L�[�R�[�h�ւ̕ϊ��e�[�u��(E0�v���t�B�b�N�X)
	// 0x00-0x83�܂�
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,VK_RMENU,/*VK_LSHIFT*/ 0,0,VK_RCONTROL,0,0,0,0,0,0,0,0,0,0,VK_LWIN,
	0,0,0,0,0,0,0,VK_RWIN,0,0,0,0,0,0,0,VK_APPS,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,VK_DIVIDE,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,VK_SEPARATOR,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,VK_END,0,VK_LEFT,VK_HOME,0,0,0,
	VK_INSERT,VK_DELETE,VK_DOWN,0,VK_RIGHT,VK_UP,0,0,0,0,VK_NEXT,0,VK_SNAPSHOT,VK_PRIOR,VK_CANCEL,0,
	0,0,0,0
};
const unsigned char vk2asc1_en[]={
	// ���z�L�[�R�[�h����ASCII�R�[�h�ւ̕ϊ��e�[�u���iSHIFT�Ȃ��j
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	' ' ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	'0' ,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,'7' ,'8' ,'9' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,'a' ,'b' ,'c' ,'d' ,'e' ,'f' ,'g' ,'h' ,'i' ,'j' ,'k' ,'l' ,'m' ,'n' ,'o' ,
	'p' ,'q' ,'r' ,'s' ,'t' ,'u' ,'v' ,'w' ,'x' ,'y' ,'z' ,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'*' ,'+' ,0x00,'-' ,0x00,'/' ,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,';' ,'=' ,',' ,'-' ,'.' ,'/' ,
	'`' ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'[' ,'\\',']' ,0x27,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};
const unsigned char vk2asc2_en[]={
	// ���z�L�[�R�[�h����ASCII�R�[�h�ւ̕ϊ��e�[�u���iSHIFT����j
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	' ' ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	')' ,'!' ,'@' ,'#' ,'$' ,'%' ,'^' ,'&' ,'*' ,'(' ,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,'A' ,'B' ,'C' ,'D' ,'E' ,'F' ,'G' ,'H' ,'I' ,'J' ,'K' ,'L' ,'M' ,'N' ,'O' ,
	'P' ,'Q' ,'R' ,'S' ,'T' ,'U' ,'V' ,'W' ,'X' ,'Y' ,'Z' ,0x00,0x00,0x00,0x00,0x00,
	'0' ,'1' ,'2' ,'3' ,'4' ,'5' ,'6' ,'7' ,'8' ,'9' ,'*' ,'+' ,0x00,'-' ,'.' ,'/' ,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,':' ,'+' ,'<' ,'_' ,'>' ,'?' ,
	'~' ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,'{' ,'|' ,'}' ,0x22,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};

void ps2receive(unsigned int data)
{
	unsigned char d;
	unsigned char c;

	if(ps2receiveError) return; // �G���[�������͎c��r�b�g����
	if(data&(0x1<<1)) return; //CLK�����オ��͖���
	d=data&(0x1<<0);
	if(receivecount==0){
		//�X�^�[�g�r�b�g
		if(d){
			//�X�^�[�g�r�b�g�G���[
			ps2receiveError=PS2ERROR_STARTBIT;
			return;
		}
		else{
			//�o�C�g��M�J�n
			receivecount++;
			ps2receivetime=0;//��M�^�C�}�[�J�E���g�J�n
		}
	}
	else if(receivecount<=8){
		//�f�[�^
		receivedata=(receivedata>>1)+(d<<7);
		receivecount++;
	}
	else if(receivecount==9){
		//�p���e�B
		c=receivedata;
		while(c){
			d^=c;
			c>>=1;
		}
		if((d&1)==0){
			//�p���e�B�G���[
			ps2receiveError=PS2ERROR_PARITY;
			return;
		}
		else receivecount++;
	}
	else{
		//�X�g�b�v�r�b�g
		if(d==0){
			//�X�g�b�v�r�b�g�G���[
			ps2receiveError=PS2ERROR_STOPBIT;
			return;
		}
		//�ǂݍ��݊���
		receivecount=0;
		if(receivedata==0 || receivedata==PS2CMD_BAT ||
			receivedata==PS2CMD_ECHO || receivedata>=PS2CMD_ACK){
			//�L�[�{�[�h����̃R�}���h��M�̏ꍇ
			ps2receivecommand=receivedata;
			ps2receivecommandflag=1;
		}
		else{
			//�f�[�^��M�̏ꍇ
			if((scancodebufp1+1==scancodebufp2) ||
				(scancodebufp1==scancodebuf+SCANCODEBUFSIZE-1)&&(scancodebufp2==scancodebuf)){
				//�o�b�t�@�t��
				ps2receiveError=PS2ERROR_BUFFERFUL;
				return;
			}
			*scancodebufp1++=receivedata;
			if(scancodebufp1>=scancodebuf+SCANCODEBUFSIZE) scancodebufp1=scancodebuf;
		}
	}
}
void ps2send()
{
	if(PORTPS2CLK){
		//CLK�����オ�莞
		if(sendcount==10){
			mTRISPS2DATIN(); //DAT����͂ɐݒ�
		}
		else if(sendcount==11){
			ps2sending=0;
		}
		return;
	}
// �ȉ�CLK�����莞����
	sendcount++;
	if(sendcount<=10){
		//�f�[�^�o��
		if(senddata & 1) mPS2DATSET();
		else mPS2DATCLR();
		senddata>>=1;
	}
/*
	else{
		//������DAT����=L�iACK�j�ƂȂ��Ă���͂�
		//�����ă`�F�b�N�͂��Ȃ�
	}
*/
}

#ifdef PIC32MX370F
void __ISR(33, ipl6) CNHandler(void)
{
	//CN���荞�݃n���h��
	unsigned int cnstatd;
	unsigned int volatile portd; //dummy

	cnstatd=CNSTATD;
	if(cnstatd){
		portd=PORTD; //dummy read
		if(cnstatd & PS2CLKBIT){
			// CLK���ω�������
			if(ps2sending) ps2send(); //�R�}���h���M����CLK�ω�
			else ps2receive(); //��M����CLK�ω�
		}
		mCNDClearIntFlag(); // CND���荞�݃t���O�N���A
	}
}
#else
void __ISR(34, ipl6) CNHandler(void)
{
	//CN���荞�݃n���h��
	unsigned int cnstatb;
	unsigned int volatile portb; //dummy

	cnstatb=CNSTATB;
	if(1){
		portb=PORTB; //dummy read
		if(cnstatb & PS2CLKBIT){
			// CLK���ω�������
			if(ps2sending) ps2send(); //�R�}���h���M����CLK�ω�
			else ps2receive(PORTB>>8); //��M����CLK�ω�
		}
		mCNBClearIntFlag(); // CNB���荞�݃t���O�N���A 
        //IFS1
	}
}
#endif

volatile unsigned char keyboard_rcvdata[16];
volatile unsigned int dma_readpt;

int getdata(unsigned int *data){
    while(dma_readpt != DCH3DPTR){
        ps2receive(keyboard_rcvdata[dma_readpt]);
        dma_readpt++;
        if(dma_readpt==16)dma_readpt=0;
        return 1;
    }
    return 0;
}

void dmainit(int ch){
    DmaChnOpen(ch, 0, DMA_OPEN_AUTO);

    DmaChnSetEventControl(ch, DMA_EV_START_IRQ(_CHANGE_NOTICE_B_IRQ));

    DmaChnSetTxfer(ch, 0xBF886121,keyboard_rcvdata,  1,sizeof (keyboard_rcvdata), 1);/*PORTB + 1*/
    
    DmaChnEnable(ch);
    
    DMACONbits.ON = 1;
}

void ps2statusprogress(){
	//�R�}���h���M��Ԃ�100�}�C�N���b���ƂɌĂяo��
	//ps2status�ő��M�󋵂�\��
	unsigned int parity;
	unsigned char c;
	unsigned int volatile dummy;

    
	ps2statuscount--;
	switch(ps2status){
		case PS2STATUS_INIT:
			//�V�X�e���N�����̈���҂�
			if(ps2statuscount==0) ps2status=0;
			return;

		case PS2STATUS_SENDSTART:
			//�R�}���h���M�X�^�[�g

			if(ps2statuscount) return;//�R�}���h���M�O�҂�����
			// CLK��CN���荞�ݖ���
#ifdef PIC32MX370F
			CNENDCLR=PS2CLKBIT;
#else
			CNENBCLR=PS2CLKBIT;
#endif
			mTRISPS2CLKOUT();//CLK���o�͂ɐݒ�
			asm volatile ("nop");
			mPS2CLKCLR(); //CLK=L�A��������100us�ȏ�p��������
			//��p���e�B����
			c=ps2sendcomdata;
			parity=1;
			while(c){
				parity^=c;
				c>>=1;
			}
			//���M�f�[�^�idata,parity,stop bit�j
			senddata=0x200+((parity&1)<<8)+(ps2sendcomdata & 0xff);
			receivecount=0;//������M�r���Ȃ狭���I��
			ps2sending=1;//���M���t���O
			sendcount=0;//���M�J�E���^
			ps2status=PS2STATUS_WAIT100us;
			ps2statuscount=PS2TIME_CLKDOWN;
			return;

		case PS2STATUS_WAIT100us:
			//CLK=L�ɂ���100us�҂����
			if(ps2statuscount) return;//100us�o�߂��Ă��Ȃ��ꍇ
			mTRISPS2DATOUT();//DAT���o�͂ɐݒ�
			asm volatile ("nop");
			mPS2DATCLR(); //start bit:0
			mTRISPS2CLKIN();//CLK����͂ɐݒ�
			asm volatile ("nop"); //1�N���b�N�ȏ�̃E�F�C�g�K�v
#ifdef PIC32MX370F
			dummy=PORTD;//���荞�ݖ��������̃|�[�g�ω����z��
			CNENDSET=PS2CLKBIT; // CLK�̕ω���CN���荞�ݗL��
#else
			dummy=PORTB;//���荞�ݖ��������̃|�[�g�ω����z��
			CNENBSET=PS2CLKBIT; // CLK�̕ω���CN���荞�ݗL��
#endif
			ps2status=PS2STATUS_SENDING; //���͑��M�����܂ő҂�
			ps2statuscount=PS2TIME_CMDTIMEOUT; //�^�C���A�E�g�G���[�̐ݒ�
			return;

		case PS2STATUS_SENDING:
			if(ps2sending==0){
				//���M����
				c=ps2sendcomdata;
				if(c==PS2CMD_RESEND){
					//�đ��v���R�}���h���M�̏ꍇ�͒ʏ��M��Ԃɖ߂�
					ps2status=0;
				}
				else{
					ps2status=PS2STATUS_WAITACK;//����ACK�����҂�
					ps2statuscount=PS2TIME_ACKTIMEOUT;
					ps2receiveError=0;
				}
			}
			else if(ps2statuscount==0){
				//�^�C���A�E�g����
				ps2sendError=PS2ERROR_TIMEOUT;
				ps2sending=0; //�ʏ��M��Ԃɖ߂�
				mTRISPS2DATIN(); //DAT����͂ɐݒ�
				ps2status=0;
			}
			return;

		case PS2STATUS_WAITACK:
			//ACK�����҂�
			if(ps2receivecommandflag){
				//�R�}���h��M����
				ps2receivecommandflag=0;
				if(ps2receivecommand==PS2CMD_RESEND){
					//�đ��v���������ꍇ�A�ŏ�����đ�����
					ps2status=PS2STATUS_SENDSTART;
					ps2statuscount=PS2TIME_PRESEND;
					ps2sending=1;
					return;
				}
				else if(ps2receivecommand==PS2CMD_ACK){
					//ACK��M����
					c=ps2sendcomdata;
					if(c==PS2CMD_RESET){
						//���Z�b�g�R�}���h�̏ꍇ�ABAT�ԐM�҂�
						ps2sendcombyte=0;//�R�}���h���M�o�b�t�@�N���A
						ps2status=PS2STATUS_WAITBAT;
						ps2statuscount=PS2TIME_BATTIMEOUT;
						ps2receiveError=0;
						return;
					}
				}
				else {
					//ACK�ł͂Ȃ������ꍇ
					//ECHO�Ȃǂ̏����͂����ɋL�q
				}
				if(--ps2sendcombyte){
					//�R�}���h���c���Ă���ꍇ�A���̃o�C�g���M
					ps2sendcomdata>>=8;
					ps2status=PS2STATUS_SENDSTART;
					ps2statuscount=PS2TIME_PRESEND;
					ps2sending=1;
					return;
				}
				//�S�ẴR�}���h����M����
				ps2status=0;
				ps2sendcombyte=0;
				return;
			}
			else if(ps2statuscount==0){
				//�����Ȃ��A�^�C���A�E�g
				ps2sendError=PS2ERROR_NOACK;
				ps2status=0;
				ps2sendcombyte=0;
			}
			return;

		case PS2STATUS_WAITBAT:
			if(ps2receivecommandflag){
				//�R�}���h��M����
				ps2receivecommandflag=0;
				if(ps2receivecommand!=PS2CMD_BAT){
					//BAT�i�Z���t�e�X�g�j���s�̏ꍇ
					ps2sendError=PS2ERROR_NOBAT;
				}
				ps2status=0;
			}
			else if(ps2statuscount==0){
				//BAT���������^�C���A�E�g
				ps2sendError=PS2ERROR_NOBAT;
				ps2status=0;
			}
			return;
	}
}
void ps2command(unsigned int d,unsigned char n){
// PS/2�L�[�{�[�h�ɑ΂��ăR�}���h���M
// d:�R�}���h
// n:�R�}���h�̃o�C�g���id�͉��ʂ���8�r�b�g���j

	if(ps2status) return; //�R�}���h���M���̏ꍇ�A������߂�
	ps2receivecommandflag=0; //�R�}���h��M�t���O�N���A
	ps2sendcomdata=d;
	ps2sendcombyte=n;
	ps2sendError=0;
	ps2status=PS2STATUS_SENDSTART; //����Timer5���荞�݂ő��M�J�n
	ps2statuscount=PS2TIME_PRESEND;
}

void shiftkeycheck(unsigned char vk,unsigned char breakflag){
// SHIFT,ALT,CTRL,Win�L�[�̉�����Ԃ��X�V
	unsigned short k;
	k=0;
	switch(vk){
		case VK_SHIFT:
		case VK_LSHIFT:
			k=CHK_SHIFT_L;
			break;
		case VK_RSHIFT:
			k=CHK_SHIFT_R;
			break;
		case VK_CONTROL:
		case VK_LCONTROL:
			k=CHK_CTRL_L;
			break;
		case VK_RCONTROL:
			k=CHK_CTRL_R;
			break;
		case VK_MENU:
		case VK_LMENU:
			k=CHK_ALT_L;
			break;
		case VK_RMENU:
			k=CHK_ALT_R;
			break;
		case VK_LWIN:
			k=CHK_WIN_L;
			break;
		case VK_RWIN:
			k=CHK_WIN_R;
			break;
	}
	if(breakflag) ps2shiftkey_a &= ~k;
	else ps2shiftkey_a |= k;

	ps2shiftkey &= CHK_SCRLK | CHK_NUMLK | CHK_CAPSLK;
	if(ps2shiftkey_a & (CHK_SHIFT_L | CHK_SHIFT_R)) ps2shiftkey|=CHK_SHIFT;
	if(ps2shiftkey_a & (CHK_CTRL_L | CHK_CTRL_R)) ps2shiftkey|=CHK_CTRL;
	if(ps2shiftkey_a & (CHK_ALT_L | CHK_ALT_R)) ps2shiftkey|=CHK_ALT;
	if(ps2shiftkey_a & (CHK_WIN_L | CHK_WIN_R)) ps2shiftkey|=CHK_WIN;
}
void lockkeycheck(unsigned char vk){
// NumLock,CapsLock,ScrollLock�̏�ԍX�V���A�C���W�P�[�^�R�}���h���s
	switch(vk){
		case VK_SCROLL:
		case VK_KANA:
			ps2shiftkey_a^=CHK_SCRLK_A;
			ps2shiftkey  ^=CHK_SCRLK;
			break;
		case VK_NUMLOCK:
			ps2shiftkey_a^=CHK_NUMLK_A;
			ps2shiftkey  ^=CHK_NUMLK;
			break;
		case VK_CAPITAL:
			if((ps2shiftkey & CHK_SHIFT)==0) return;
			ps2shiftkey_a^=CHK_CAPSLK_A;
			ps2shiftkey  ^=CHK_CAPSLK;
			break;
		default:
			return;
	}
	ps2command(PS2CMD_SETLED+(ps2shiftkey_a & 0xff00),2); // �L�[�{�[�h�̃C���W�P�[�^�ݒ�
}
int isShiftkey(unsigned char vk){
// SHIFT,ALT,WIN,CTRL�������ꂽ���̃`�F�b�N�i�����ꂽ�ꍇ-1��Ԃ��j
	switch(vk){
		case VK_SHIFT:
		case VK_LSHIFT:
		case VK_RSHIFT:
		case VK_CONTROL:
		case VK_LCONTROL:
		case VK_RCONTROL:
		case VK_MENU:
		case VK_LMENU:
		case VK_RMENU:
		case VK_LWIN:
		case VK_RWIN:
			return -1;
		default:
			return 0;
	}
}
int isLockkey(unsigned char vk){
// NumLock,SCRLock,CapsLock�������ꂽ���̃`�F�b�N�i�����ꂽ�ꍇ-1��Ԃ��j
	switch(vk){
		case VK_SCROLL:
		case VK_NUMLOCK:
		case VK_CAPITAL:
		case VK_KANA:
			return -1;
		default:
			return 0;
	}
}
void readscancode(){
// scancodebuf����X�L�����R�[�h��ǂݏo���A�L�[�R�[�h�ɕϊ�
// keycodebuf�ɂ��߂�
	int e0flag,breakflag;
	unsigned char d;
	unsigned char vk;
	unsigned char *p;

	e0flag=0;
	breakflag=0;
	p=scancodebufp2;
	while(1){
		//�v���t�B�b�N�X�iE0�AE1�A�u���C�N�j���I���܂Ń��[�v
		if(p==scancodebufp1) return;//�܂��s���S�ɂ����o�b�t�@�ɂ�����Ă��Ȃ�
		d=*p++;
		if(p==scancodebuf+SCANCODEBUFSIZE) p=scancodebuf;
		if(d==PS2CMD_BREAK) breakflag=1;
		else if(d==0xe0) e0flag=1;
		else if(d!=0xe1) break; //E1�͖���
	}
	scancodebufp2=p;
	if(d>0x83) return; //���Ή��X�L�����R�[�h
	if(e0flag){
		if(keytype==1) vk=ps2scan2vktable2_en[d];
		else vk=ps2scan2vktable2_jp[d];
	}
	else{
		if(keytype==1) vk=ps2scan2vktable1_en[d];
		else vk=ps2scan2vktable1_jp[d];
	}
	if(vk==0) return; //���Ή��X�L�����R�[�h

	if(isShiftkey(vk)){
		if(breakflag==0 && ps2keystatus[vk]) return; //�L�[���s�[�g�̏ꍇ�A����
		shiftkeycheck(vk,breakflag); //SHIFT�n�L�[�̃t���O����
	}
	else if(breakflag==0 && isLockkey(vk)){
		if(ps2keystatus[vk]) return; //�L�[���s�[�g�̏ꍇ�A����
		if((ps2shiftkey & CHK_CTRL)==0) lockkeycheck(vk); //NumLock�ACapsLock�AScrollLock���]����
	}
	//�L�[�R�[�h�ɑ΂��鉟����Ԕz����X�V
	if(breakflag){
		ps2keystatus[vk]=0;
		return;
	}
	ps2keystatus[vk]=1;

	if((keycodebufp1+1==keycodebufp2) ||
			(keycodebufp1==keycodebuf+KEYCODEBUFSIZE-1)&&(keycodebufp2==keycodebuf)){
		return; //�o�b�t�@�������ς��̏ꍇ����
	}
	*keycodebufp1++=((unsigned short)ps2shiftkey<<8)+vk;
	if(keycodebufp1==keycodebuf+KEYCODEBUFSIZE) keycodebufp1=keycodebuf;
}
void __ISR(20, ipl4) T5Handler(void)
{
    if(IEC1bits.CNBIE==0)
        getdata(0);
    else
        dma_readpt = DCH3DPTR;

	if(ps2status!=0) ps2statusprogress();
	if(receivecount){
		//�f�[�^��M���̃^�C���A�E�g�`�F�b�N
		//��莞�ԃf�[�^�����Ȃ��ꍇ�A������߂�
		ps2receivetime++;
		if(ps2receivetime>PS2TIME_RECEIVETIMEOUT) receivecount=0;
	}
	if(ps2receiveError){
		// ��M�G���[�������͍đ��v���R�}���h���s
		ps2receiveError=0;
		ps2command(PS2CMD_RESEND,1); //�đ��v���R�}���h
	}

	//�X�L�����R�[�h�o�b�t�@����ǂݏo���A�L�[�R�[�h�ɕϊ����ăL�[�R�[�h�o�b�t�@�Ɋi�[
	if(scancodebufp1!=scancodebufp2) readscancode();
	mT5ClearIntFlag(); // T5���荞�݃t���O�N���A
}

int ps2init()
{
// PS/2�L�[�{�[�h�V�X�e��������
// ����I����0��Ԃ�
// �G���[�I����-1��Ԃ�

	int i;

	receivecount=0;
	scancodebufp1=scancodebuf;
	scancodebufp2=scancodebuf;
	keycodebufp1=keycodebuf;
	keycodebufp2=keycodebuf;
	ps2status=0;
	ps2sending=0;
	ps2receiveError=0;
	ps2sendError=0;
	ps2receivecommandflag=0;
	ps2shiftkey_a=lockkey<<8; //Lock�֘A�L�[��ϐ�lockkey�ŏ�����
	ps2shiftkey=lockkey<<4; //Lock�֘A�L�[��ϐ�lockkey�ŏ�����
	for(i=0;i<256;i++) ps2keystatus[i]=0; //�S�L�[���������

	mTRISPS2CLKOUT();//CLK���o�͂ɐݒ�
	asm volatile ("nop");
	mPS2CLKCLR(); //CLK=L

	// Timer5���荞�ݗL����
	T5CON=0x0040; //Timer5 1:16 prescale
	PR5=TIMER5_100us;//100us����
	TMR5=0;
	mT5SetIntPriority(4); // ���荞�݃��x��4
	mT5ClearIntFlag();
	mT5IntEnable(1); // T5���荞�ݗL����
	T5CONSET=0x8000; //Timer5 Start

	// �d���I������̋N���҂�
	ps2status=PS2STATUS_INIT;
	ps2statuscount=PS2TIME_INIT;
	while(ps2status) ;
	// CN���荞�ݗL����
#ifdef PIC32MX370F
	mCNSetIntPriority(6); // ���荞�݃��x��6
	mCNDClearIntFlag();
	mCNDIntEnable(1); // CND���荞�ݗL����
	CNENDSET=PS2CLKBIT; // CLK�̕ω���CN���荞��
	CNCOND=0x8000; // PORTD�ɑ΂���CN���荞�݃I��
#else
	mCNSetIntPriority(6); // ���荞�݃��x��6
	mCNBClearIntFlag();
	mCNBIntEnable(1); // CNB���荞�ݗL����
	CNENBSET=PS2CLKBIT; // CLK�̕ω���CN���荞��
	CNCONB=0x8000; // PORTB�ɑ΂���CN���荞�݃I��
#endif
	ps2command(PS2CMD_RESET,1); // PS/2���Z�b�g�R�}���h���s���A�����҂�
	while(ps2status) ; //���M�����҂�
	if(ps2sendError) return -1; //�G���[����
	ps2command(PS2CMD_SETLED+(ps2shiftkey_a & 0xff00),2); // �L�[�{�[�h�̃C���W�P�[�^�ݒ�
	while(ps2status) ; //���M�����҂�
	if(ps2sendError) return -1; //�G���[����
    dmainit(DMA_CHANNEL3);
    IEC1bits.CNBIE=0;

	return 0;
}
unsigned char ps2readkey(){
// ���͂��ꂽ1�̃L�[�̃L�[�R�[�h���O���[�o���ϐ�vkey�Ɋi�[�i������Ă��Ȃ����0��Ԃ��j
// ����8�r�b�g�F�L�[�R�[�h
// ���8�r�b�g�F�V�t�g���
// �p���E�L�������̏ꍇ�A�߂�l�Ƃ���ASCII�R�[�h�i����ȊO��0��Ԃ��j

	unsigned short k;
	unsigned char sh;

	vkey=0;
	if(keycodebufp1==keycodebufp2) return 0;
	vkey=*keycodebufp2++;
	if(keycodebufp2==keycodebuf+KEYCODEBUFSIZE) keycodebufp2=keycodebuf;
	sh=vkey>>8;
	if(sh & (CHK_CTRL | CHK_ALT | CHK_WIN)) return 0;
	k=vkey & 0xff;
	if(keytype==1){
	//�p��L�[�{�[�h
		if(k>='A' && k<='Z'){
			//SHIFT�܂���CapsLock�i�����ł͂Ȃ��j
			if((sh & (CHK_SHIFT | CHK_CAPSLK))==CHK_SHIFT || (sh & (CHK_SHIFT | CHK_CAPSLK))==CHK_CAPSLK)
				return vk2asc2_en[k];
			else return vk2asc1_en[k];
		}
		if(k>=VK_NUMPAD0 && k<=VK_DIVIDE){ //�e���L�[
			if((sh & (CHK_SHIFT | CHK_NUMLK))==CHK_NUMLK) //NumLock�iSHIFT�{NumLock�͖����j
				return vk2asc2_en[k];
			else return vk2asc1_en[k];
		}
		if(sh & CHK_SHIFT) return vk2asc2_en[k];
		else return vk2asc1_en[k];
	}

	if(sh & CHK_SCRLK){
	//���{��L�[�{�[�h�i�J�i���[�h�j
		if(k>=VK_NUMPAD0 && k<=VK_DIVIDE){ //�e���L�[
			if((sh & (CHK_SHIFT | CHK_NUMLK))==CHK_NUMLK) //NumLock�iSHIFT�{NumLock�͖����j
				return vk2kana2[k];
			else return vk2kana1[k];
		}
		if(sh & CHK_SHIFT) return vk2kana2[k];
		else return vk2kana1[k];
	}

	//���{��L�[�{�[�h�i�p�����[�h�j
	if(k>='A' && k<='Z'){
		//SHIFT�܂���CapsLock�i�����ł͂Ȃ��j
		if((sh & (CHK_SHIFT | CHK_CAPSLK))==CHK_SHIFT || (sh & (CHK_SHIFT | CHK_CAPSLK))==CHK_CAPSLK)
			return vk2asc2_jp[k];
		else return vk2asc1_jp[k];
	}
	if(k>=VK_NUMPAD0 && k<=VK_DIVIDE){ //�e���L�[
		if((sh & (CHK_SHIFT | CHK_NUMLK))==CHK_NUMLK) //NumLock�iSHIFT�{NumLock�͖����j
			return vk2asc2_jp[k];
		else return vk2asc1_jp[k];
	}
	if(sh & CHK_SHIFT) return vk2asc2_jp[k];
	else return vk2asc1_jp[k];
}
unsigned char shiftkeys(){
// SHIFT�֘A�L�[�̉�����Ԃ�Ԃ�
// ��ʂ���<0><CAPSLK><NUMLK><SCRLK><Wiin><ALT><CTRL><SHIFT>
	return ps2shiftkey;
}
#ifndef PIC32MX370F
void ps2mode(){
	LATASET=2; //RA1�I���iPS/2���[�h�j
	T5CONSET=0x8000; //Timer5�ĊJ
	CNENBSET=PS2CLKBIT; //CLK���荞�ݍĊJ
}
void buttonmode(){
	CNENBCLR=PS2CLKBIT; //CLK���荞�ݒ�~
	T5CONCLR=0x8000; //Timer5��~
	LATACLR=2; //RA1�I�t�i�{�^�����[�h�j
}
#endif