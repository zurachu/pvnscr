/****************************************/
/*                                      */
/*      P/ECE VisualNovel Scripter      */
/*                                      */
/* (c)2004-2005 ヅラChu＠てとら★ぽっと */
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

unsigned char *pData;	// スクリプトデータ
short ver;				// スクリプトバージョン

SAVE_DATA sData;

unsigned char *pgd[7];	// 画像ファイルデータ
unsigned char *pmd;		// BGMファイルデータ
unsigned char *ppd[3];	// SEファイルデータ
PCEWAVEINFO   pWav[3];	// SE再生用（グローバルで宣言しないとエラーが起きる）

SCRIPT_POINTER label[_SIZEOF_LABEL_];	// ラベルポインタ
SCRIPT_POINTER jump[_SIZEOF_JUMP_];		// sel文での飛び先等
short textPtr;							// テキスト表示ポインタ

BOOL saveAllow;			// セーブ許可
BOOL lineFeedKeyWait;	// 改行によるキー待ちページ送り（PVNS2以前）
BOOL clickStrFlag;		// 自動キー待ち文字群によるキー待ち
BOOL carriageReturn;	// キャリッジリターンの可否

short waitTime;			// ウェイト時間（PVNS2以前=100ms、PVNSCR=10ms）
BOOL waitSkip;			// クリックによるウェイト飛ばしの可否

extern unsigned char ARROW[];
unsigned char arrow[7*8];

/***2バイト文字判定関数***/
BOOL IsKanji(unsigned char cData)
{
	if(cData < 0x81) { return FALSE; }
	if(cData < 0xa0) { return TRUE;  }
	if(cData < 0xe0) { return FALSE; }
	if(cData < 0xff) { return TRUE;  }
	return FALSE;
}


//=============================================================================
//  メモリ初期化
//=============================================================================
void pvns_MemoryClear(void)
{
	short i;
	
	for(i = 0; i < 7; i++) {	// pgd初期化
		pceHeapFree(pgd[i]);
		pgd[i] = NULL;
	}
	for(i = 1; i <= 3; i++) {	// ppd初期化
		pceWaveAbort(i);
		pceHeapFree(ppd[i]);
		ppd[i] = NULL;
	}
	StopMusic();				// pmd初期化
	pceHeapFree(pmd);
	pmd = NULL;
}


//=============================================================================
//  変数初期化
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
//  キー待ち矢印描画
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
//  タイトルバー描画
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
//  枠付きウィンドウ描画
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
//  メッセージダイアログ表示
//=============================================================================
void pvns_DrawMessageDialog(char* str)
{
	pvns_DrawWindow(5, 28, 120, 32);	// ウィンドウを描画
	
	sFontStatus.x = 64 - strlen(str) * 2;
	sFontStatus.y = 39;
	_SetFontColor;
	sFontPutStr(str);	// 縮小フォントで表示
/*
pceFontSetPos(64 - strlen(str) * 3, 39);
pceFontPutStr(str);
*/
	LCDUpdate =TRUE;
}


