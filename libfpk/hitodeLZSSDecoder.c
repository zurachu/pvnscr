/*
	* libfpk -- using FPK archive on P/ECE
	* 	By Yui N., 2003.
	* http://www.autch.net/
*/

/*
	* reference: http://www.aw.wakwak.com/~hitode/piece/index.html#plz
	* author: Hitode Yamatsuki
	* for more information, see libfpk.txt
*/

#include "hitodeLZSSDecoder.h"

void hitodeLZSSDecode(BYTE *dst, BYTE *src, DWORD size)
{
	BYTE *base = src;

	while((src - base) < size)		// yui: fpk のパディングバイトの関係でサイズチェックが必要
	{
		int ct = 8, flag = *src++;

		do
		{
			if(flag & 0x80)
			{
				*dst++ = *src++;
			}
			else
			{
				unsigned int length, ptr = *src++;

				length = (ptr >> 4) + 3;
				if(!(ptr = ((ptr & 0xf) << 8) | (*src++)))
					return;
				{
					unsigned char *rp = dst - ptr;
					do
					{
						*dst++ = *rp++;
					}while(--length);
				}
			}
			flag <<= 1;
		}while(--ct);
	}
}
