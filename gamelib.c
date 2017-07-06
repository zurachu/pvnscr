/*******************************************************************************
//  汎用ゲームライブラリ
//  (c)2003 てとら★ぽっと
*******************************************************************************/
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <piece.h>
#include <smcvals.h>

unsigned char *vbuff;
BOOL LCDUpdate;

//=============================================================================
//  起動時処理
//=============================================================================
void App_Init()
{
	pceLCDDispStop();		// 液晶表示を無効に
	vbuff = pceHeapAlloc( 128*88 );
	memset( vbuff, 0, 128*88 );
	pceLCDSetBuffer( vbuff );

	pceLCDDispStart();		//液晶表示を有効に
	LCDUpdate = TRUE;
}


//=============================================================================
//  終了時処理
//=============================================================================
void App_Exit()
{
	pceWaveStop( 0 );		// 音の停止
}


//=============================================================================
//  P/ECE用ビットマップ生成
//=============================================================================
void Get_PieceBmp( PIECE_BMP* pBmp, unsigned char* data )
{
	memcpy( &(pBmp->header), data, sizeof(PBMP_FILEHEADER) );
	pBmp->buf = data + sizeof(PBMP_FILEHEADER);
	pBmp->mask = pBmp->buf + ( pBmp->header.w * pBmp->header.h ) / 4;
}


//=============================================================================
//  P/ECE用ビットマップ描画
//=============================================================================
void Draw_Object( PIECE_BMP *pBmp, int dx, int dy, int sx, int sy, int w, int h, int param )
{
	DRAW_OBJECT dObj;

	pceLCDSetObject( &dObj, pBmp, dx, dy, sx, sy, w, h, param );
	pceLCDDrawObject( dObj );
}

//=============================================================================
//  P/ECE用PCM音源生成
//=============================================================================
void Get_PieceWave( PCEWAVEINFO* pWav, unsigned char* data )
{
	*pWav = *((PCEWAVEINFO*)(data+8));
	pWav->pData = data + 8 + sizeof(PCEWAVEINFO);
}


//=============================================================================
//  縁取り文字描画
//=============================================================================
int wFontPrintf( short c, short x, short y, const char* fmt, ... )
{
	extern unsigned char _def_vbuff[]; /* 文字列展開に利用 */
	int ret;
	va_list ap;
	short i, j;

	va_start( ap, fmt );
	ret = vsprintf( _def_vbuff, fmt, ap );
	va_end( ap );

	pceFontSetBkColor( FC_SPRITE );
	if( c == 0 ) { pceFontSetTxColor( 3 ); }
	else		 { pceFontSetTxColor( 0 ); }
	for( i = y - 1; i <= y + 1; i++ ) {
		for( j = x - 1; j <= x + 1; j++ ) {
			if( j != x || i != y ) {
				pceFontSetPos( j, i );
				pceFontPutStr( _def_vbuff );
			}
		}
	}
	if( c == 0 ) { pceFontSetTxColor( 0 ); }
	else		 { pceFontSetTxColor( 3 ); }
	pceFontSetPos( x, y );
	pceFontPutStr( _def_vbuff );

	return( ret );
}


