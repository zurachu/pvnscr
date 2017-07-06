/****************************************/
/*                                      */
/*      P/ECE VisualNovel Scripter      */
/*                                      */
/* (c)2004-2005 �d��Chu���ĂƂ灚�ۂ��� */
/*                                      */
/****************************************/

#include "common.h"
#include <ctype.h>
#include "gamelib.h"
#include "sfont.h"
#include "htomei2.h"
#include "muslib2.h"

#include "file.h"

int errno;

BOOL demo;
unsigned short gameMode;

unsigned char *pData;	// �X�N���v�g�f�[�^
short ver;				// �X�N���v�g�o�[�W����

SAVE_DATA sData;

unsigned char *pgd[7];	// �摜�t�@�C���f�[�^
unsigned char *pmd;		// BGM�t�@�C���f�[�^
unsigned char *ppd[3];	// SE�t�@�C���f�[�^
PCEWAVEINFO   pWav[3];	// SE�Đ��p�i�O���[�o���Ő錾���Ȃ��ƃG���[���N����j

SCRIPT_POINTER label[_SIZEOF_LABEL_];	// ���x���|�C���^
SCRIPT_POINTER jump[_SIZEOF_JUMP_];		// sel���ł̔�ѐ擙
short textPtr;							// �e�L�X�g�\���|�C���^

BOOL saveAllow;			// �Z�[�u����
BOOL lineFeedKeyWait;	// ���s�ɂ��L�[�҂��y�[�W����iPVNS2�ȑO�j
BOOL clickStrFlag;		// �����L�[�҂������Q�ɂ��L�[�҂�
BOOL carriageReturn;	// �L�����b�W���^�[���̉�

short waitTime;			// �E�F�C�g���ԁiPVNS2�ȑO=100ms�APVNSCR=10ms�j
BOOL waitSkip;			// �N���b�N�ɂ��E�F�C�g��΂��̉�

extern unsigned char ARROW[];
unsigned char arrow[7*8];

/***2�o�C�g��������֐�***/
BOOL IsKanji(unsigned char cData)
{
	if(cData < 0x81) { return FALSE; }
	if(cData < 0xa0) { return TRUE;  }
	if(cData < 0xe0) { return FALSE; }
	if(cData < 0xff) { return TRUE;  }
	return FALSE;
}


//=============================================================================
//  ������������
//=============================================================================
void pvns_MemoryClear(void)
{
	short i;
	
	for(i = 0; i < 7; i++) {	// pgd������
		pceHeapFree(pgd[i]);
		pgd[i] = NULL;
	}
	for(i = 1; i <= 3; i++) {	// ppd������
		pceWaveAbort(i);
		pceHeapFree(ppd[i]);
		ppd[i] = NULL;
	}
	StopMusic();				// pmd������
	pceHeapFree(pmd);
	pmd = NULL;
}


//=============================================================================
//  �ϐ�������
//=============================================================================
void pvns_VariableClear(void)
{
	short i;
	
	memset(&sData, 0, sizeof(SAVE_DATA));
	sData.sp.ptr = _HEADER_LENGTH_;
	sData.sp.line = 1;
	sData.win.pos = 1;
	sData.win.line = 3;
	sData.win.font = 0;
	sData.win.color = 1;
	for(i = 0; i < _PGD_POS_BG_; i++) { sData.zOrder[i] = i; }
	sData.zOrder[1] = 2;
	sData.zOrder[2] = 1;
	textPtr = 0;
	lineFeedKeyWait = FALSE;
	clickStrFlag = TRUE;
	carriageReturn = TRUE;
}


//=============================================================================
//  �L�[�҂����`��
//=============================================================================
void pvns_DrawArrow(short flag)
{
	PIECE_BMP pBmp;
	short i;
	short x = (textPtr % _TEXT_LENGTH_) * _FONT_WIDTH_;
	short y = (textPtr / _TEXT_LENGTH_) * 10 + 3;
	if(sData.win.line == 8) {
		y += 4;
	} else {
		if(sData.win.pos) { y += 86 - sData.win.line * 10; }
	}
	if(!x) {
		x = 120;
		y -= 10;
	}
	if(!sData.win.font) { x++; }
	
	if(flag) {
		for(i = 0; i < 8; i++) {
			memcpy(arrow+i*7, vbuff+128*(y+i)+x, 7);
		}
		Get_PieceBmp(&pBmp, ARROW);
		Draw_Object(&pBmp, x, y, 8 - sData.win.color * 8, 0, pBmp.header.w/2, pBmp.header.h, DRW_NOMAL);
	} else {
		for(i = 0; i < 8; i++) {
			memcpy(vbuff+128*(y+i)+x, arrow+i*7, 7);
		}
	}
	LCDUpdate = TRUE;
}


//=============================================================================
//  �^�C�g���o�[�`��
//=============================================================================
void pvns_DrawTitleBar(short y)
{
	pceLCDPaint(2, 0, y, 128, 8);
	pceFontSetType(2);
	pceFontSetBkColor(FC_SPRITE);
	pceFontSetTxColor(0);
	pceFontSetPos(2, y+2);
	pceFontPutStr(_TITLE_);
}


//=============================================================================
//  �g�t���E�B���h�E�`��
//=============================================================================
void pvns_DrawWindow(short x, short y, short w, short h)
{
	pceLCDPaint(_WIN_COLOR_, x, y, w, h);
	pceLCDPaint(_FONT_COLOR_, x+1, y+1, w-2, 1);
	pceLCDPaint(_FONT_COLOR_, x+1, y+h-2, w-2, 1);
	pceLCDPaint(_FONT_COLOR_, x+1, y+1, 1, h-2);
	pceLCDPaint(_FONT_COLOR_, x+w-2, y+1, 1, h-2);
}


