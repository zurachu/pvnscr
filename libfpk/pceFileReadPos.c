/*
	* libfpk -- using FPK archive on P/ECE
	* 	By Yui N., 2003.
	* http://www.autch.net/
*/

/*
	* reference: 「おでかけマルチ」のソースコードより piece_ex.c
	* original copyright: AQUA (Leaf)  since 2001 - 
	* original coder: Programd by.  Miyakusa Masakazu
*/

#include "pceFileReadPos.h"

int pceFileReadPos(FILEACC *pfa, unsigned char *buf, int pos, int size)
{
	int work = pos;
	int size2 = 0;

	if(pos % 4096)
	{
		pceFileReadSct(pfa, NULL, work / 4096, 4096);
		size2 += memcpy(buf, pfa->aptr + pos % 4096, min(4096 - pos % 4096, size));
		work += min(4096 - pos % 4096, size);
	}
	while( pos+size > work )
	{
		pceFileReadSct(pfa, NULL, work / 4096, min(pos + size - work, 4096));
		size2 += memcpy(buf + (work - pos), pfa->aptr, min(pos + size - work, 4096));
		work += 4096;
	}

	return size2;
}
