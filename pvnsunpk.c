/******************************************************************************/
/* �p�b�N�t�@�C���W�J�EPVNSPACK���k�W�J�Eppack���k�W�J                        */
/*                                       (c)2003-2004 �d��Chu���ĂƂ灚�ۂ��� */
/* 2004/08/19 �w�b�_��1�Z�N�^�������ǂ�ł��Ȃ������̂��C��                   */
/*            MMC�J�[�l���i�܂ǂ�����j���Ή�                                 */
/******************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <piece.h>
#include <ctype.h>

#define PPACK_HEADER 0x1c0258

// MMC����t�@�C�����J���ꍇ�R�����g����
// #define USE_MMC
// ���C���v���O�����̕���mmcInit(),mmcExit()���Ăяo���ĉ�����

//�������ƕʖ��Œ�`
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
/* ��������filepack.c(\usr\PIECE\tools\filepack\�f�R�[�_src)                  */
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

/* ���p�啶�����A�������ɕϊ� */
static void strlower( char *str )
{
	while( *str ) {
		*str = (char)tolower( *str );
		str++;
	}
}

/* �������r */
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
/* �����܂�filepack.c(\usr\PIECE\tools\filepack\�f�R�[�_src)                  */
/******************************************************************************/

/******************************************************************************/
/* ��������zlib�W�J���[�`���T���v��(from P/ECE HAND BOOK Vol.2)               */
/******************************************************************************/
#include "piecezl.h"
// 0x13e000�`0x13ffff �܂ŁA�W�J�p�o�b�t�@
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

// pArcData: ���k�f�[�^
// pOutBuff: �W�J�����f�[�^������o�b�t�@
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
/* �����܂�zlib�W�J���[�`���T���v��(from P/ECE HAND BOOK Vol.2)               */
/******************************************************************************/

////////////////////////////////////////////////////////////////////////////////
// ���k�t�@�C�����ǂ����m�F
BOOL ppack_checkHeader( unsigned char* arcData )
{
	if(*(long*)(arcData) == PPACK_HEADER) { return TRUE; }
	return FALSE;
}


////////////////////////////////////////////////////////////////////////////////
// ���k���t�@�C���̒����擾
long ppack_getExpandSize( unsigned char* arcData )
{
	return(*(long*)(arcData+28));
}


////////////////////////////////////////////////////////////////////////////////
// �W�J�p�q�[�v�̈���m�ۂ��Ă���W�J
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
// �t���b�V���������ɂ���fpk�t�@�C���Apva�t�@�C������f�[�^���q�[�v�Ɏ擾
unsigned char* ppack_findPackDataEx(const char *fpkName, const char *fName)
{
	unsigned char *ret, *tmp;
	FILE_PAC_INFO fpi;	// �p�b�N�t�@�C���w�b�_�\����
	FILE_INFO     fi;	// �t�@�C�����\����
	FILEACC       pfa;	// �t�@�C���ǂݍ��ݗp�\����
	short i = 0, j, ptr = sizeof(FILE_PAC_INFO), sct = 0, ext = 0;
	
	// �t�@�C���̐擪1�Z�N�^�̃|�C���^���擾
	if(_FileOpen(&pfa, fpkName, FOMD_RD))
		return NULL;
	
	_FileReadSct(&pfa, NULL, 0, 4096);
	tmp = (unsigned char *)pfa.aptr;
	_FileClose(&pfa);
	
	// �p�b�N�t�@�C���w�b�_�m�F
	memcpy(&fpi, tmp, sizeof(FILE_PAC_INFO));
	if(fpi.head == DWORD_CHAR('F','P','A','K')) { ext = 1; }	// fpk
	if(fpi.head == DWORD_CHAR('A','N','V','P')) { ext = 2; }	// pva
	if(ext == 0)
		return NULL; 	// ����ȊO�i���s�j
	
	// �K�v�ȃt�@�C���̔z�u���擾
	while(i < fpi.famount) {
		if(ptr + sizeof(FILE_INFO) < 4096) {
			memcpy(&fi, tmp + ptr, sizeof(FILE_INFO));	// �t�@�C�����\���̂��擾
			ptr += sizeof(FILE_INFO);
		} else {	// �t�@�C����񂪎��̃Z�N�^�ɂ܂�����ꍇ
			j = 4096 - ptr;
			memcpy(&fi, tmp + ptr, j);	// �t�@�C�����\���́i�O���j
			sct++;
			_FileOpen(&pfa, fpkName, FOMD_RD);	// �|�C���^�����̃Z�N�^�ɍ��ւ�
			_FileReadSct(&pfa, NULL, sct, 4096);
			tmp = (unsigned char *)pfa.aptr;
			_FileClose(&pfa);
			ptr = sizeof(FILE_INFO) - j;
			memcpy((unsigned char *)&fi + j, tmp, ptr);	// �t�@�C�����\���́i�㔼�j
		}
		if(!strcmpUL(fi.fname, fName)) { break; }	// �t�@�C���������v���Ă���Δ���
		i++;
	}
	if(i >= fpi.famount)
		return NULL;	// �������s
	
	if(ext == 1 && strstr(fName, ".pvn") != NULL) { fi.size++; }	// pvn�t�@�C������
	if((ret = pceHeapAlloc(fi.size)) == NULL)
		return NULL; 	// �������m��
	
	// �t�@�C������ǂݍ���
	_FileOpen(&pfa, fpkName, FOMD_RD);
	sct = fi.offset / 4096;
	_FileReadSct(&pfa, NULL, sct, 0);	// �擪
	if(sct == (fi.offset + fi.size) / 4096) {	// 1�Z�N�^���Ɏ��܂��Ă���ꍇ
		memcpy(ret, pfa.aptr + fi.offset % 4096, fi.size);
	} else {
		memcpy(ret, pfa.aptr + fi.offset % 4096, 4096 - (fi.offset % 4096));
		for(i = sct + 1; i < (fi.offset + fi.size) / 4096; i++) {	// ����
			_FileReadSct(&pfa, ret - (fi.offset % 4096) + (i - sct) * 4096, i, 4096);
		}
		_FileReadSct(&pfa, ret - (fi.offset % 4096) + (i - sct) * 4096, i, (fi.offset + fi.size) % 4096);	// �I�[
	}
	_FileClose(&pfa);
		
	if(ext == 1) {	// fpk�Ȃ炱���Ŕ�����
		if(strstr(fName, ".pvn") != NULL) { *(ret + fi.size - 1) = '\0'; }	// pvn�t�@�C������
		return(ret);
	}
	
	// ���k�f�[�^��W�J
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
	if(strstr(fName, ".pvn") != NULL) { *(ret + fi.size - 1) = '\0'; }	// pvn�t�@�C������
	
	return(ret);
}

