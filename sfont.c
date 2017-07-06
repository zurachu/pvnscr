#include <stdarg.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <piece.h>

#include <smcvals.h>

#include "sfont.h"

const unsigned long SFONT_TBL1[32] = {		//�t�H���g�ϊ��p�e�[�u��(5bit��4byte)���n�ɍ�
	0x00000000, 0x02000000, 0x00020000, 0x03020000,
	0x00020100, 0x02020100, 0x00030100, 0x03030100,
	0x00000200, 0x02000200, 0x00020200, 0x03020200,
	0x00010300, 0x02010300, 0x00030300, 0x03030300,
	0x00000002, 0x02000002, 0x00020002, 0x03020002,
	0x00020102, 0x02020102, 0x00030102, 0x03030102,
	0x00000203, 0x02000203, 0x00020203, 0x03020203,
	0x00010303, 0x02010303, 0x00030303, 0x03030303
};

const unsigned long SFONT_TBL2[32] = {		//�t�H���g�ϊ��p�e�[�u��(5bit��4byte)���n�ɔ�
	0x03030303, 0x01030303, 0x03010303, 0x00010303,
	0x03010203, 0x01010203, 0x03000203, 0x00000203,
	0x03030103, 0x01030103, 0x03010103, 0x00010103,
	0x03020003, 0x01020003, 0x03000003, 0x00000003,
	0x03030301, 0x01030301, 0x03010301, 0x00010301,
	0x03010201, 0x01010201, 0x03000201, 0x00000201,
	0x03030100, 0x01030100, 0x03010100, 0x00010100,
	0x03020000, 0x01020000, 0x03000000, 0x00000000
};

const unsigned short SFONT_TBL3[32] = {		//�t�H���g�����p�e�[�u��
	0x0000, 0x0038, 0x001c, 0x003c,
	0x001e, 0x003e, 0x001e, 0x003e,
	0x000e, 0x003e, 0x001e, 0x003e,
	0x001e, 0x003e, 0x001e, 0x003e,
	0x0007, 0x003f, 0x001f, 0x003f,
	0x001f, 0x003f, 0x001f, 0x003f,
	0x000f, 0x003f, 0x001f, 0x003f,
	0x001f, 0x003f, 0x001f, 0x003f
};

typedef struct _KFONTHDR {
	unsigned short ofs;
	unsigned char s;
	unsigned char e;
} KFONTHDR;

static const unsigned char __nofont[] = {
	0x55,0x55,0x55, 0x55,0x55,0x55, 0x55,0x55,0x55,
	0x55,0x55,0x55, 0x55,0x55,0x55, 0x55,0x55,0x55,
};

#define NOFONT ((unsigned char *)__nofont)

//#define Fnt_hp 0xc0c000
#define Fnt_n 10
#define Fnt_hb Fnt_n
#define Fnt_zb (Fnt_n + (Fnt_n>>1))
#define Fnt_zp (Fnt_hp + Fnt_hb * 160)
#define Fnt_zc 84

SFONT sFontStatus = { 0,0,0,127,0,87,0 };	//8*10�^�C�vFONT�̕\���ݒ�

////////////////////////////////////////////////////////////////////
//	MMC�J�[�l���֘A�iMMC�J�[�l���ŕ�����������̂�h���j
//	2005/04/09	�d��Chu
unsigned char *Fnt_hp = (unsigned char *)0xc0c000;
void sfont_mmckn_VersionCheck(void)
{
	if(pceSystemGetInfo()->bios_ver > VERSION(1, 20)) {	// 1.28�ȍ~
		Fnt_hp = (unsigned char *)0xc0d000;
	}
}

unsigned short sFontPutFuchi( int x, int y, unsigned short code );
unsigned short sFontPutMoji( int x, int y, unsigned short code );
char *sFontPutStrEx( char Type, const char *p );

