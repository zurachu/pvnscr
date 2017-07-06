/*
	* libfpk -- using FPK archive on P/ECE
	* 	By Yui N., 2003.
	* http://www.autch.net/
*/

#include "libfpk.h"

BOOL fpkGetFileInfoN(HFPK hFpk, DWORD dwFileIndex, FPKENTRY *lpFileEntry)
{
	DWORD dwCount = 0, dwBytesRead = 0;
	FPKENTRY fpkEntry;

	if(hFpk == NULL || lpFileEntry == NULL)
		return FALSE;

	if(dwFileIndex >= hFpk->fpkHeader.dwFilesAmount)
		return FALSE;

	pceFileReadPos(&hFpk->pfa, (void *)&fpkEntry, sizeof(FPKHEADER) + sizeof(FPKENTRY) * dwFileIndex, sizeof(FPKENTRY));
	*lpFileEntry = fpkEntry;
	return TRUE;
}
