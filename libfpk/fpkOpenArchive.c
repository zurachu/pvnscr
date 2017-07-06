/*
	* libfpk -- using FPK archive on P/ECE
	* 	By Yui N., 2003.
	* http://www.autch.net/
*/
#include "libfpk.h"

HFPK fpkOpenArchive(LPSTR lpszFileName)
{
	PFPK fpk;

	fpk = pceHeapAlloc(sizeof(FPK));
	if(fpk == NULL)	return NULL;

	if(pceFileOpen(&fpk->pfa, lpszFileName, FOMD_RD) != 0)
	{
		pceHeapFree(fpk);
		return NULL;
	}

	pceFileReadPos(&fpk->pfa, (void *)&fpk->fpkHeader, 0, sizeof(FPKHEADER));

	return fpk;
}
