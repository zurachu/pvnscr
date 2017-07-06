
#include "zlibDecode.h"

#include "libfpk.h"
#include "piecezl.h"

/* zlib 展開ルーチンサンプル（from P/ECE HAND BOOK Vol.2 緋色の霧）*/
/* このソースの元は，まかべひろし さんの上記コード */
/* このソース自体は，ヅラChu＠てとら★ぽっと さんの myunpack のソースから三次利用（ぉ */
/* だって P/ECE HAND BOOK 持ってないし（ぉ */

// 0x13e000～0x13ffff まで、展開用バッファ
#define	EXTSIZE		0x0400
#define	WORKSADR	0x13d400
#define	WORKS 		((void *)WORKSADR)
#define	EXTBUFF		((BYTE *)(WORKSADR - EXTSIZE))
static BYTE *pZlibDstBuffer;
static DWORD dwDecodeLength = 0;

static int dummy(zlibIO *zi)
{
	return 0xff;
}

static void DecodeProc1(zlibIO *zi)
{
	int n = zi->ptr - zi->ptr0;

	zi->ptr = zi->ptr0;
	memcpy(&pZlibDstBuffer[dwDecodeLength], zi->ptr0, n);
	dwDecodeLength += n;
}

// pArcData: 圧縮データ
// pOutBuff: 展開したデータが入るバッファ
DWORD zlibDecode(BYTE *pArcData, BYTE *pOutBuff)
{
	zlibIO Zin, Zout;

	dwDecodeLength = 0;
	pZlibDstBuffer = pOutBuff;

	Zin.ptr = (BYTE *)pArcData;
	Zin.ptre = (BYTE *)-1;
	Zin.fn.fil = dummy;

	Zout.ptr0 = EXTBUFF;
	Zout.ptr = EXTBUFF;
	Zout.ptre = EXTBUFF + EXTSIZE;
	Zout.fn.fls = DecodeProc1;

	pceZlibExpand(&Zin, &Zout, WORKS);

	return dwDecodeLength;
}
