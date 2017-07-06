/******************************************************************************/
/* パックファイル展開・PVNSPACK圧縮展開・ppack圧縮展開                        */
/*                                       (c)2003-2004 ヅラChu＠てとら★ぽっと */
/* 2004/08/19 ヘッダを1セクタ分しか読んでいなかったのを修正                   */
/*            MMCカーネル（まどかさん）仮対応                                 */
/******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <piece.h>
#include <ctype.h>

#define PPACK_HEADER 0x1c0258

// MMCからファイルを開く場合コメント解除
// #define USE_MMC
// メインプログラムの方でmmcInit(),mmcExit()を呼び出して下さい

//☆ちゃんと別名で定義
#ifdef USE_MMC
	#include "mmc_api.h"
	#define _FileOpen		mmcFileOpen
	#define _FileReadSct	mmcFileReadSct
	#define _FileClose		mmcFileClose	
#else
	#define _FileOpen		pceFileOpen
	#define _FileReadSct	pceFileReadSct
	#define _FileClose		pceFileClose	
#endif


/******************************************************************************/
/* ここからfilepack.c(\usr\PIECE\tools\filepack\デコーダsrc)                  */
/******************************************************************************/
#define DWORD_CHAR(ch0, ch1, ch2, ch3)                                     \
				(((DWORD)(BYTE)(ch0) << 24) | ((DWORD)(BYTE)(ch1) << 16) | \
				 ((DWORD)(BYTE)(ch2) <<  8) | ((DWORD)(BYTE)(ch3)      ))

typedef struct {
	unsigned long head;
	long          famount;
}FILE_PAC_INFO;

typedef struct {
	char          fname[16];
	unsigned long offset;
	unsigned long size;
}FILE_INFO;

/* 半角大文字を、小文字に変換 */
static void strlower( char *str )
{
	while( *str ) {
		*str = (char)tolower( *str );
		str++;
	}
}

/* 文字列比較 */
static int strcmpUL( const char *str1, const char *str2 )
{
	char buf1[16], buf2[16];
	strcpy( buf1, str1 );
	strcpy( buf2, str2 );
	strlower( buf1 );
	strlower( buf2 );
	return strcmp( buf1, buf2 );
}
/******************************************************************************/
/* ここまでfilepack.c(\usr\PIECE\tools\filepack\デコーダsrc)                  */
/******************************************************************************/

/******************************************************************************/
/* ここからzlib展開ルーチンサンプル(from P/ECE HAND BOOK Vol.2)               */
/******************************************************************************/
#include "piecezl.h"
// 0x13e000～0x13ffff まで、展開用バッファ
#define	EXTSIZE		0x0400
#define	WORKSADR	0x13d400
#define	WORKS 		((void *)WORKSADR)
#define	EXTBUFF		((unsigned char *)(WORKSADR-EXTSIZE))
static char *pzLibDstBuf;
static long declen = 0;

static int dummy( zlibIO *zi ) {
	return 0xff;
}

static void DecodeProc1( zlibIO *zi ) {
	int n = zi->ptr - zi->ptr0;
	zi->ptr = zi->ptr0;
	memcpy( &pzLibDstBuf[declen], zi->ptr0, n );
	declen += n;
}

// pArcData: 圧縮データ
// pOutBuff: 展開したデータが入るバッファ
int unpack( char *pArcData, char *pOutBuff ) {
	zlibIO Zin, Zout;
	declen = 0;
	pzLibDstBuf = pOutBuff;
	Zin.ptr = (unsigned char *)(pArcData+36);
	Zin.ptre = (unsigned char *)-1;
	Zin.fn.fil = dummy;
	Zout.ptr0 = EXTBUFF;
	Zout.ptr = EXTBUFF;
	Zout.ptre = EXTBUFF + EXTSIZE;
	Zout.fn.fls = DecodeProc1;
	pceZlibExpand( &Zin, &Zout, WORKS );
	return declen;
}

