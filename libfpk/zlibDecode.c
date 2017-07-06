
#include "zlibDecode.h"

#include "libfpk.h"
#include "piecezl.h"

/* zlib �W�J���[�`���T���v���ifrom P/ECE HAND BOOK Vol.2 ��F�̖��j*/
/* ���̃\�[�X�̌��́C�܂��ׂЂ낵 ����̏�L�R�[�h */
/* ���̃\�[�X���̂́C�d��Chu���ĂƂ灚�ۂ��� ����� myunpack �̃\�[�X����O�����p�i�� */
/* ������ P/ECE HAND BOOK �����ĂȂ����i�� */

// 0x13e000�`0x13ffff �܂ŁA�W�J�p�o�b�t�@
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

// pArcData: ���k�f�[�^
// pOutBuff: �W�J�����f�[�^������o�b�t�@
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
