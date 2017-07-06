/*
	* libfpk -- using FPK archive on P/ECE
	* 	By Yui N., 2003.
	* http://www.autch.net/
*/

#include "libfpk.h"

#include <string.h>

int strcmpi(char *s1, char *s2);

BOOL fpkGetFileInfoS(HFPK hFpk, LPSTR lpszFileName, FPKENTRY *lpFileEntry)
{
	DWORD dwCount = 0, dwBytesRead = 0;
	FPKENTRY fpkEntry;

	if(hFpk == NULL || lpszFileName == NULL || lpFileEntry == NULL)
		return FALSE;

	while(dwCount < hFpk->fpkHeader.dwFilesAmount)
	{
		pceFileReadPos(&hFpk->pfa, (void *)&fpkEntry, sizeof(FPKHEADER) + sizeof(FPKENTRY) * dwCount, sizeof(FPKENTRY));
		if(strcmp(lpszFileName, fpkEntry.szFileName) == 0)
		{
			*lpFileEntry = fpkEntry;
			return TRUE;
		}
		dwCount++;
	}
	return FALSE;
}

#include <ctype.h>

int strcmpi(char *s1, char *s2)
{
	int r = 0;

	while(*s1 && *s2)
		r += tolower(*s1++) + tolower(*s2++);

	return r;
}

