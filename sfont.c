#include <stdarg.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <piece.h>

#include <smcvals.h>

#include "sfont.h"

const unsigned long SFONT_TBL1[32] = {		//フォント変換用テーブル(5bit→4byte)白地に黒
	0x00000000, 0x02000000, 0x00020000, 0x03020000,
	0x00020100, 0x02020100, 0x00030100, 0x03030100,
	0x00000200, 0x02000200, 0x00020200, 0x03020200,
	0x00010300, 0x02010300, 0x00030300, 0x03030300,
	0x00000002, 0x02000002, 0x00020002, 0x03020002,
	0x00020102, 0x02020102, 0x00030102, 0x03030102,
	0x00000203, 0x02000203, 0x00020203, 0x03020203,
	0x00010303, 0x02010303, 0x00030303, 0x03030303
};

const unsigned long SFONT_TBL2[32] = {		//フォント変換用テーブル(5bit→4byte)黒地に白
	0x03030303, 0x01030303, 0x03010303, 0x00010303,
	0x03010203, 0x01010203, 0x03000203, 0x00000203,
	0x03030103, 0x01030103, 0x03010103, 0x00010103,
	0x03020003, 0x01020003, 0x03000003, 0x00000003,
	0x03030301, 0x01030301, 0x03010301, 0x00010301,
	0x03010201, 0x01010201, 0x03000201, 0x00000201,
	0x03030100, 0x01030100, 0x03010100, 0x00010100,
	0x03020000, 0x01020000, 0x03000000, 0x00000000
};