//=============================================================================
//  指定範囲内ノーマル文字描画
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
//  ゲーム画面グラフィック描画
//=============================================================================
void pvns_DrawGameGraphic(void)
{
	PIECE_BMP pBmp;		// ビットマップ構造体
	short dx, dy, i;
	
	if(!textPtr) {
		// 背景
		if(sData.bgColor == _BG_GRAPHIC_) {	// 画像指定の場合
			pceLCDPaint(0,0,0,128,88);
			Get_PieceBmp(&pBmp, pgd[_PGD_POS_BG_]);	// ビットマップ取得
			for(dy = 0; dy < 88; dy += pBmp.header.h) {	// 一面に並べて描画
				for(dx = 0; dx < 128; dx += pBmp.header.w) {
					Draw_Object(&pBmp, dx, dy, 0, 0, pBmp.header.w, pBmp.header.h, DRW_NOMAL);
				}
			}
		} else {							// 色指定の場合
			pceLCDPaint(sData.bgColor, 0, 0, 128, 88);	// 背景色で塗りつぶし
		}
		// 立ち絵
		for(i = 0; i < _PGD_POS_SP_; i++) {	// zOrderの重ね順に描画
			if(*sData.pgdName[sData.zOrder[i]]) {
				Get_PieceBmp(&pBmp, pgd[sData.zOrder[i]]);	// ビットマップ取得
				dx = 0;
				switch(sData.zOrder[i]) {	// 位置に応じてx座標を指定
					case 0:		// 左
						dx = 0;
						break;
					case 1:		// 中央
						dx = (128 - pBmp.header.w) / 2;
						break;
					case 2:		// 右
						dx = 128 - pBmp.header.w;
						break;
				}
				Draw_Object(&pBmp, dx, 88 - pBmp.header.h, 0, 0, pBmp.header.w, pBmp.header.h, DRW_NOMAL);
			}
		}
		// スプライト
		for(i = 0; i < 3; i++) {
			if(*sData.pgdName[_PGD_POS_SP_ + i] && sData.sprV[i]) {
				Get_PieceBmp(&pBmp, pgd[_PGD_POS_SP_ + i]);	// ビットマップ取得
				Draw_Object(&pBmp, sData.sprX[i], sData.sprY[i], 0, 0, pBmp.header.w, pBmp.header.h, DRW_NOMAL);
			}
		}
		
		if(strlen(sData.msg) && !(gameMode & _GAMEMODE_WINOFF_)) {
			// 半透明ウィンドウ
			if(sData.win.color == 0) { i = 2; }
			else					 { i = 4; }
			if(sData.win.line == 8) {	// 全画面
				hanToumeiAll(i);
			} else {
				if(sData.win.pos) {	// ウィンドウ位置下
					hanToumei(i, 0, 86 - sData.win.line * 10, 128, sData.win.line * 10 + 2);
				} else {			// ウィンドウ位置上
					hanToumei(i, 0, 0, 128, sData.win.line * 10 + 2);
				}
			}
		}
	}
	
	// 文字差分描画
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
	
/*	//バグ検出用
	pceFontSetPos(0, 60);
	pceFontPrintf("%s,%d\n%s,%d", sData.pgdName[0], pgd[0], sData.pgdName[2], pgd[2]);
*/	
	LCDUpdate = TRUE;
}


//=============================================================================
//  SEの再生
//=============================================================================
BOOL pvns_PlaySound(const char* fName, short ch)
{
	if(ch < 1 || ch > 3) { return FALSE; }	// 不正なチャネルはエラー
	
	pceWaveAbort(ch);	// 該当チャネルのWave出力を中止
	pceHeapFree(ppd[ch]);
	ppd[ch] = NULL;
	if((ppd[ch] = pvns_LoadResourceFile(fName, _TYPE_PPD_)) == NULL) {
		return FALSE;
	}
	Get_PieceWave(&pWav[ch], ppd[ch]);
	pceWaveDataOut(ch, &pWav[ch]);	// 該当チャネルにWave出力
	return TRUE;
}


//=============================================================================
//  エラーメッセージ表示
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
//  タイトルバーロゴ表示
//=============================================================================
void pvns_DrawLogo(void)
{
	static short y = 88;
	
	y--;
	if(pcePadGet() & (TRG_A | TRG_C) && !(gameMode & _GAMEMODE_LCDINIT_)) { y = 0; }	// AかSTARTで省略
	if(pcePadGet() & PAD_D) { pceAppReqExit(0); }	// SELECTが押された→終了
	pceLCDPaint(0, 0, 0, 128, 88);
	pvns_DrawTitleBar(y);
	LCDUpdate = TRUE;
	if(y <= 0) {
		y = 88;					// 再度呼び出す時のためにバー位置初期化
		demo = FALSE;			// 非デモモード
		sData.win.color = 1;	// ウィンドウ黒
		pvns_DrawMessageDialog(_WAIT_MESSAGE_);
		gameMode = _GAMEMODE_LAUNCH_ | _GAMEMODE_PREPARE_;
	}
}