/*******************************************************************
* �֐����FsFontGetAdrs 
*  �W���t�H���g�̃A�h���X���擾���܂��B
*-------------------------------------------------------------------
* ����
*  code --- �����R�[�h
*-------------------------------------------------------------------
* �߂�l
*  �t�H���g�̃A�h���X
*   �A�h���X��b31��1�̏ꍇ�͔��p�t�H���g�ł��邱�Ƃ������܂��B
*-------------------------------------------------------------------
* ���l
*  �����R�[�h�͈ȉ��͈̔͂ł��B
*   0x20 - 0x7f : ASCII
*   0xa0 - 0xdf : ���p�J�i
*   0x8140 - 0xfcfc : ����
*******************************************************************/
const unsigned char *sFontGetAdrs( unsigned short code )
{
	unsigned char h = code>>8;
	unsigned char l = code;
	KFONTHDR *hp;
	
	if ( h >= 0x81 && h <= 0x9f ) {
		h-=0x81;
	}
	else if ( h >= 0xe0 && h <= 0xfc ) {
		h-=(0x81+0x40);
	}
	else if ( h == 0 ) {	// ���p
		if ( l >= 0x20 && l <= 0x7f ) {
			return (unsigned char *)(0x80000000 + Fnt_hp + (l-0x20)*Fnt_hb);
		}
		else if ( l >= 0xa0 && l <= 0xdf ) {
			return (unsigned char *)(0x80000000 + Fnt_hp + (l-0x40)*Fnt_hb);
		}
		return NOFONT+0x80000000;
	}
	else {
		return NOFONT;
	}
	
	h<<=1;
	if ( l >= 0x80 ) l--;
	l-=0x40;
	if ( l >= 94 ) { l-=94; h++; }
	
	if ( h >= Fnt_zc || l >= 94 ) return NOFONT;
	
	hp = ((KFONTHDR *)Fnt_zp) + h;
	
	if ( l < hp->s || l > hp->e ) return NOFONT;
	
	return (unsigned char *)(Fnt_zp + sizeof(*hp)*Fnt_zc + (hp->ofs-hp->s+l)*Fnt_zb);
}

/*******************************************************************
* �֐����FsFontPutFuchi
*  �����p�̔�(��)���������z��ʂɕ\�����܂��B
*-------------------------------------------------------------------
* ����
*  x    --- �����W
*  y    --- �c���W
*  code --- �����R�[�h
*-------------------------------------------------------------------
* ���l
*  ���̊֐��͕ʂ̊֐�������Ăяo���܂��B
*   sFontStatus.spr  --- �����F���w�i�F������F������
*                    5:    ��      ��      ��    ����
*                    7:    ��      ��      ��    ����
*******************************************************************/
unsigned short sFontPutFuchi( int x, int y, unsigned short code )
{
	int i,j,xe;
	int xs = (x<1)?-x+1:0;
	int ys = (y<1)?-y+1:0;
	int ye = (y>77)?89-y:12;
	const unsigned char *p = sFontGetAdrs( code );
	unsigned char *q = pceLCDSetBuffer(INVALIDPTR)+((y+ys-1)<<7);
	char wkcolor = (sFontStatus.spr&2)?3:0;
	unsigned short fsv[12],fsvb,ret;
	
	fsv[0] = 0;
	fsv[1] = 0;
	if ( (int)p & 0x80000000 ) {//���p
		xe = (x>123)?129-x:6;
		ret = (10<<8)|4;
		
		//��ʔ͈͓��ɏo�͂��������I��
		if ( xs >= xe || ys >= ye ) return ret;
		
		//�����p�̈�쐬
		for ( j = 0 ; j < 10 ; j += 1 ) {
			fsv[j+2] = SFONT_TBL3[*p++>>3];
			fsv[j] |= fsv[j+2];
			fsv[j+1] |= fsv[j+2];
		}
	}
	else {//�S�p
		unsigned long sv;
		unsigned char *svp = (unsigned char *)(&sv);
		xe = (x>119)?129-x:10;
		ret = (10<<8)|8;
		
		//��ʔ͈͓��ɏo�͂��������I��
		if ( xs >= xe || ys >= ye ) return ret;
		
		//�����p�̈�쐬
		for ( j = 0 ; j < 10 ; j += 2 ) {
			svp[0] = p[2];
			svp[1] = p[1];
			svp[2] = p[0];
			svp[3] = 0;
			
			//�S�p�Q�s��
			sv>>=2;
			fsv[j+3] = SFONT_TBL3[sv & 0x1f]<<4;
			sv>>=5;
			fsv[j+3] |= SFONT_TBL3[sv & 0x1f];
			
			//�S�p�P�s��
			sv>>=7;
			fsv[j+2] = SFONT_TBL3[sv & 0x1f]<<4;
			sv>>=5;
			fsv[j+2] |= SFONT_TBL3[sv];
			
			fsv[j] |= fsv[j+2];
			fsv[j+1] |= fsv[j+2]|fsv[j+3];
			fsv[j+2] |= fsv[j+3];
			
			p+=3;
		}
	}
	
	//�����`��
	for ( j = ys ; j < ye ; j += 1 ) {
		fsvb = fsv[j]>>xs;
		for ( i = xs ; i < xe ; i += 1 ) {
			if ( fsvb&1 ) q[x-1+i] = wkcolor;
			fsvb >>= 1;
		}
		q+=128;
	}
	
	return ret;
}