//=============================================================================
//  ���b�Z�[�W�_�C�A���O�\��
//=============================================================================
void pvns_DrawMessageDialog(char* str)
{
	pvns_DrawWindow(5, 28, 120, 32);	// �E�B���h�E��`��
	
	sFontStatus.x = 64 - strlen(str) * 2;
	sFontStatus.y = 39;
	_SetFontColor;
	sFontPutStr(str);	// �k���t�H���g�ŕ\��
/*
pceFontSetPos(64 - strlen(str) * 3, 39);
pceFontPutStr(str);
*/
	LCDUpdate =TRUE;
}


//=============================================================================
//  �w��͈͓��m�[�}�������`��
//=============================================================================
void pvns_FontPutStr(char* str, short x, short y, short minx)
{
	short p = 0;
	
	while(*(str+p) != '\0') {
		if(x + IsKanji(*(str+p)) * 5 + 5 > 127) {
			y += 10;
			x = minx;
		}
		pceFontSetPos(x, y);
		if(IsKanji(*(str+p))) {
			pceFontPrintf("%c%c", *(str+p), *(str+p+1));
			x += 10;
			p += 2;
		} else {
			pceFontPrintf("%c", *(str+p));
			x += 5;
			p++;
		}
	}
}


//=============================================================================
//  �Q�[����ʃO���t�B�b�N�`��
//=============================================================================
void pvns_DrawGameGraphic(void)
{
	PIECE_BMP pBmp;		// �r�b�g�}�b�v�\����
	short dx, dy, i;
	
	if(!textPtr) {
		// �w�i
		if(sData.bgColor == _BG_GRAPHIC_) {	// �摜�w��̏ꍇ
			pceLCDPaint(0,0,0,128,88);
			Get_PieceBmp(&pBmp, pgd[_PGD_POS_BG_]);	// �r�b�g�}�b�v�擾
			for(dy = 0; dy < 88; dy += pBmp.header.h) {	// ��ʂɕ��ׂĕ`��
				for(dx = 0; dx < 128; dx += pBmp.header.w) {
					Draw_Object(&pBmp, dx, dy, 0, 0, pBmp.header.w, pBmp.header.h, DRW_NOMAL);
				}
			}
		} else {							// �F�w��̏ꍇ
			pceLCDPaint(sData.bgColor, 0, 0, 128, 88);	// �w�i�F�œh��Ԃ�
		}
		// �����G
		for(i = 0; i < _PGD_POS_SP_; i++) {	// zOrder�̏d�ˏ��ɕ`��
			if(*sData.pgdName[sData.zOrder[i]]) {
				Get_PieceBmp(&pBmp, pgd[sData.zOrder[i]]);	// �r�b�g�}�b�v�擾
				dx = 0;
				switch(sData.zOrder[i]) {	// �ʒu�ɉ�����x���W���w��
					case 0:		// ��
						dx = 0;
						break;
					case 1:		// ����
						dx = (128 - pBmp.header.w) / 2;
						break;
					case 2:		// �E
						dx = 128 - pBmp.header.w;
						break;
				}
				Draw_Object(&pBmp, dx, 88 - pBmp.header.h, 0, 0, pBmp.header.w, pBmp.header.h, DRW_NOMAL);
			}
		}
		// �X�v���C�g
		for(i = 0; i < 3; i++) {
			if(*sData.pgdName[_PGD_POS_SP_ + i] && sData.sprV[i]) {
				Get_PieceBmp(&pBmp, pgd[_PGD_POS_SP_ + i]);	// �r�b�g�}�b�v�擾
				Draw_Object(&pBmp, sData.sprX[i], sData.sprY[i], 0, 0, pBmp.header.w, pBmp.header.h, DRW_NOMAL);
			}
		}
		
		if(strlen(sData.msg) && !(gameMode & _GAMEMODE_WINOFF_)) {
			// �������E�B���h�E
			if(sData.win.color == 0) { i = 2; }
			else					 { i = 4; }
			if(sData.win.line == 8) {	// �S���
				hanToumeiAll(i);
			} else {
				if(sData.win.pos) {	// �E�B���h�E�ʒu��
					hanToumei(i, 0, 86 - sData.win.line * 10, 128, sData.win.line * 10 + 2);
				} else {			// �E�B���h�E�ʒu��
					hanToumei(i, 0, 0, 128, sData.win.line * 10 + 2);
				}
			}
		}
	}
	
	// ���������`��
	if(textPtr < strlen(sData.msg) && !(gameMode & _GAMEMODE_WINOFF_)) {
		sFontStatus.x = (textPtr % _TEXT_LENGTH_) * _FONT_WIDTH_ + 1;
		sFontStatus.xMin = 1;
		sFontStatus.y = (textPtr / _TEXT_LENGTH_) * 10 + 1;
		if(sData.win.font && !(textPtr % _TEXT_LENGTH_)) {
			sFontStatus.x--;
		}
		if(sData.win.line == 8) {
			sFontStatus.y += 4;
		} else {
			if(sData.win.pos) { sFontStatus.y += 86 - sData.win.line * 10; }
		}
		_SetFontColor;
		if(sData.win.font) {
			sFontStatus.xMin = 0;
			sFontPutStr(sData.msg + textPtr);
		} else {
			pceFontSetTxColor(_WIN_COLOR_);
			for(dy = sFontStatus.y - 1; dy <= sFontStatus.y + 1; dy++) {
				for(dx = sFontStatus.x - 1; dx <= sFontStatus.x + 1; dx++) {
					if(dx != sFontStatus.x || dy != sFontStatus.y) {
						pvns_FontPutStr(sData.msg + textPtr, dx, dy, sFontStatus.xMin + dx - sFontStatus.x);
					}
				}
			}
			pceFontSetTxColor(_FONT_COLOR_);
			pvns_FontPutStr(sData.msg + textPtr, sFontStatus.x, sFontStatus.y, sFontStatus.xMin);
		}
		textPtr = strlen(sData.msg);
	}
	
/*	//�o�O���o�p
	pceFontSetPos(0, 60);
	pceFontPrintf("%s,%d\n%s,%d", sData.pgdName[0], pgd[0], sData.pgdName[2], pgd[2]);
*/	
	LCDUpdate = TRUE;
}


