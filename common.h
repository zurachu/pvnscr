#include <piece.h>
#include <stdlib.h>
#include <string.h>

// MMCからファイルを開く場合コメント解除
// #define USE_MMC

#ifdef USE_MMC
	#include "mmc_api.h"
#endif

#define _PVNSCR_HEADER_		"PVSC"
#define _PVNS2_HEADER_		"PVN2"
#define _PVNS1_HEADER_		"PVNS"
#define _PBMP_HEADER_		"PMBP"
#define _PPCM_HEADER_		"MCPP"
#define _SAVE_HEADER_		"PVSS"
#define _HEADER_LENGTH_		4

#define _TYPE_PGD_			1
#define _TYPE_PPD_			2
#define _TYPE_PMD_			3

#define _VER_PVNSCR_		3
#define _VER_PVNS2_			2
#define _VER_PVNS1_			1

#define _GAMEMODE_LOGO_		0x0000
#define _GAMEMODE_LAUNCH_	0x0001
#define _GAMEMODE_SCRIPT_	0x0002
#define _GAMEMODE_KEYWAIT_	0x0004
#define _GAMEMODE_SELWAIT_	0x0008
#define _GAMEMODE_WAIT_		0x0010

#define _GAMEMODE_MENU_		0x0040
#define _GAMEMODE_PREPARE_	0x0080
#define _GAMEMODE_TITLE_	0x0100
#define _GAMEMODE_ERROR_	0x0200
#define _GAMEMODE_LCDINIT_	0x0400
#define _GAMEMODE_WINOFF_	0x0800
#define _GAMEMODE_LOAD_		0x1000
#define _GAMEMODE_SAVE_		0x2000

#define _SIZEOF_STACK_		8
#define _SIZEOF_LABEL_		128
#define _SIZEOF_JUMP_		8
#define _SIZEOF_LOCAL_		128
#define _SIZEOF_GLOBAL_		16

#define _STRING_KANJI_		1
#define _STRING_FILENAME_	2

#define _TITLE_				"P/ECE VISUALNOVEL SCRIPTER"
#define _WAIT_MESSAGE_		"しばらくお待ち下さい…"
#define _LEGACY_VERSION_	(ver<=_VER_PVNS2_)

#define _WIN_COLOR_			(sData.win.color*3)
#define _FONT_COLOR_		(3-_WIN_COLOR_)
#define _SetFontColor		pceFontSetTxColor(_FONT_COLOR_); \
							pceFontSetBkColor(FC_SPRITE); \
							sFontStatus.spr = 5 + sData.win.color * 2
#define _FONT_WIDTH_		(5-sData.win.font*1)
#define _TEXT_LENGTH_		(24+sData.win.font*6)
#define _TEXT_MAX_			(_TEXT_LENGTH_*sData.win.line)
#define _FillMsgWindow		while( strlen(sData.msg) < _TEXT_MAX_ ) \
							strcat( sData.msg, "　" )
#define _NOW_WORD_			*(pData+sData.sp.ptr)
#define _SetWait(X,Y)		do { \
								if(!(gameMode & _GAMEMODE_WAIT_)) { \
									waitTime = X; \
									waitSkip = Y; \
									gameMode = _GAMEMODE_WAIT_; \
								} \
							} while(0)
#define _BG_GRAPHIC_		-1
#define _PGD_POS_SP_		3
#define _PGD_POS_BG_		6

typedef struct tagSCRIPT_POINTER {	// スクリプトポインタ
	unsigned long ptr;	// ポインタ
	unsigned long line;	// 行
} SCRIPT_POINTER;

typedef struct tagWINDOW_STATE {	// ウィンドウの状態
	short pos;		// 位置	0:上から	1:下から
	short line;		// 行数	1～8
	short font;		// 文字	0:標準	1:縮小
	short color;	// 色	0:白	1:黒
} WINDOW_STATE;

typedef struct tagSAVE_DATA {
	SCRIPT_POINTER sp;			// ロード時のスクリプト実行開始ポインタ
	SCRIPT_POINTER stack[_SIZEOF_STACK_];	// gosub用スタック
	char msg[241];				// 表示中のメッセージ
	short msgSpeed;				// メッセージスピード（Ver3.1追加！）
	char clickStr[17];			// 自動キー待ち文字群
	short clickLine;			// clickstr時の改ページ行数
	short variable[_SIZEOF_LOCAL_+_SIZEOF_GLOBAL_];		// ローカル変数＋グローバル変数
	WINDOW_STATE win;			// ウィンドウの状態
	short bgColor;				// 背景色
	char pgdName[7][17];		// 画像ファイル名（左、中、右、Sp1～3、背景）
	short sprV[3];				// スプライト表示フラグ
	short sprX[3];				// スプライトX座標
	short sprY[3];				// スプライトY座標
	short zOrder[3];			// 画像表示の奥行き順（奥から）
	char pmdName[17];			// BGMファイル名
	BOOL debug;					// デバッグモードか否か
} SAVE_DATA;

#define _SAVE_DATA_SIZE_	1280