//=============================================================================
//  シナリオランチャ準備
//=============================================================================
void pvns_PrepareLaunch(void)
{
	/* 注意：sData.variable[]をスクリプトファイル番号列取得用に使い回してます */
	if(pvns_LaunchGetFileArray(sData.variable)) {
		gameMode ^= _GAMEMODE_PREPARE_ | _GAMEMODE_LCDINIT_;
	} else {
		pvns_DrawMessageDialog("スクリプトがありません");
		gameMode = _GAMEMODE_ERROR_;
	}
}


//=============================================================================
//  シナリオランチャ
//=============================================================================
void pvns_Launch(void)
{
	static short index = 0, cursor = 0;
	short        fAmount, i;
	FILEINFO     pfi;

	for(fAmount = 0; sData.variable[fAmount] != -1; fAmount++);	// ファイル数取得

	if(pcePadGet() & TRG_UP) {	// ↑が押された
		LCDUpdate = TRUE;
		if(cursor == 0) {		// カーソルが最上段のとき
			if(fAmount <= 8) {	// ファイル数8以下（スクロールの必要なし）
				cursor = fAmount - 1;
			} else {				// ファイル数9以上（要スクロール）
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

	if(pcePadGet() & TRG_DN) {	// ↓が押された
		LCDUpdate = TRUE;
		cursor++;
		if(cursor >= fAmount) {	// ファイル数8以下（スクロール不要）
			cursor = 0;
		}
		if(cursor >= 8) {			// 要スクロール＆カーソルが最下段のとき
			cursor = 7;
			index++;
			if(index > fAmount - 8) {
				cursor = 0;
				index = 0;
			}
		}
	}
	
	/* AまたはSTARTが押された（LOGOからAまたはSTARTで飛んできた場合を回避） */
	if(!(gameMode & _GAMEMODE_LCDINIT_) && pcePadGet() & (TRG_A | TRG_C)) {
		pfi = pvns_LaunchGetFileInfo(sData.variable[index+cursor]);	// ファイル名取得
		/* 注意：sData.msgをファイル名取得用に使い回してます */
		strcpy(sData.msg, pfi.filename);
		gameMode = _GAMEMODE_SCRIPT_ | _GAMEMODE_PREPARE_;
		LCDUpdate = TRUE;
	}
	
	if(pcePadGet() & PAD_D) { pceAppReqExit(0); }	// SELECTが押された→終了
	
	if(LCDUpdate) {
		pceLCDPaint(0, 0, 8, 128, 80);
		pceFontSetType(0);
		pceFontSetBkColor(FC_SPRITE);
		pceFontSetTxColor(3);
		pceFontSetPos(0, 8);
		for(i = index; i < index+8 && i < fAmount; i++) {
			pfi = pvns_LaunchGetFileInfo(sData.variable[i]);	// ファイル情報取得
			pceFontPrintf(" %-15s%7ld\n", pfi.filename, pfi.length);	// 画面に描画
		}
		hanToumei(7, 0, cursor*10+8, 128, 10);	// 選択項目を反転
		if(gameMode & _GAMEMODE_PREPARE_) { pvns_DrawMessageDialog(_WAIT_MESSAGE_); }
	}
}

//=============================================================================
//  スクリプト準備（正規化等）
//=============================================================================
void pvns_PrepareScript(void)
{
	short i = 0;
	
	pData = pvns_LoadScriptFile(sData.msg);	// ファイル読み込み
	ver = pvns_CheckScriptHeader(pData);	// スクリプトバージョン取得
	memset(label, 0, sizeof(SCRIPT_POINTER)*_SIZEOF_LABEL_);	// ラベル記憶クリア
	sData.sp.line = 1;
	
	for(sData.sp.ptr = _HEADER_LENGTH_; _NOW_WORD_ != '\0'; sData.sp.ptr++) {
		if(_NOW_WORD_ == '\t') {	// タブを半角空白に
			_NOW_WORD_ = ' ';
			continue;
		}
		if(_NOW_WORD_ == ';') {	// ';'～行末（コメント）を半角空白に
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
			if(*(pData+sData.sp.ptr+1) == '\n') {	// CR+LFを半角空白+LFに
				_NOW_WORD_ = ' ';
				sData.sp.ptr++;
			} else {
				_NOW_WORD_ = '\n';			// CRをLFに
			}
		}
		if(_NOW_WORD_ == '\n') {	// LFは行数カウント
			sData.sp.line++;
			/* LF+'*'（ラベル）は位置を128個まで記憶（gosub,gotoの高速化のため） */
			if(*(pData+sData.sp.ptr+1) == '*') {
				sData.sp.ptr++;	// '*'の位置を記憶
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
	
	/* 各種変数初期化 */
	pvns_VariableClear();
	pvns_SaveFileClear();
	gameMode ^= _GAMEMODE_PREPARE_;
}


/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
// 命令解析は↓↓↓（行数長いので分けます）
#include "command.c"
/*****************************************************************************/
/*****************************************************************************/
/*****************************************************************************/
//=============================================================================
//  スクリプト解析（TRUEを返す間動き続ける）
//=============================================================================
BOOL pvns_AnalyzeScript(void)
{
	static char str[6];
	short i;
	
	// メッセージ＋変数が1画面に収まらなければキー待ちページ送り
	if(strlen(sData.msg) + strlen(str)*2 >= _TEXT_MAX_) {
		_FillMsgWindow;
		gameMode = _GAMEMODE_KEYWAIT_ | _GAMEMODE_LCDINIT_;
		return FALSE;
	}
	// 変数バッファに符号または数字が残っていれば先に表示
	if(*str) {
		if(isdigit(str[0])) {
			strcat(sData.msg, "０");
			sData.msg[strlen(sData.msg)-1] += str[0] - '0';	// 数字
		} else {
			strcat(sData.msg, "－");				// マイナス符号
		}
		for(i = 0; i < 4; i++) { str[i] = str[i+1]; }	// 表示した文字をバッファから消す
		lineFeedKeyWait = TRUE;	// \での改行によるキー待ちページ送り回避終了したので元に戻す
		clickStrFlag = TRUE;	// _でのキー待ち回避終了したので元に戻す
		carriageReturn = FALSE;	// テキストx座標0でのキャリッジリターンを不許可
		if(sData.msgSpeed > 0) { _SetWait(sData.msgSpeed, TRUE); }	// !sによる1文字毎のウェイト
		return pcePadGet() & PAD_RI;
	}
	
	while(_NOW_WORD_ == ' ') { sData.sp.ptr++; }	// 空白は読み飛ばす
	
	switch(_NOW_WORD_) {
		case '\0':	// 終端文字
			if(sData.msg[0] != '\0') {	// メッセージがあればキー待ちページ送り
				_FillMsgWindow;
				return TRUE;
			}
			pceAppReqExit(0);	// メッセージがなければ終了
			return FALSE;
			break;
		case '\n':	// 改行
			sData.sp.ptr++;
			sData.sp.line++;
			if(lineFeedKeyWait) {
				if(_LEGACY_VERSION_) {	// PVNS2以前＝キー待ちページ送り
					_FillMsgWindow;
					return FALSE;
				} else {					// PVNSCR＝次の行頭へ（現在行頭の場合は無視）
					while(strlen(sData.msg) % _TEXT_LENGTH_) {
						strcat(sData.msg, "　");
					}
					carriageReturn = TRUE;
				}
			}
			return TRUE;
			break;
		case '*':	// ラベル
			sData.sp.ptr++;
			while(_NOW_WORD_ != ' ' && _NOW_WORD_ != '\n' && _NOW_WORD_ != '\0') {
				sData.sp.ptr++;
			}	// 空白、改行まで読み飛ばす
			return TRUE;
			break;
		case '%':	// %（ローカル変数）
		case '$':	// $（グローバル変数）
			if(pvns_GetValueOfArgument(&i)) {
				sprintf(str, "%d", i);
				return TRUE;
			}
			break;
		case '@':	// @
			if(!_LEGACY_VERSION_) {	// PVNSCR＝キー待ち
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
			if(!_LEGACY_VERSION_) {	// PVNSCR＝次文字のclickstr解除
				clickStrFlag = FALSE;
				return TRUE;
			}
			break;
		case '/':	// /
			if(!_LEGACY_VERSION_) {	// PVNSCR＝次文字まで改行無効化
				sData.sp.ptr++;
				lineFeedKeyWait = FALSE;
				return TRUE;
			}
			break;
		case '!':	// !
			if(!_LEGACY_VERSION_) {	// PVNSCR＝特殊文字
				sData.sp.ptr++;
				switch(_NOW_WORD_) {
					case 'd':	// !d	テキスト表示待ち（クリック省略可）
						sData.sp.ptr++;
						if(pvns_GetValueOfArgument(&i)) {
							_SetWait(i, TRUE);
						} else {
							/* エラー処理 */
							pvns_SetError("!d");
						}
						return FALSE;
						break;
					case 'w':	// !w	テキスト表示待ち（クリック省略不可）
						sData.sp.ptr++;
						if(pvns_GetValueOfArgument(&i)) {
							_SetWait(i, FALSE);
						} else {
							/* エラー処理 */
							pvns_SetError("!w");
						}
						return FALSE;
						break;
					case 's':	// !s	テキスト速度変更（Ver3.1実装）
						sData.sp.ptr++;
						if(pvns_GetValueOfArgument(&i)) {
							sData.msgSpeed = i;
						} else {
							/* エラー処理 */
							pvns_SetError("!s");
						}
						
						break;
				}
			}
			break;
		case '\\':	// '\'
			sData.sp.ptr++;
			if(_LEGACY_VERSION_) {	// PVNS2以前＝特殊文字
				switch(_NOW_WORD_) {
					case 'n':	// \n	メッセージ内で改行
						sData.sp.ptr++;
						if(carriageReturn && !(strlen(sData.msg) % _TEXT_LENGTH_)) {
							strcat(sData.msg, "　");
						}
						while(strlen(sData.msg) % _TEXT_LENGTH_) {
							strcat(sData.msg, "　");
						}
						carriageReturn = TRUE;
						return TRUE;
						break;
					case 'c':	// \c	メッセージを強制的に消去
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
					default:	// \	次文字まで改行無効化
						lineFeedKeyWait = FALSE;
						return TRUE;
						break;
				}
			} else {					// PVNSCR＝キー待ちページ送り
				if(sData.msg[0]) { _FillMsgWindow; }
				return TRUE;
			}
			break;
	}
	
	if(IsKanji(_NOW_WORD_)) {	// 2バイト文字（画面に表示）
		strncat(str, pData+sData.sp.ptr, 2);
		sData.sp.ptr += 2;
		strcat(sData.msg, str);
		// PVNSCR＝clickstrに含まれるならばキー待ちに入る
		if(!_LEGACY_VERSION_ && clickStrFlag && strstr(sData.clickStr, str) != NULL ) {
			str[0] = '\0';
			if(strlen(sData.msg) / _TEXT_LENGTH_ >= sData.win.line - sData.clickLine) {
				_FillMsgWindow;
			}
			gameMode = _GAMEMODE_KEYWAIT_;
			return FALSE;
		}
		str[0] = '\0';
		lineFeedKeyWait = TRUE;	// \での改行によるキー待ちページ送り回避終了したので元に戻す
		clickStrFlag = TRUE;	// _でのキー待ち回避終了したので元に戻す
		carriageReturn = FALSE;	// テキストx座標0でのキャリッジリターンを不許可
		if(sData.msgSpeed > 0) { _SetWait(sData.msgSpeed, TRUE); }	// !sによる1文字毎のウェイト
		return (pcePadGet() & PAD_RI);
	}
	
	/* 以上に該当しなければ命令解析へ */
	return(pvns_AnalyzeCommand());
}


//=============================================================================
//  メッセージ送り待ち
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
	
	if(!(gameMode & _GAMEMODE_LCDINIT_) && pcePadGet() & (TRG_A | PAD_RI)) {	// Aボタンまたは右で続きを読み込む
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
//  選択肢決定待ち
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
	if(keyRelease && pcePadGet() & TRG_A) {	// Aボタンでジャンプ
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
//  タイトル画面
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
	if(keyRelease && pcePadGet() & TRG_A) {	// Aボタンでジャンプ
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
		pceFontPutStr("最初から");
		if(!pvns_IsSaveFileExist()) {
			if(sData.win.color) { pceFontSetTxColor(1); }
			else				{ pceFontSetTxColor(2); }
		}
		pceFontSetPos(74, 71);
		pceFontPutStr("ロード");
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
//  メニュー画面
//=============================================================================
void pvns_Menu()
{
	static short cursor = 0;
	int bright = pceLCDSetBright(INVALIDVAL);		// コントラスト
	int volume = pceWaveSetMasterAtt(INVALIDVAL);	// 音量

	if(pcePadGet() & TRG_UP) {	// 上
		cursor--;
		if(cursor < 0) { cursor = 7; }
		LCDUpdate = TRUE;
	}
	if(pcePadGet() & TRG_DN) {	// 下
		cursor++;
		if(cursor > 7) { cursor = 0; }
		LCDUpdate = TRUE;
	}
	if(pcePadGet() & TRG_LF) {	// 左
		switch(cursor) {
			case 0:	// コントラスト
				bright -= 2;
				if(bright < 0) { bright = 0; }
				pceLCDSetBright(bright);
				break;
			case 1:	// 音量
				volume += 4;
				if(volume > 127) { volume = 127; }
				pceWaveSetMasterAtt(volume);
				break;
			case 2:	// スタンバイ
				demo = !demo;
				break;
		}
		LCDUpdate = TRUE;
	}
	if(pcePadGet() & TRG_RI) {	// 右
		switch( cursor ) {
			case 0:	// コントラスト
				bright += 2;
				if(bright > 63) { bright = 63; }
				pceLCDSetBright(bright);
				break;
			case 1:	// 音量
				volume -= 4;
				if(volume < 0) { volume = 0; }
				pceWaveSetMasterAtt(volume);
				break;
			case 2:	// スタンバイ
				demo = !demo;
				break;
		}
		LCDUpdate = TRUE;
	}
	if(pcePadGet() & TRG_A && !(gameMode & _GAMEMODE_LCDINIT_)) {
		switch(cursor) {
			case 0:	// コントラスト
			case 1:	// 音量
			case 2:	// スタンバイ
				textPtr = 0;
				gameMode ^= _GAMEMODE_MENU_ | _GAMEMODE_LCDINIT_;
				break;
			case 3:	// ロード
				if(pvns_IsSaveFileExist()) {
					gameMode |= _GAMEMODE_LOAD_ | _GAMEMODE_LCDINIT_;
				}
				break;
			case 4:	// セーブ
				if(pvns_IsSaveFileExist() && saveAllow) {
					gameMode |= _GAMEMODE_SAVE_ | _GAMEMODE_LCDINIT_;
				}
				break;
			case 5:	// 最初に戻る
				pvns_MemoryClear();
				pvns_VariableClear();
				pvns_SaveFileClear();
				gameMode = _GAMEMODE_SCRIPT_;
				break;
			case 6:	// ランチャに戻る
				pvns_MemoryClear();
				pceHeapFree(pData);			// pvn初期化
				pData = NULL;
				gameMode = _GAMEMODE_LOGO_ | _GAMEMODE_LCDINIT_;
				break;
			case 7:	// 終了
				pceAppReqExit(0);
				break;
		}
	}
	if(pcePadGet() & (TRG_B | TRG_C | TRG_D) && !(gameMode & _GAMEMODE_LCDINIT_)) {
		textPtr = 0;
		gameMode ^= _GAMEMODE_MENU_ | _GAMEMODE_LCDINIT_;
	}
	LCDUpdate = TRUE;
	if(LCDUpdate) {	// 画面描画
		pceLCDPaint(0, 0, 0, 128, 88);
		pvns_DrawTitleBar(0);
		pceFontSetType(0);
		pceFontSetTxColor(3);
		pceFontSetBkColor(FC_SPRITE);
		pceFontSetPos(0, 8);
		pceFontPrintf("コントラスト\n音量\nスタンバイ\n");
		pceLCDPaint(3, 62, 9, 65, 8);
		pceLCDPaint(3, 62, 19, 65, 8);
		if(!pvns_IsSaveFileExist()) { pceFontSetTxColor(1); }
		pceFontPrintf("ロード\n");
		if(!pvns_IsSaveFileExist() || !saveAllow) { pceFontSetTxColor(1); }
		pceFontPrintf("セーブ\n");
		pceFontSetTxColor(3);
		pceFontPrintf("最初に戻る\nランチャに戻る\n終了");
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
		pceLCDPaint(2, 63, 10, bright, 6);				// コントラスト
		pceLCDPaint(1, 63, 20, 63, 6);
		pceLCDPaint(2, 63, 20, (127 - volume) / 2, 6);	// 音量
	}
}


//=============================================================================
//  ロード・セーブ選択画面
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
		if(gameMode & _GAMEMODE_LOAD_) {	// ロード
			pvns_SaveFileInfo(cursor, &pTime, msg);
			if(pTime.mm) {
				pvns_MemoryClear();
				pvns_VariableClear();
				pvns_SaveFileLoad(cursor, &sData);
				pvns_GlobalLoad(&sData);
				for(i = 0; i <= _PGD_POS_BG_; i++) {	// 画像
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
		} else {							// セーブ
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
//  アプリケーションの初期化
//=============================================================================
void pceAppInit(void)
{
	PCETIME pTime;	// 乱数初期化用
	
	pceCPUSetSpeed(CPU_SPEED_NORMAL);
	pceAppSetProcPeriod(10);
	App_Init();					// 初期化処理
	loadInst();					// ドラム音源分離キットの初期化
	InitMusic();				// 音楽ライブラリの初期化
	sfont_mmckn_VersionCheck();	// MMCカーネルならフォントアドレス変更
	pceTimeGet(&pTime);
	srand(pTime.s100);	// 乱数を初期化
	sFontStatus.xMax = 124;

#ifdef USE_MMC
	mmcInit(MMC_MAXFILESIZE * 1024);
#endif
	
	gameMode = _GAMEMODE_LOGO_;
}


//=============================================================================
//  メインプロセス
//=============================================================================
void pceAppProc(int cnt)
{
	if(demo) { pceAppActiveResponse(AAR_ACTIVE); }	// デモモード

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
			if(!(gameMode & _GAMEMODE_ERROR_)) {	//☆ANDで取らないとせっかくのエラー表示が無効になります
				/* 画面描画 */
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
	
	if(LCDUpdate) {	// 画面描画
		LCDUpdate = FALSE;
		pceLCDTrans();
	}
}


//=============================================================================
//  アプリケーションの終了処理
//=============================================================================
void pceAppExit(void)
{
	StopMusic();	// 音楽の停止
	
#ifdef USE_MMC
	mmcExit();
#endif
	
	App_Exit();		// 終了処理
}

