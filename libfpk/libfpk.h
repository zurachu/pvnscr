/*
	* libfpk -- using FPK archive on P/ECE
	* 	By Yui N., 2003.
	* http://www.autch.net/
*/

#ifndef LIBFPK_H
#define LIBFPK_H

#include "Win2Piece.h"
#include <piece.h>
#include "fpk.h"
#include "pceFileReadPos.h"

typedef struct
{
	FPKHEADER	fpkHeader;
	FILEACC pfa;
}FPK, *PFPK;

typedef PFPK HFPK;

HFPK fpkOpenArchive(LPSTR lpszFileName);

// C にも関数オーバーロードがあれば……（ぉ
BOOL fpkGetFileInfoS(HFPK hFpk, LPSTR lpszFileName, FPKENTRY *lpFileEntry);
BOOL fpkGetFileInfoN(HFPK hFpk, DWORD dwFileIndex, FPKENTRY *lpFileEntry);

BYTE *fpkExtractToBuffer(HFPK hFpk, FPKENTRY *fpkEntry);
VOID fpkCloseArchive(HFPK hFpk);

#endif /* !LIBFPK_H */