const unsigned short SFONT_TBL3[32] = {		//フォント縁取り用テーブル
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

SFONT sFontStatus = { 0,0,0,127,0,87,0 };	//8*10タイプFONTの表示設定

////////////////////////////////////////////////////////////////////
//	MMCカーネル関連（MMCカーネルで文字化けするのを防ぐ）
//	2005/04/09	ヅラChu
unsigned char *Fnt_hp = (unsigned char *)0xc0c000;
void sfont_mmckn_VersionCheck(void)
{
	if(pceSystemGetInfo()->bios_ver > VERSION(1, 20)) {	// 1.28以降
		Fnt_hp = (unsigned char *)0xc0d000;
	}
}

unsigned short sFontPutFuchi( int x, int y, unsigned short code );
unsigned short sFontPutMoji( int x, int y, unsigned short code );
char *sFontPutStrEx( char Type, const char *p );

/*******************************************************************
* 関数名：sFontGetAdrs 
*  標準フォントのアドレスを取得します。
*-------------------------------------------------------------------
* 引数
*  code --- 文字コード
*-------------------------------------------------------------------
* 戻り値
*  フォントのアドレス
*   アドレスのb31が1の場合は半角フォントであることを示します。
*-------------------------------------------------------------------
* 備考
*  文字コードは以下の範囲です。
*   0x20 - 0x7f : ASCII
*   0xa0 - 0xdf : 半角カナ
*   0x8140 - 0xfcfc : 漢字
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
	else if ( h == 0 ) {	// 半角
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
* 関数名：sFontPutFuchi
*  縁取り用の白(黒)抜きを仮想画面に表示します。
*-------------------------------------------------------------------
* 引数
*  x    --- 横座標
*  y    --- 縦座標
*  code --- 文字コード
*-------------------------------------------------------------------
* 備考
*  この関数は別の関数内から呼び出します。
*   sFontStatus.spr  --- 文字色＆背景色＆縁取色＆透過
*                    5:    黒      白      白    あり
*                    7:    白      黒      黒    あり
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
	if ( (int)p & 0x80000000 ) {//半角
		xe = (x>123)?129-x:6;
		ret = (10<<8)|4;
		
		//画面範囲内に出力が無い時終了
		if ( xs >= xe || ys >= ye ) return ret;
		
		//縁取り用領域作成
		for ( j = 0 ; j < 10 ; j += 1 ) {
			fsv[j+2] = SFONT_TBL3[*p++>>3];
			fsv[j] |= fsv[j+2];
			fsv[j+1] |= fsv[j+2];
		}
	}
	else {//全角
		unsigned long sv;
		unsigned char *svp = (unsigned char *)(&sv);
		xe = (x>119)?129-x:10;
		ret = (10<<8)|8;
		
		//画面範囲内に出力が無い時終了
		if ( xs >= xe || ys >= ye ) return ret;
		
		//縁取り用領域作成
		for ( j = 0 ; j < 10 ; j += 2 ) {
			svp[0] = p[2];
			svp[1] = p[1];
			svp[2] = p[0];
			svp[3] = 0;
			
			//全角２行目
			sv>>=2;
			fsv[j+3] = SFONT_TBL3[sv & 0x1f]<<4;
			sv>>=5;
			fsv[j+3] |= SFONT_TBL3[sv & 0x1f];
			
			//全角１行目
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
	
	//縁取り描画
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
* 関数名：sFontPutMoji
*  半角4*10,全角8*10フォント（文字のみ）を仮想画面に表示します。
*-------------------------------------------------------------------
* 引数
*  x    --- 横座標
*  y    --- 縦座標
*  code --- 文字コード
*-------------------------------------------------------------------
* 戻り値
*  b0-7  --- 横方向の幅
*  b8-15 --- 縦方向の幅
*-------------------------------------------------------------------
* 備考
*  この関数は別の関数内から呼び出します。
*******************************************************************/
unsigned short sFontPutMoji( int x, int y, unsigned short code )
{
	const unsigned char *p = sFontGetAdrs( code );
	unsigned char *q = pceLCDSetBuffer(INVALIDPTR)+(y<<7);
	int ye = y + 10;
	char wkcolor;					//透過色
	const unsigned long *SFONT_TBL;	//フォント変換用テーブルポインタ
	char wkFlg = sFontStatus.spr&1;	//画面内に収まらない時 及び 透過有り用フラグ
	
	//文字色、背景色チェック
	if ( sFontStatus.spr&2 ) {
		SFONT_TBL = SFONT_TBL2;		//文字色:白 背景色:黒
		wkcolor = 3;				//透過色:黒
	} else {
		SFONT_TBL = SFONT_TBL1;		//文字色:黒 背景色:白
		wkcolor = 0;				//透過色:白
	}
	
	//透過なしの時
	if ( !wkFlg ) wkcolor = -1;		//透過色:なし
	
	if ( (int)p & 0x80000000 ) {//半角
		int xe = x + 4;
		
		//画面範囲に収まっているかチェック
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
		
		//画面範囲内に出力が無い時終了
		if ( x >= xe || y >= ye ) goto HEND;
		
		if ( wkFlg ) {//画面内に収まらない時 及び 透過有り
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
		else {//画面内に収まる時（透過なし）
			q += x;
			for ( ; y < ye; y+=1 ) {
				memcpy( q, &SFONT_TBL[*p++>>3], 4 );
				q+=128;
			}
		}
HEND:
		return (10<<8)|4;
	}
	else {//全角
		int xe = x + 8;
		int xs = 0;
		unsigned long sv,sv1[2],sv2[2];
		unsigned char *svp = (unsigned char *)(&sv);
		unsigned char *svp1 = (unsigned char *)sv1;
		unsigned char *svp2 = (unsigned char *)sv2;
		
		//画面範囲に収まっているかチェック
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
		
		//画面範囲内に出力が無い時終了
		if ( x >= xe || y >= ye ) goto ZEND;
		
		if ( wkFlg ) {//画面内に収まらない時 及び 透過有り
			int i,xi,ys = y;
			
			//全角２行目のみ出力（画面範囲外の描画を避けるため）
			if ( y == -1 ) {
				xi = xs;
				svp[0] = p[2];
				svp[1] = p[1];
				svp[2] = p[0];
				svp[3] = 0;
				
				//全角２行目
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
			
			//全角１行目、２行目ともに出力
			for ( ; y+1 < ye; y+=2 ) {
				xi = xs;
				svp[0] = p[2];
				svp[1] = p[1];
				svp[2] = p[0];
				svp[3] = 0;
				
				//全角２行目
				sv>>=2;
				sv2[1] = SFONT_TBL[sv & 0x1f];
				sv>>=5;
				sv2[0] = SFONT_TBL[sv & 0x1f];
				
				//全角１行目
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
			
			//全角１行目のみ出力（画面範囲外の描画を避けるため）
			if ( ys > 78 && y == 87 ) {
				xi = xs;
				svp[0] = p[2];
				svp[1] = p[1];
				svp[2] = p[0];
				svp[3] = 0;
				
				//全角１行目
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
		else {//画面内に収まる時（透過なし）
			q += x;
			for ( ; y < ye; y+=2 ) {
				svp[0] = p[2];
				svp[1] = p[1];
				svp[2] = p[0];
				svp[3] = 0;
				
				//全角２行目
				sv>>=2;
				sv2[1] = SFONT_TBL[sv & 0x1f];
				sv>>=5;
				sv2[0] = SFONT_TBL[sv & 0x1f];
				
				//全角１行目
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
* 関数名：sFontPut
*  半角4*10,全角8*10フォントを仮想画面に表示します。
*-------------------------------------------------------------------
* 引数
*  x    --- 横座標
*  y    --- 縦座標
*  code --- 文字コード
*-------------------------------------------------------------------
* 戻り値
*  b0-7  --- 横方向の幅
*  b8-15 --- 縦方向の幅
*-------------------------------------------------------------------
* 備考
*  使用前に sFontStatus.spr を設定しておいて下さい。
*   sFontStatus.spr  --- 文字色＆背景色＆縁取色＆透過
*                    0:    黒      白     なし   なし
*                    1:    黒      白     なし   あり
*                    2:    白      黒     なし   なし
*                    3:    白      黒     なし   あり
*                    5:    黒      白      白    あり
*                    7:    白      黒      黒    あり
*  使用できる文字コードは pceFontGetAdrs() をご覧下さい。
*  仮想画面の範囲外はクリッピングされます。
*******************************************************************/
unsigned short sFontPut( int x, int y, unsigned short code )
{
	//縁取り描画
	if ( sFontStatus.spr&4 ) sFontPutFuchi( x, y, code );
	
	//文字描画
	return sFontPutMoji( x, y, code );
}

/*******************************************************************
* 関数名：sFontPutStrEx
*  半角4*10,全角8*10フォントで文字列を仮想画面に書き込みます。
*-------------------------------------------------------------------
* 引数
*  Type --- 縁取りタイプ 0:縁取り描画 1:文字描画
*  *p   --- 文字列のアドレス。0 で終端。
*-------------------------------------------------------------------
* 戻り値
*  sFontStatusで指定された画面範囲外まで表示が進んでいた場合は
*   表示しきれなかった文字列のアドレス
*  それ以外は
*   NULL
*  を返します
*-------------------------------------------------------------------
* 備考
*  この関数は別の関数内から呼び出します。
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
* 関数名：sFontPutStr
*  半角4*10,全角8*10フォントで文字列を仮想画面に書き込みます。
*-------------------------------------------------------------------
* 引数
*  *p --- 文字列のアドレス。0 で終端。
*-------------------------------------------------------------------
* 戻り値
*  sFontStatusで指定された画面範囲外まで表示が進んでいた場合は
*   表示しきれなかった文字列のアドレス
*  それ以外は
*   NULL
*  を返します
*-------------------------------------------------------------------
* 備考
*  使用前に sFontStatus を設定して下さい。
*   sFontStatus.x    --- 表示位置横座標(0)
*   sFontStatus.y    --- 表示位置縦座標(0)
*   sFontStatus.xMin --- 表示範囲横軸最小値(0)
*   sFontStatus.xMax --- 表示範囲横軸最大値(127)
*   sFontStatus.yMin --- 表示範囲縦軸最小値(0)
*   sFontStatus.yMax --- 表示範囲縦軸最大値(87)
*   sFontStatus.spr  --- 文字色＆背景色＆縁取色＆透過(0)
*                    0:    黒      白     なし   なし
*                    1:    黒      白     なし   あり
*                    2:    白      黒     なし   なし
*                    3:    白      黒     なし   あり
*                    5:    黒      白      白    あり
*                    7:    白      黒      黒    あり
*    ()内の数値はデフォルト値です
*    sFontStatus.x と sFontStatus.yはこの関数が実行されると、
*    次の文字を表示すべき所に自動的に更新されます。
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

// 以下 nsawaさんの『赤外線XMODEM』 より抜粋

/****************************************************************************
 *	sFontPrintf
 ****************************************************************************/

int
sFontPrintf(const char* fmt, ...)
{
extern unsigned char _def_vbuff[]; /* 文字列展開に利用 */
	int result;
	va_list ap;

	va_start(ap, fmt);
	result = vsprintf(_def_vbuff, fmt, ap);
	va_end(ap);

	sFontPutStr(_def_vbuff);

	return result;
}