/*******************************************************************
* �֐����FsFontPutMoji
*  ���p4*10,�S�p8*10�t�H���g�i�����̂݁j�����z��ʂɕ\�����܂��B
*-------------------------------------------------------------------
* ����
*  x    --- �����W
*  y    --- �c���W
*  code --- �����R�[�h
*-------------------------------------------------------------------
* �߂�l
*  b0-7  --- �������̕�
*  b8-15 --- �c�����̕�
*-------------------------------------------------------------------
* ���l
*  ���̊֐��͕ʂ̊֐�������Ăяo���܂��B
*******************************************************************/
unsigned short sFontPutMoji( int x, int y, unsigned short code )
{
	const unsigned char *p = sFontGetAdrs( code );
	unsigned char *q = pceLCDSetBuffer(INVALIDPTR)+(y<<7);
	int ye = y + 10;
	char wkcolor;					//���ߐF
	const unsigned long *SFONT_TBL;	//�t�H���g�ϊ��p�e�[�u���|�C���^
	char wkFlg = sFontStatus.spr&1;	//��ʓ��Ɏ��܂�Ȃ��� �y�� ���ߗL��p�t���O
	
	//�����F�A�w�i�F�`�F�b�N
	if ( sFontStatus.spr&2 ) {
		SFONT_TBL = SFONT_TBL2;		//�����F:�� �w�i�F:��
		wkcolor = 3;				//���ߐF:��
	} else {
		SFONT_TBL = SFONT_TBL1;		//�����F:�� �w�i�F:��
		wkcolor = 0;				//���ߐF:��
	}
	
	//���߂Ȃ��̎�
	if ( !wkFlg ) wkcolor = -1;		//���ߐF:�Ȃ�
	
	if ( (int)p & 0x80000000 ) {//���p
		int xe = x + 4;
		
		//��ʔ͈͂Ɏ��܂��Ă��邩�`�F�b�N
		if ( y < 0 ) {
			q = pceLCDSetBuffer(INVALIDPTR);
			p -= y;
			y = 0;
			wkFlg = 1;
		}
		if ( ye > 88 ) {
			ye = 88;
			wkFlg = 1;
		}
		if ( x < 0 ) {
			x = 0;
			wkFlg = 1;
		}
		if ( xe > 128 ) {
			xe = 128;
			wkFlg = 1;
		}
		
		//��ʔ͈͓��ɏo�͂��������I��
		if ( x >= xe || y >= ye ) goto HEND;
		
		if ( wkFlg ) {//��ʓ��Ɏ��܂�Ȃ��� �y�� ���ߗL��
			int xs = x;
			
			for ( ; y < ye; y+=1 ) {
				unsigned char *b = (unsigned char *)&SFONT_TBL[*p++>>3];
				for ( ; x < xe; x+=1 ) {
					if ( *b != wkcolor ) q[x] = *b;
					b += 1;
				}
				x = xs;
				q += 128;
			}
		}
		else {//��ʓ��Ɏ��܂鎞�i���߂Ȃ��j
			q += x;
			for ( ; y < ye; y+=1 ) {
				memcpy( q, &SFONT_TBL[*p++>>3], 4 );
				q+=128;
			}
		}
HEND:
		return (10<<8)|4;
	}
	else {//�S�p
		int xe = x + 8;
		int xs = 0;
		unsigned long sv,sv1[2],sv2[2];
		unsigned char *svp = (unsigned char *)(&sv);
		unsigned char *svp1 = (unsigned char *)sv1;
		unsigned char *svp2 = (unsigned char *)sv2;
		
		//��ʔ͈͂Ɏ��܂��Ă��邩�`�F�b�N
		if ( y < 0 ) {
			p -= y/2*3;
			y %= 2;
			q = pceLCDSetBuffer(INVALIDPTR)+(y<<7);
			wkFlg = 1;
		}
		if ( ye > 88 ) {
			ye = 88;
			wkFlg = 1;
		}
		if ( x < 0 ) {
			xs = -x;
			x = 0;
			wkFlg = 1;
		}
		if ( xe > 128 ) {
			xe = 128;
			wkFlg = 1;
		}
		
		//��ʔ͈͓��ɏo�͂��������I��
		if ( x >= xe || y >= ye ) goto ZEND;
		
		if ( wkFlg ) {//��ʓ��Ɏ��܂�Ȃ��� �y�� ���ߗL��
			int i,xi,ys = y;
			
			//�S�p�Q�s�ڂ̂ݏo�́i��ʔ͈͊O�̕`�������邽�߁j
			if ( y == -1 ) {
				xi = xs;
				svp[0] = p[2];
				svp[1] = p[1];
				svp[2] = p[0];
				svp[3] = 0;
				
				//�S�p�Q�s��
				sv>>=2;
				sv2[1] = SFONT_TBL[sv & 0x1f];
				sv>>=5;
				sv2[0] = SFONT_TBL[sv & 0x1f];
				
				for ( i = x+128 ; i < xe ; i+=1 ) {
					if ( svp2[xi] != wkcolor ) q[i] = svp2[xi];
					xi += 1;
				}
				
				p+=3;
				q+=256;
				y=1;
			}
			
			//�S�p�P�s�ځA�Q�s�ڂƂ��ɏo��
			for ( ; y+1 < ye; y+=2 ) {
				xi = xs;
				svp[0] = p[2];
				svp[1] = p[1];
				svp[2] = p[0];
				svp[3] = 0;
				
				//�S�p�Q�s��
				sv>>=2;
				sv2[1] = SFONT_TBL[sv & 0x1f];
				sv>>=5;
				sv2[0] = SFONT_TBL[sv & 0x1f];
				
				//�S�p�P�s��
				sv>>=7;
				sv1[1] = SFONT_TBL[sv & 0x1f];
				sv>>=5;
				sv1[0] = SFONT_TBL[sv];
				
				for ( i = x ; i < xe ; i+=1 ) {
					if ( svp1[xi] != wkcolor ) q[i] = svp1[xi];
					if ( svp2[xi] != wkcolor ) q[i+128] = svp2[xi];
					xi += 1;
				}
				
				p+=3;
				q+=256;
			}
			
			//�S�p�P�s�ڂ̂ݏo�́i��ʔ͈͊O�̕`�������邽�߁j
			if ( ys > 78 && y == 87 ) {
				xi = xs;
				svp[0] = p[2];
				svp[1] = p[1];
				svp[2] = p[0];
				svp[3] = 0;
				
				//�S�p�P�s��
				sv>>=14;
				sv1[1] = SFONT_TBL[sv & 0x1f];
				sv>>=5;
				sv1[0] = SFONT_TBL[sv];
				
				for ( i = x ; i < xe ; i+=1 ) {
					if ( svp1[xi] != wkcolor ) q[i] = svp1[xi];
					xi += 1;
				}
				
				p+=3;
				q+=256;
			}
		}
		else {//��ʓ��Ɏ��܂鎞�i���߂Ȃ��j
			q += x;
			for ( ; y < ye; y+=2 ) {
				svp[0] = p[2];
				svp[1] = p[1];
				svp[2] = p[0];
				svp[3] = 0;
				
				//�S�p�Q�s��
				sv>>=2;
				sv2[1] = SFONT_TBL[sv & 0x1f];
				sv>>=5;
				sv2[0] = SFONT_TBL[sv & 0x1f];
				
				//�S�p�P�s��
				sv>>=7;
				sv1[1] = SFONT_TBL[sv & 0x1f];
				sv>>=5;
				sv1[0] = SFONT_TBL[sv];
				
				memcpy( q, svp1, 8 );
				memcpy( q+128, svp2, 8 );
				
				p+=3;
				q+=256;
			}
		}
ZEND:
		return (10<<8)|8;
	}
}