//=============================================================================
//  SE�̍Đ�
//=============================================================================
BOOL pvns_PlaySound(const char* fName, short ch)
{
	if(ch < 1 || ch > 3) { return FALSE; }	// �s���ȃ`���l���̓G���[
	
	pceWaveAbort(ch);	// �Y���`���l����Wave�o�͂𒆎~
	pceHeapFree(ppd[ch]);
	ppd[ch] = NULL;
	if((ppd[ch] = pvns_LoadResourceFile(fName, _TYPE_PPD_)) == NULL) {
		return FALSE;
	}
	Get_PieceWave(&pWav[ch], ppd[ch]);
	pceWaveDataOut(ch, &pWav[ch]);	// �Y���`���l����Wave�o��
	return TRUE;
}


//=============================================================================
//  �G���[���b�Z�[�W�\��
//=============================================================================
void pvns_SetError(char* msg)
{
	char str[30];
//	sprintf(str, "%d:%s", sData.sp.line, msg);
	sprintf(str, "%d:%d:%s", sData.sp.line, sData.sp.ptr, msg);
	pvns_DrawMessageDialog(str);
	LCDUpdate = TRUE;
	gameMode |= _GAMEMODE_ERROR_;
}

//=============================================================================
//  �^�C�g���o�[���S�\��
//=============================================================================
void pvns_DrawLogo(void)
{
	static short y = 88;
	
	y--;
	if(pcePadGet() & (TRG_A | TRG_C) && !(gameMode & _GAMEMODE_LCDINIT_)) { y = 0; }	// A��START�ŏȗ�
	if(pcePadGet() & PAD_D) { pceAppReqExit(0); }	// SELECT�������ꂽ���I��
	pceLCDPaint(0, 0, 0, 128, 88);
	pvns_DrawTitleBar(y);
	LCDUpdate = TRUE;
	if(y <= 0) {
		y = 88;					// �ēx�Ăяo�����̂��߂Ƀo�[�ʒu������
		demo = FALSE;			// ��f�����[�h
		sData.win.color = 1;	// �E�B���h�E��
		pvns_DrawMessageDialog(_WAIT_MESSAGE_);
		gameMode = _GAMEMODE_LAUNCH_ | _GAMEMODE_PREPARE_;
	}
}


//=============================================================================
//  �V�i���I�����`������
//=============================================================================
void pvns_PrepareLaunch(void)
{
	/* ���ӁFsData.variable[]���X�N���v�g�t�@�C���ԍ���擾�p�Ɏg���񂵂Ă܂� */
	if(pvns_LaunchGetFileArray(sData.variable)) {
		gameMode ^= _GAMEMODE_PREPARE_ | _GAMEMODE_LCDINIT_;
	} else {
		pvns_DrawMessageDialog("�X�N���v�g������܂���");
		gameMode = _GAMEMODE_ERROR_;
	}
}


//=============================================================================
//  �V�i���I�����`��
//=============================================================================
void pvns_Launch(void)
{
	static short index = 0, cursor = 0;
	short        fAmount, i;
	FILEINFO     pfi;

	for(fAmount = 0; sData.variable[fAmount] != -1; fAmount++);	// �t�@�C�����擾

	if(pcePadGet() & TRG_UP) {	// ���������ꂽ
		LCDUpdate = TRUE;
		if(cursor == 0) {		// �J�[�\�����ŏ�i�̂Ƃ�
			if(fAmount <= 8) {	// �t�@�C����8�ȉ��i�X�N���[���̕K�v�Ȃ��j
				cursor = fAmount - 1;
			} else {				// �t�@�C����9�ȏ�i�v�X�N���[���j
				index--;
				if(index < 0) {
					cursor = 7;
					index = fAmount - 8;
				}
			}
		} else {
			cursor--;
		}
	}

	if(pcePadGet() & TRG_DN) {	// ���������ꂽ
		LCDUpdate = TRUE;
		cursor++;
		if(cursor >= fAmount) {	// �t�@�C����8�ȉ��i�X�N���[���s�v�j
			cursor = 0;
		}
		if(cursor >= 8) {			// �v�X�N���[�����J�[�\�����ŉ��i�̂Ƃ�
			cursor = 7;
			index++;
			if(index > fAmount - 8) {
				cursor = 0;
				index = 0;
			}
		}
	}
	
	/* A�܂���START�������ꂽ�iLOGO����A�܂���START�Ŕ��ł����ꍇ������j */
	if(!(gameMode & _GAMEMODE_LCDINIT_) && pcePadGet() & (TRG_A | TRG_C)) {
		pfi = pvns_LaunchGetFileInfo(sData.variable[index+cursor]);	// �t�@�C�����擾
		/* ���ӁFsData.msg���t�@�C�����擾�p�Ɏg���񂵂Ă܂� */
		strcpy(sData.msg, pfi.filename);
		gameMode = _GAMEMODE_SCRIPT_ | _GAMEMODE_PREPARE_;
		LCDUpdate = TRUE;
	}
	
	if(pcePadGet() & PAD_D) { pceAppReqExit(0); }	// SELECT�������ꂽ���I��
	
	if(LCDUpdate) {
		pceLCDPaint(0, 0, 8, 128, 80);
		pceFontSetType(0);
		pceFontSetBkColor(FC_SPRITE);
		pceFontSetTxColor(3);
		pceFontSetPos(0, 8);
		for(i = index; i < index+8 && i < fAmount; i++) {
			pfi = pvns_LaunchGetFileInfo(sData.variable[i]);	// �t�@�C�����擾
			pceFontPrintf(" %-15s%7ld\n", pfi.filename, pfi.length);	// ��ʂɕ`��
		}
		hanToumei(7, 0, cursor*10+8, 128, 10);	// �I�����ڂ𔽓]
		if(gameMode & _GAMEMODE_PREPARE_) { pvns_DrawMessageDialog(_WAIT_MESSAGE_); }
	}
}