/******************************************************************************/
/* ここまでzlib展開ルーチンサンプル(from P/ECE HAND BOOK Vol.2)               */
/******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
// 圧縮ファイルかどうか確認
BOOL ppack_checkHeader( unsigned char* arcData )
{
	if(*(long*)(arcData) == PPACK_HEADER) { return TRUE; }
	return FALSE;
}


////////////////////////////////////////////////////////////////////////////////
// 圧縮元ファイルの長さ取得
long ppack_getExpandSize( unsigned char* arcData )
{
	return(*(long*)(arcData+28));
}


////////////////////////////////////////////////////////////////////////////////
// 展開用ヒープ領域を確保してから展開
unsigned char* ppack_heapUnpack( unsigned char* arcData )
{
	unsigned char* ret = NULL;
	long size;
	
	if(!ppack_checkHeader(arcData)) { return NULL; }
	size = ppack_getExpandSize(arcData);
	if((ret = pceHeapAlloc(size)) == NULL) { return NULL; }
	unpack(arcData, ret);
	return(ret);
}


////////////////////////////////////////////////////////////////////////////////
// フラッシュメモリにあるfpkファイル、pvaファイルからデータをヒープに取得
unsigned char* ppack_findPackDataEx(const char *fpkName, const char *fName)
{
	unsigned char *ret, *tmp;
	FILE_PAC_INFO fpi;	// パックファイルヘッダ構造体
	FILE_INFO     fi;	// ファイル情報構造体
	FILEACC       pfa;	// ファイル読み込み用構造体
	short i = 0, j, ptr = sizeof(FILE_PAC_INFO), sct = 0, ext = 0;
	
	// ファイルの先頭1セクタのポインタを取得
	if(_FileOpen(&pfa, fpkName, FOMD_RD))
		return NULL;
	
	_FileReadSct(&pfa, NULL, 0, 4096);
	tmp = (unsigned char *)pfa.aptr;
	_FileClose(&pfa);
	
	// パックファイルヘッダ確認
	memcpy(&fpi, tmp, sizeof(FILE_PAC_INFO));
	if(fpi.head == DWORD_CHAR('F','P','A','K')) { ext = 1; }	// fpk
	if(fpi.head == DWORD_CHAR('A','N','V','P')) { ext = 2; }	// pva
	if(ext == 0)
		return NULL; 	// それ以外（失敗）
	
	// 必要なファイルの配置を取得
	while(i < fpi.famount) {
		if(ptr + sizeof(FILE_INFO) < 4096) {
			memcpy(&fi, tmp + ptr, sizeof(FILE_INFO));	// ファイル情報構造体を取得
			ptr += sizeof(FILE_INFO);
		} else {	// ファイル情報が次のセクタにまたがる場合
			j = 4096 - ptr;
			memcpy(&fi, tmp + ptr, j);	// ファイル情報構造体（前半）
			sct++;
			_FileOpen(&pfa, fpkName, FOMD_RD);	// ポインタを次のセクタに差替え
			_FileReadSct(&pfa, NULL, sct, 4096);
			tmp = (unsigned char *)pfa.aptr;
			_FileClose(&pfa);
			ptr = sizeof(FILE_INFO) - j;
			memcpy((unsigned char *)&fi + j, tmp, ptr);	// ファイル情報構造体（後半）
		}
		if(!strcmpUL(fi.fname, fName)) { break; }	// ファイル名が合致していれば発見
		i++;
	}
	if(i >= fpi.famount)
		return NULL;	// 発見失敗
	
	if(ext == 1 && strstr(fName, ".pvn") != NULL) { fi.size++; }	// pvnファイル処理
	if((ret = pceHeapAlloc(fi.size)) == NULL)
		return NULL; 	// メモリ確保
	
	// ファイルから読み込む
	_FileOpen(&pfa, fpkName, FOMD_RD);
	sct = fi.offset / 4096;
	_FileReadSct(&pfa, NULL, sct, 0);	// 先頭
	if(sct == (fi.offset + fi.size) / 4096) {	// 1セクタ内に収まっている場合
		memcpy(ret, pfa.aptr + fi.offset % 4096, fi.size);
	} else {
		memcpy(ret, pfa.aptr + fi.offset % 4096, 4096 - (fi.offset % 4096));
		for(i = sct + 1; i < (fi.offset + fi.size) / 4096; i++) {	// 中間
			_FileReadSct(&pfa, ret - (fi.offset % 4096) + (i - sct) * 4096, i, 4096);
		}
		_FileReadSct(&pfa, ret - (fi.offset % 4096) + (i - sct) * 4096, i, (fi.offset + fi.size) % 4096);	// 終端
	}
	_FileClose(&pfa);
		
	if(ext == 1) {	// fpkならここで抜ける
		if(strstr(fName, ".pvn") != NULL) { *(ret + fi.size - 1) = '\0'; }	// pvnファイル処理
		return(ret);
	}
	
	// 圧縮データを展開
	tmp = ret;
	if(!ppack_checkHeader(tmp)) {
		pceHeapFree(tmp);
		return NULL;
	}
	
	fi.size = ppack_getExpandSize(tmp);
	if(strstr(fName, ".pvn") != NULL) { fi.size++; }
	if((ret = pceHeapAlloc(fi.size)) == NULL) {
		pceHeapFree(tmp);
		return NULL;
	}
	
	unpack(tmp, ret);
	pceHeapFree(tmp);
	if(strstr(fName, ".pvn") != NULL) { *(ret + fi.size - 1) = '\0'; }	// pvnファイル処理
	
	return(ret);
}