/*******************************************************************
* �֐����FsFontPut
*  ���p4*10,�S�p8*10�t�H���g�����z��ʂɕ\�����܂��B
*-------------------------------------------------------------------
* ����
*  x    --- �����W
*  y    --- �c���W
*  code --- �����R�[�h
*-------------------------------------------------------------------
* �߂�l
*  b0-7  --- �������̕�
*  b8-15 --- �c�����̕�
*-------------------------------------------------------------------
* ���l
*  �g�p�O�� sFontStatus.spr ��ݒ肵�Ă����ĉ������B
*   sFontStatus.spr  --- �����F���w�i�F������F������
*                    0:    ��      ��     �Ȃ�   �Ȃ�
*                    1:    ��      ��     �Ȃ�   ����
*                    2:    ��      ��     �Ȃ�   �Ȃ�
*                    3:    ��      ��     �Ȃ�   ����
*                    5:    ��      ��      ��    ����
*                    7:    ��      ��      ��    ����
*  �g�p�ł��镶���R�[�h�� pceFontGetAdrs() �������������B
*  ���z��ʂ͈̔͊O�̓N���b�s���O����܂��B
*******************************************************************/
unsigned short sFontPut( int x, int y, unsigned short code )
{
	//�����`��
	if ( sFontStatus.spr&4 ) sFontPutFuchi( x, y, code );
	
	//�����`��
	return sFontPutMoji( x, y, code );
}