//=============================================================================
//  �X�N���v�g�����i���K�����j
//=============================================================================
void pvns_PrepareScript(void)
{
	short i = 0;
	
	pData = pvns_LoadScriptFile(sData.msg);	// �t�@�C���ǂݍ���
	ver = pvns_CheckScriptHeader(pData);	// �X�N���v�g�o�[�W�����擾
	memset(label, 0, sizeof(SCRIPT_POINTER)*_SIZEOF_LABEL_);	// ���x���L���N���A
	sData.sp.line = 1;
	
	for(sData.sp.ptr = _HEADER_LENGTH_; _NOW_WORD_ != '\0'; sData.sp.ptr++) {
		if(_NOW_WORD_ == '\t') {	// �^�u�𔼊p�󔒂�
			_NOW_WORD_ = ' ';
			continue;
		}
		if(_NOW_WORD_ == ';') {	// ';'�`�s���i�R�����g�j�𔼊p�󔒂�
			while(_NOW_WORD_ != '\r' && _NOW_WORD_ != '\n' && _NOW_WORD_ != '\0') {
				if(IsKanji(_NOW_WORD_)) {
					_NOW_WORD_ = ' ';
					sData.sp.ptr++;
				}
				_NOW_WORD_ = ' ';
				sData.sp.ptr++;
			}
		}
		if(_NOW_WORD_ == '\r') {
			if(*(pData+sData.sp.ptr+1) == '\n') {	// CR+LF�𔼊p��+LF��
				_NOW_WORD_ = ' ';
				sData.sp.ptr++;
			} else {
				_NOW_WORD_ = '\n';			// CR��LF��
			}
		}
		if(_NOW_WORD_ == '\n') {	// LF�͍s���J�E���g
			sData.sp.line++;
			/* LF+'*'�i���x���j�͈ʒu��128�܂ŋL���igosub,goto�̍������̂��߁j */
			if(*(pData+sData.sp.ptr+1) == '*') {
				sData.sp.ptr++;	// '*'�̈ʒu���L��
				if(i >= _SIZEOF_LABEL_) {
					for(i = 0; i < _SIZEOF_LABEL_ - 1; i++) {
						label[i] = label[i+1];
					}
				}
				label[i] = sData.sp;
				i++;
			}
		}
	}
	
	/* �e��ϐ������� */
	pvns_VariableClear();
	pvns_SaveFileClear();
	gameMode ^= _GAMEMODE_PREPARE_;
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
// ���߉�͂́������i�s�������̂ŕ����܂��j
#include "command.c"
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
//=============================================================================
//  �X�N���v�g��́iTRUE��Ԃ��ԓ���������j
//=============================================================================
BOOL pvns_AnalyzeScript(void)
{
	static char str[6];
	short i;
	
	// ���b�Z�[�W�{�ϐ���1��ʂɎ��܂�Ȃ���΃L�[�҂��y�[�W����
	if(strlen(sData.msg) + strlen(str)*2 >= _TEXT_MAX_) {
		_FillMsgWindow;
		gameMode = _GAMEMODE_KEYWAIT_ | _GAMEMODE_LCDINIT_;
		return FALSE;
	}
	// �ϐ��o�b�t�@�ɕ����܂��͐������c���Ă���ΐ�ɕ\��
	if(*str) {
		if(isdigit(str[0])) {
			strcat(sData.msg, "�O");
			sData.msg[strlen(sData.msg)-1] += str[0] - '0';	// ����
		} else {
			strcat(sData.msg, "�|");				// �}�C�i�X����
		}
		for(i = 0; i < 4; i++) { str[i] = str[i+1]; }	// �\�������������o�b�t�@�������
		lineFeedKeyWait = TRUE;	// \�ł̉��s�ɂ��L�[�҂��y�[�W�������I�������̂Ō��ɖ߂�
		clickStrFlag = TRUE;	// _�ł̃L�[�҂�����I�������̂Ō��ɖ߂�
		carriageReturn = FALSE;	// �e�L�X�gx���W0�ł̃L�����b�W���^�[����s����
		if(sData.msgSpeed > 0) { _SetWait(sData.msgSpeed, TRUE); }	// !s�ɂ��1�������̃E�F�C�g
		return pcePadGet() & PAD_RI;
	}
	
	while(_NOW_WORD_ == ' ') { sData.sp.ptr++; }	// �󔒂͓ǂݔ�΂�
	
	switch(_NOW_WORD_) {
		case '\0':	// �I�[����
			if(sData.msg[0] != '\0') {	// ���b�Z�[�W������΃L�[�҂��y�[�W����
				_FillMsgWindow;
				return TRUE;
			}
			pceAppReqExit(0);	// ���b�Z�[�W���Ȃ���ΏI��
			return FALSE;
			break;
		case '\n':	// ���s
			sData.sp.ptr++;
			sData.sp.line++;
			if(lineFeedKeyWait) {
				if(_LEGACY_VERSION_) {	// PVNS2�ȑO���L�[�҂��y�[�W����
					_FillMsgWindow;
					return FALSE;
				} else {					// PVNSCR�����̍s���ցi���ݍs���̏ꍇ�͖����j
					while(strlen(sData.msg) % _TEXT_LENGTH_) {
						strcat(sData.msg, "�@");
					}
					carriageReturn = TRUE;
				}
			}
			return TRUE;
			break;
		case '*':	// ���x��
			sData.sp.ptr++;
			while(_NOW_WORD_ != ' ' && _NOW_WORD_ != '\n' && _NOW_WORD_ != '\0') {
				sData.sp.ptr++;
			}	// �󔒁A���s�܂œǂݔ�΂�
			return TRUE;
			break;
		case '%':	// %�i���[�J���ϐ��j
		case '$':	// $�i�O���[�o���ϐ��j
			if(pvns_GetValueOfArgument(&i)) {
				sprintf(str, "%d", i);
				return TRUE;
			}
			break;
		case '@':	// @
			if(!_LEGACY_VERSION_) {	// PVNSCR���L�[�҂�
				sData.sp.ptr++;
				if(sData.msg[0]) {
					gameMode = _GAMEMODE_KEYWAIT_ | _GAMEMODE_LCDINIT_;
					return FALSE;
				}
				return TRUE;
			}
			break;
		case '_':	// _
			sData.sp.ptr++;
			if(!_LEGACY_VERSION_) {	// PVNSCR����������clickstr����
				clickStrFlag = FALSE;
				return TRUE;
			}
			break;
		case '/':	// /
			if(!_LEGACY_VERSION_) {	// PVNSCR���������܂ŉ��s������
				sData.sp.ptr++;
				lineFeedKeyWait = FALSE;
				return TRUE;
			}
			break;
		case '!':	// !
			if(!_LEGACY_VERSION_) {	// PVNSCR�����ꕶ��
				sData.sp.ptr++;
				switch(_NOW_WORD_) {
					case 'd':	// !d	�e�L�X�g�\���҂��i�N���b�N�ȗ��j
						sData.sp.ptr++;
						if(pvns_GetValueOfArgument(&i)) {
							_SetWait(i, TRUE);
						} else {
							/* �G���[���� */
							pvns_SetError("!d");
						}
						return FALSE;
						break;
					case 'w':	// !w	�e�L�X�g�\���҂��i�N���b�N�ȗ��s�j
						sData.sp.ptr++;
						if(pvns_GetValueOfArgument(&i)) {
							_SetWait(i, FALSE);
						} else {
							/* �G���[���� */
							pvns_SetError("!w");
						}
						return FALSE;
						break;
					case 's':	// !s	�e�L�X�g���x�ύX�iVer3.1�����j
						sData.sp.ptr++;
						if(pvns_GetValueOfArgument(&i)) {
							sData.msgSpeed = i;
						} else {
							/* �G���[���� */
							pvns_SetError("!s");
						}
						
						break;
				}
			}
			break;
		case '\\':	// '\'
			sData.sp.ptr++;
			if(_LEGACY_VERSION_) {	// PVNS2�ȑO�����ꕶ��
				switch(_NOW_WORD_) {
					case 'n':	// \n	���b�Z�[�W���ŉ��s
						sData.sp.ptr++;
						if(carriageReturn && !(strlen(sData.msg) % _TEXT_LENGTH_)) {
							strcat(sData.msg, "�@");
						}
						while(strlen(sData.msg) % _TEXT_LENGTH_) {
							strcat(sData.msg, "�@");
						}
						carriageReturn = TRUE;
						return TRUE;
						break;
					case 'c':	// \c	���b�Z�[�W�������I�ɏ���
						sData.sp.ptr++;
						sData.msg[0] = '\0';
						textPtr = 0;
						lineFeedKeyWait = FALSE;
						carriageReturn = TRUE;
						return FALSE;
						break;
					case 'w':	// \w
						sData.sp.ptr++;
						gameMode = _GAMEMODE_KEYWAIT_ | _GAMEMODE_LCDINIT_;
						return FALSE;
						break;
					default:	// \	�������܂ŉ��s������
						lineFeedKeyWait = FALSE;
						return TRUE;
						break;
				}
			} else {					// PVNSCR���L�[�҂��y�[�W����
				if(sData.msg[0]) { _FillMsgWindow; }
				return TRUE;
			}
			break;
	}
	
	if(IsKanji(_NOW_WORD_)) {	// 2�o�C�g�����i��ʂɕ\���j
		strncat(str, pData+sData.sp.ptr, 2);
		sData.sp.ptr += 2;
		strcat(sData.msg, str);
		// PVNSCR��clickstr�Ɋ܂܂��Ȃ�΃L�[�҂��ɓ���
		if(!_LEGACY_VERSION_ && clickStrFlag && strstr(sData.clickStr, str) != NULL ) {
			str[0] = '\0';
			if(strlen(sData.msg) / _TEXT_LENGTH_ >= sData.win.line - sData.clickLine) {
				_FillMsgWindow;
			}
			gameMode = _GAMEMODE_KEYWAIT_;
			return FALSE;
		}
		str[0] = '\0';
		lineFeedKeyWait = TRUE;	// \�ł̉��s�ɂ��L�[�҂��y�[�W�������I�������̂Ō��ɖ߂�
		clickStrFlag = TRUE;	// _�ł̃L�[�҂�����I�������̂Ō��ɖ߂�
		carriageReturn = FALSE;	// �e�L�X�gx���W0�ł̃L�����b�W���^�[����s����
		if(sData.msgSpeed > 0) { _SetWait(sData.msgSpeed, TRUE); }	// !s�ɂ��1�������̃E�F�C�g
		return (pcePadGet() & PAD_RI);
	}
	
	/* �ȏ�ɊY�����Ȃ���Ζ��߉�͂� */
	return(pvns_AnalyzeCommand());
}


//=============================================================================
//  ���b�Z�[�W����҂�
//=============================================================================
void pvns_KeyWait()
{
	static short flag = 1, wait = 0;
	
	if(LCDUpdate) {
		pvns_DrawGameGraphic();
		flag = 1;
		wait = 0;
	}
	
	if(wait <= 0) {
		pvns_DrawArrow(flag);
		flag = !flag;
		wait = 30;
	} else {
		wait--;
	}
	
	if(!(gameMode & _GAMEMODE_LCDINIT_) && pcePadGet() & (TRG_A | PAD_RI)) {	// A�{�^���܂��͉E�ő�����ǂݍ���
		pvns_DrawArrow(0);
		if(strlen(sData.msg) >= _TEXT_LENGTH_ * sData.win.line) {
			sData.msg[0] = '\0';
			textPtr = 0;
			lineFeedKeyWait = FALSE;
			carriageReturn = TRUE;
		}
		flag = 1;
		wait = 0;
		gameMode = _GAMEMODE_SCRIPT_;
	}
	if(pcePadGet() & TRG_B) {
		flag = 1;
		wait = 0;
		textPtr = 0;
		gameMode |= _GAMEMODE_WINOFF_;
		pvns_DrawGameGraphic();
	}
	if(pcePadGet() & (TRG_C | TRG_D)) {
		gameMode |= _GAMEMODE_MENU_ | _GAMEMODE_LCDINIT_;
	}
}


//=============================================================================
//  �I��������҂�
//=============================================================================
void pvns_SelWait()
{
	static short cursor = 0;
	static BOOL keyRelease = FALSE;
	short y, i;
	
	if(pcePadGet() & TRG_UP) {
		cursor--;
		if(cursor < 0) {
			cursor = 7;
			while(!jump[cursor].ptr) { cursor--; }
		}
		LCDUpdate = TRUE;
	}
	if(pcePadGet() & TRG_DN) {
		cursor++;
		if(cursor >= 8 || !jump[cursor].ptr) {
			cursor = 0;
		}
		LCDUpdate = TRUE;
	}
	if(!(pcePadGet() & PAD_A)) { keyRelease = TRUE; }
	if(keyRelease && pcePadGet() & TRG_A) {	// A�{�^���ŃW�����v
		sData.sp = jump[cursor];
		sData.msg[0] = '\0';
		textPtr = 0;
		lineFeedKeyWait = FALSE;
		carriageReturn = TRUE;
		cursor = 0;
		keyRelease = FALSE;
		gameMode = _GAMEMODE_SCRIPT_;
	}
	
	if(LCDUpdate) {
		textPtr = 0;
		pvns_DrawGameGraphic();
		for(i = 0; i < 8 && jump[i].ptr; i++);
		y = (strlen(sData.msg) / _TEXT_LENGTH_ - i + cursor) * 10;
		if(sData.win.line == 8) {
			y += 4;
		} else {
			if(sData.win.pos) { y += 86 - sData.win.line * 10; }
		}
		hanToumei(5, 0, y, 128, 11);
	}
	
	if(pcePadGet() & TRG_B) {
		keyRelease = FALSE;
		for(i = 0; i < 8 && jump[i].ptr; i++);
		sData.msg[strlen(sData.msg) - i * _TEXT_LENGTH_] = '\0';
		textPtr = 0;
		gameMode = _GAMEMODE_SCRIPT_ | _GAMEMODE_WINOFF_;
		pvns_DrawGameGraphic();
	}
	if(pcePadGet() & (TRG_C | TRG_D)) {
		keyRelease = FALSE;
		for(i = 0; i < 8 && jump[i].ptr; i++);
		sData.msg[strlen(sData.msg) - i * _TEXT_LENGTH_] = '\0';
		gameMode = _GAMEMODE_SCRIPT_ | _GAMEMODE_MENU_ | _GAMEMODE_LCDINIT_;
	}
}


//=============================================================================
//  �^�C�g�����
//=============================================================================
void pvns_Title()
{
	static short cursor = 0;
	static BOOL keyRelease = FALSE;
	
	if(pcePadGet() & (TRG_LF | TRG_RI)) {
		cursor = (cursor + 1) % 2;
		LCDUpdate = TRUE;
	}
	if(!(pcePadGet() & PAD_A)) { keyRelease = TRUE; }
	if(keyRelease && pcePadGet() & TRG_A) {	// A�{�^���ŃW�����v
		keyRelease = FALSE;
		switch(cursor) {
			case 0:
				gameMode = _GAMEMODE_SCRIPT_;
				break;
			case 1:
				gameMode |= _GAMEMODE_LOAD_ | _GAMEMODE_LCDINIT_;
				break;
		}
	}
	
	if(LCDUpdate) {
		textPtr = 0;
		pvns_DrawGameGraphic();
		pvns_DrawWindow(16, 68, 96, 16);
		_SetFontColor;
		pceFontSetPos(19, 71);
		pceFontPutStr("�ŏ�����");
		if(!pvns_IsSaveFileExist()) {
			if(sData.win.color) { pceFontSetTxColor(1); }
			else				{ pceFontSetTxColor(2); }
		}
		pceFontSetPos(74, 71);
		pceFontPutStr("���[�h");
		hanToumei(5, cursor*50+19, 71, 40, 10);
	}
	
	if(pcePadGet() & TRG_B) {
		keyRelease = FALSE;
		textPtr = 0;
		gameMode |= _GAMEMODE_WINOFF_;
		pvns_DrawGameGraphic();
	}
	if(pcePadGet() & (TRG_C | TRG_D)) {
		gameMode |= _GAMEMODE_MENU_ | _GAMEMODE_LCDINIT_;
	}
}


//=============================================================================
//  ���j���[���
//=============================================================================
void pvns_Menu()
{
	static short cursor = 0;
	int bright = pceLCDSetBright(INVALIDVAL);		// �R���g���X�g
	int volume = pceWaveSetMasterAtt(INVALIDVAL);	// ����

	if(pcePadGet() & TRG_UP) {	// ��
		cursor--;
		if(cursor < 0) { cursor = 7; }
		LCDUpdate = TRUE;
	}
	if(pcePadGet() & TRG_DN) {	// ��
		cursor++;
		if(cursor > 7) { cursor = 0; }
		LCDUpdate = TRUE;
	}
	if(pcePadGet() & TRG_LF) {	// ��
		switch(cursor) {
			case 0:	// �R���g���X�g
				bright -= 2;
				if(bright < 0) { bright = 0; }
				pceLCDSetBright(bright);
				break;
			case 1:	// ����
				volume += 4;
				if(volume > 127) { volume = 127; }
				pceWaveSetMasterAtt(volume);
				break;
			case 2:	// �X�^���o�C
				demo = !demo;
				break;
		}
		LCDUpdate = TRUE;
	}
	if(pcePadGet() & TRG_RI) {	// �E
		switch( cursor ) {
			case 0:	// �R���g���X�g
				bright += 2;
				if(bright > 63) { bright = 63; }
				pceLCDSetBright(bright);
				break;
			case 1:	// ����
				volume -= 4;
				if(volume < 0) { volume = 0; }
				pceWaveSetMasterAtt(volume);
				break;
			case 2:	// �X�^���o�C
				demo = !demo;
				break;
		}
		LCDUpdate = TRUE;
	}
	if(pcePadGet() & TRG_A && !(gameMode & _GAMEMODE_LCDINIT_)) {
		switch(cursor) {
			case 0:	// �R���g���X�g
			case 1:	// ����
			case 2:	// �X�^���o�C
				textPtr = 0;
				gameMode ^= _GAMEMODE_MENU_ | _GAMEMODE_LCDINIT_;
				break;
			case 3:	// ���[�h
				if(pvns_IsSaveFileExist()) {
					gameMode |= _GAMEMODE_LOAD_ | _GAMEMODE_LCDINIT_;
				}
				break;
			case 4:	// �Z�[�u
				if(pvns_IsSaveFileExist() && saveAllow) {
					gameMode |= _GAMEMODE_SAVE_ | _GAMEMODE_LCDINIT_;
				}
				break;
			case 5:	// �ŏ��ɖ߂�
				pvns_MemoryClear();
				pvns_VariableClear();
				pvns_SaveFileClear();
				gameMode = _GAMEMODE_SCRIPT_;
				break;
			case 6:	// �����`���ɖ߂�
				pvns_MemoryClear();
				pceHeapFree(pData);			// pvn������
				pData = NULL;
				gameMode = _GAMEMODE_LOGO_ | _GAMEMODE_LCDINIT_;
				break;
			case 7:	// �I��
				pceAppReqExit(0);
				break;
		}
	}
	if(pcePadGet() & (TRG_B | TRG_C | TRG_D) && !(gameMode & _GAMEMODE_LCDINIT_)) {
		textPtr = 0;
		gameMode ^= _GAMEMODE_MENU_ | _GAMEMODE_LCDINIT_;
	}
	LCDUpdate = TRUE;
	if(LCDUpdate) {	// ��ʕ`��
		pceLCDPaint(0, 0, 0, 128, 88);
		pvns_DrawTitleBar(0);
		pceFontSetType(0);
		pceFontSetTxColor(3);
		pceFontSetBkColor(FC_SPRITE);
		pceFontSetPos(0, 8);
		pceFontPrintf("�R���g���X�g\n����\n�X�^���o�C\n");
		pceLCDPaint(3, 62, 9, 65, 8);
		pceLCDPaint(3, 62, 19, 65, 8);
		if(!pvns_IsSaveFileExist()) { pceFontSetTxColor(1); }
		pceFontPrintf("���[�h\n");
		if(!pvns_IsSaveFileExist() || !saveAllow) { pceFontSetTxColor(1); }
		pceFontPrintf("�Z�[�u\n");
		pceFontSetTxColor(3);
		pceFontPrintf("�ŏ��ɖ߂�\n�����`���ɖ߂�\n�I��");
		pceFontSetPos(62, 28);
		pceFontPutStr("<<    >>");
		if(demo) {
			pceFontSetPos(75, 28);
			pceFontPutStr("OFF");
		} else {
			pceFontSetPos(77, 28);
			pceFontPutStr("ON");
		}
		hanToumei(5, 0, cursor*10+8, 128, 10);
		pceLCDPaint(1, 63, 10, 63, 6);
		pceLCDPaint(2, 63, 10, bright, 6);				// �R���g���X�g
		pceLCDPaint(1, 63, 20, 63, 6);
		pceLCDPaint(2, 63, 20, (127 - volume) / 2, 6);	// ����
	}
}


//=============================================================================
//  ���[�h�E�Z�[�u�I�����
//=============================================================================
void pvns_LoadSave(void)
{
	static short cursor = 0;
	short i;
	PCETIME pTime;
	char msg[17];
	
	if(pcePadGet() & TRG_UP) {
		cursor--;
		if(cursor < 0) { cursor = 2; }
		LCDUpdate = TRUE;
	}
	if(pcePadGet() & TRG_DN) {
		cursor++;
		if(cursor > 2) { cursor = 0; }
		LCDUpdate = TRUE;
	}
	if(pcePadGet() & TRG_A && !(gameMode & _GAMEMODE_LCDINIT_)) {
		if(gameMode & _GAMEMODE_LOAD_) {	// ���[�h
			pvns_SaveFileInfo(cursor, &pTime, msg);
			if(pTime.mm) {
				pvns_MemoryClear();
				pvns_VariableClear();
				pvns_SaveFileLoad(cursor, &sData);
				pvns_GlobalLoad(&sData);
				for(i = 0; i <= _PGD_POS_BG_; i++) {	// �摜
					if(*sData.pgdName[i]) {
						pgd[i] = pvns_LoadResourceFile(sData.pgdName[i], _TYPE_PGD_);
					}
				}
				if(*sData.pmdName) {		// BGM
					pmd = pvns_LoadResourceFile(sData.pmdName, _TYPE_PMD_);
					PlayMusic(pmd);
				}
				if(!strncmp(pData + sData.sp.ptr, "sel", 3)) {
					gameMode = _GAMEMODE_SCRIPT_;
				} else {
					gameMode = _GAMEMODE_KEYWAIT_ | _GAMEMODE_LCDINIT_;
				}
			}
		} else {							// �Z�[�u
			pvns_SaveFileSave(cursor, &sData);
			LCDUpdate = TRUE;
		}
	}
	if(pcePadGet() & (TRG_B | TRG_C | TRG_D) && !(gameMode & _GAMEMODE_LCDINIT_)) {
		if(gameMode & _GAMEMODE_LOAD_) {
			gameMode ^= _GAMEMODE_LOAD_ | _GAMEMODE_LCDINIT_;
		} else {
			gameMode ^= _GAMEMODE_SAVE_ | _GAMEMODE_LCDINIT_;
		}
	}
	
	if(LCDUpdate) {
		pvns_DrawWindow(9, 11, 112, 66);
		_SetFontColor;
		for(i = 0; i < 3; i++) {
			pvns_SaveFileInfo(i, &pTime, msg);
			pceFontSetPos(18, 14+i*20);
			pceFontPrintf("%04d/%02d/%02d %02d:%02d:%02d", pTime.yy, pTime.mm, pTime.dd, pTime.hh, pTime.mi, pTime.ss);
			pceFontSetPos(18, 24+i*20);
			pceFontPrintf("%s...", msg);
		}
		hanToumei(5, 12, cursor*20+14, 106, 20);
	}
}


//=============================================================================
//  �A�v���P�[�V�����̏�����
//=============================================================================
void pceAppInit(void)
{
	PCETIME pTime;	// �����������p
	
	pceCPUSetSpeed(CPU_SPEED_NORMAL);
	pceAppSetProcPeriod(10);
	App_Init();					// ����������
	loadInst();					// �h�������������L�b�g�̏�����
	InitMusic();				// ���y���C�u�����̏�����
	sfont_mmckn_VersionCheck();	// MMC�J�[�l���Ȃ�t�H���g�A�h���X�ύX
	pceTimeGet(&pTime);
	srand(pTime.s100);	// ������������
	sFontStatus.xMax = 124;

#ifdef USE_MMC
	mmcInit(MMC_MAXFILESIZE * 1024);
#endif
	
	gameMode = _GAMEMODE_LOGO_;
}


//=============================================================================
//  ���C���v���Z�X
//=============================================================================
void pceAppProc(int cnt)
{
	if(demo) { pceAppActiveResponse(AAR_ACTIVE); }	// �f�����[�h

	if(gameMode & _GAMEMODE_LCDINIT_) {
		LCDUpdate = TRUE;
		gameMode ^= _GAMEMODE_LCDINIT_;
	}
	
	if(gameMode & _GAMEMODE_WINOFF_) {
		if(pcePadGet() & (TRG_A | TRG_B | TRG_C | TRG_D)) {
			textPtr = 0;
			gameMode ^= _GAMEMODE_WINOFF_ | _GAMEMODE_LCDINIT_;
		}
	}
	if(gameMode & (_GAMEMODE_LOAD_ | _GAMEMODE_SAVE_)) {
		pvns_LoadSave();
	} else {
		if(gameMode & _GAMEMODE_MENU_) { pvns_Menu(); }
	}
	if(gameMode & _GAMEMODE_ERROR_) {
		if(pcePadGet() & (TRG_A | TRG_C)) {
			if(sData.debug && gameMode & _GAMEMODE_SCRIPT_) {
				gameMode ^= _GAMEMODE_ERROR_;
			} else {
				pceAppReqExit(0);
			}
		}
	}

	switch(gameMode) {
		case _GAMEMODE_LOGO_:
			pvns_DrawLogo();
			break;
		case _GAMEMODE_LAUNCH_ | _GAMEMODE_PREPARE_:
			pvns_PrepareLaunch();
			break;
		case _GAMEMODE_LAUNCH_:
			pvns_Launch();
			break;
		case _GAMEMODE_SCRIPT_ | _GAMEMODE_PREPARE_:
			pvns_PrepareScript();
			break;
		case _GAMEMODE_SCRIPT_:
			while(pvns_AnalyzeScript());
			if(!(gameMode & _GAMEMODE_ERROR_)) {	//��AND�Ŏ��Ȃ��Ƃ��������̃G���[�\���������ɂȂ�܂�
				/* ��ʕ`�� */
				pvns_DrawGameGraphic();
			}
			break;
		case _GAMEMODE_KEYWAIT_:
			pvns_KeyWait();
			break;
		case _GAMEMODE_SELWAIT_:
			pvns_SelWait();
			break;
		case _GAMEMODE_WAIT_:
			waitTime--;
			if(waitTime <= 0 || (waitSkip && pcePadGet() & (TRG_A | PAD_RI))) {
				gameMode = _GAMEMODE_SCRIPT_;
			}
			break;
		case _GAMEMODE_TITLE_:
			pvns_Title();
			break;
	}
	
	if(LCDUpdate) {	// ��ʕ`��
		LCDUpdate = FALSE;
		pceLCDTrans();
	}
}


//=============================================================================
//  �A�v���P�[�V�����̏I������
//=============================================================================
void pceAppExit(void)
{
	StopMusic();	// ���y�̒�~
	
#ifdef USE_MMC
	mmcExit();
#endif
	
	App_Exit();		// �I������
}

