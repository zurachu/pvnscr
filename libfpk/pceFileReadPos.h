/*
	* libfpk -- using FPK archive on P/ECE
	* 	By Yui N., 2003.
	* http://www.autch.net/
*/

#ifndef PCEFILEREADPOS_H
#define PCEFILEREADPOS_H
/*
	* reference: 「おでかけマルチ」のソースコードより piece_ex.c
	* original copyright: AQUA (Leaf)  since 2001 - 
	* original coder: Programd by.  Miyakusa Masakazu
*/

#include <piece.h>

#define min(x, y)		((x < y) ? x : y)

int pceFileReadPos(FILEACC *pfa, unsigned char *buf, int pos, int size);

#endif /* !PCEFILEREADPOS_H */