/*******************************************************************
* �֐����FsFontPutStrEx
*  ���p4*10,�S�p8*10�t�H���g�ŕ���������z��ʂɏ������݂܂��B
*-------------------------------------------------------------------
* ����
*  Type --- �����^�C�v 0:�����`�� 1:�����`��
*  *p   --- ������̃A�h���X�B0 �ŏI�[�B
*-------------------------------------------------------------------
* �߂�l
*  sFontStatus�Ŏw�肳�ꂽ��ʔ͈͊O�܂ŕ\�����i��ł����ꍇ��
*   �\��������Ȃ�����������̃A�h���X
*  ����ȊO��
*   NULL
*  ��Ԃ��܂�
*-------------------------------------------------------------------
* ���l
*  ���̊֐��͕ʂ̊֐�������Ăяo���܂��B
*******************************************************************/
char *sFontPutStrEx( char Type, const char *p )
{
	union {
		unsigned short s;
		struct {
			unsigned char w;
			unsigned char h;
		} b;
	} m;
	unsigned char c1;
	char *rpt = NULL;
	m.s = 0;

	while ( 1 ) {
		c1 = *p++;
		if ( !c1 ) break;
		if ( (c1>=0x81 && c1<=0x9f) || (c1>=0xe0 && c1<=0xfc) ) {
			if ( *p ) {
				unsigned char c2 = *p++;
				if ( Type ) {
					m.s = sFontPutMoji( sFontStatus.x, sFontStatus.y, (c1<<8) + c2 );
				} else {
					m.s = sFontPutFuchi( sFontStatus.x, sFontStatus.y, (c1<<8) + c2 );
				}
			}
			else {
				break;
			}
		}
		else if ( c1 == '\r' ) {
			if ( *p == '\n' ) p+=1;
			goto CRLF;
		}
		else if ( c1 == '\n' ) {
			goto CRLF;
		}
		else {
			if ( Type ) {
				m.s = sFontPutMoji( sFontStatus.x, sFontStatus.y, c1 );
			} else {
				m.s = sFontPutFuchi( sFontStatus.x, sFontStatus.y, c1 );
			}
		}
		sFontStatus.x += m.b.w;
		c1 = *p;
		if ( sFontStatus.x+(((c1>=0x81 && c1<=0x9f)||(c1>=0xe0 && c1<=0xfc))?7:3) > sFontStatus.xMax ) {
CRLF:
			sFontStatus.x = sFontStatus.xMin;
			sFontStatus.y += 10;
			if ( sFontStatus.y+9 > sFontStatus.yMax ) {
				sFontStatus.y = sFontStatus.yMin;
				if ( *p ) rpt = (char *)p;
				break;
			}
		}
	}
	return rpt;
}

