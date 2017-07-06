/*
	* libfpk -- using FPK archive on P/ECE
	* 	By Yui N., 2003.
	* http://www.autch.net/
*/
#include "libfpk.h"

VOID fpkCloseArchive(HFPK hFpk)
{
	pceFileClose(&hFpk->pfa);
	pceHeapFree(hFpk);
}
