/*******************************************************************************
//  �ėp�Q�[�����C�u����
//  (c)2003 �ĂƂ灚�ۂ���
*******************************************************************************/

#ifndef _GAME_LIBRARY_H_

#define _GAME_LIBRARY_H_

extern unsigned char *vbuff;	// ��ʃo�b�t�@
extern BOOL LCDUpdate;

//=============================================================================
//  �N��������
//=============================================================================
extern void App_Init();

//=============================================================================
//  �I��������
//=============================================================================
extern void App_Exit();

//=============================================================================
//  P/ECE�p�r�b�g�}�b�v����
//=============================================================================
extern void Get_PieceBmp( PIECE_BMP* pBmp, unsigned char* data );

//=============================================================================
//  P/ECE�p�r�b�g�}�b�v�`��
//=============================================================================
extern void Draw_Object( PIECE_BMP *pBmp, int dx, int dy, int sx, int sy, int w, int h, int param );

//=============================================================================
//  P/ECE�pPCM��������
//=============================================================================
extern void Get_PieceWave( PCEWAVEINFO* pWav, unsigned char* data );


//=============================================================================
//  ����蕶���`��
//=============================================================================
extern int wFontPrintf( short c, short x, short y, const char* fmt, ... );

#endif