/*******************************************************************
* �֐����FsFontPutStr
*  ���p4*10,�S�p8*10�t�H���g�ŕ���������z��ʂɏ������݂܂��B
*-------------------------------------------------------------------
* ����
*  *p --- ������̃A�h���X�B0 �ŏI�[�B
*-------------------------------------------------------------------
* �߂�l
*  sFontStatus�Ŏw�肳�ꂽ��ʔ͈͊O�܂ŕ\�����i��ł����ꍇ��
*   �\��������Ȃ�����������̃A�h���X
*  ����ȊO��
*   NULL
*  ��Ԃ��܂�
*-------------------------------------------------------------------
* ���l
*  �g�p�O�� sFontStatus ��ݒ肵�ĉ������B
*   sFontStatus.x    --- �\���ʒu�����W(0)
*   sFontStatus.y    --- �\���ʒu�c���W(0)
*   sFontStatus.xMin --- �\���͈͉����ŏ��l(0)
*   sFontStatus.xMax --- �\���͈͉����ő�l(127)
*   sFontStatus.yMin --- �\���͈͏c���ŏ��l(0)
*   sFontStatus.yMax --- �\���͈͏c���ő�l(87)
*   sFontStatus.spr  --- �����F���w�i�F������F������(0)
*                    0:    ��      ��     �Ȃ�   �Ȃ�
*                    1:    ��      ��     �Ȃ�   ����
*                    2:    ��      ��     �Ȃ�   �Ȃ�
*                    3:    ��      ��     �Ȃ�   ����
*                    5:    ��      ��      ��    ����
*                    7:    ��      ��      ��    ����
*    ()���̐��l�̓f�t�H���g�l�ł�
*    sFontStatus.x �� sFontStatus.y�͂��̊֐������s�����ƁA
*    ���̕�����\�����ׂ����Ɏ����I�ɍX�V����܂��B
*******************************************************************/
char *sFontPutStr( const char *p )
{
	char *ret;
	SFONT bkFont = sFontStatus;
	
	if ( sFontStatus.spr&4 ) {
		bkFont.x += ( sFontStatus.x==sFontStatus.xMin );
		bkFont.y += ( sFontStatus.y==sFontStatus.yMin );
		sFontStatus.xMin += 1;
		sFontStatus.xMax -= 1;
		sFontStatus.yMin += 1;
		sFontStatus.yMax -= 1;
		
		sFontStatus.x = bkFont.x;
		sFontStatus.y = bkFont.y;
		sFontPutStrEx( 0, p );
		sFontStatus.x = bkFont.x;
		sFontStatus.y = bkFont.y;
	}
	
	ret = sFontPutStrEx( 1, p );
	bkFont.x = sFontStatus.x;
	bkFont.y = sFontStatus.y;
	sFontStatus = bkFont;
	
	return ret;
}

// �ȉ� nsawa����́w�ԊO��XMODEM�x ��蔲��

/****************************************************************************
 *	sFontPrintf
 ****************************************************************************/

int
sFontPrintf(const char* fmt, ...)
{
extern unsigned char _def_vbuff[]; /* ������W�J�ɗ��p */
	int result;
	va_list ap;

	va_start(ap, fmt);
	result = vsprintf(_def_vbuff, fmt, ap);
	va_end(ap);

	sFontPutStr(_def_vbuff);

	return result;
}
