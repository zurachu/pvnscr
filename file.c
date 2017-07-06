#include "common.h"
#include "libfpk\libfpk.h"

HFPK fpk;
char saveFileName[16];

//=============================================================================
//  ファイル読み出し関数
//=============================================================================
unsigned char* pvns_LoadFile( const char* fName )
{
	unsigned char *ret = NULL;	// 戻り値
	FILEINFO      pfi;			// ファイル情報構造体
	FILEACC       pfa;			// ファイル構造体
	unsigned long size;
	short         i = 0;

	pceFileFindOpen( &pfi );	// ファイル検索の準備
	while( pceFileFindNext( &pfi ) ) {	// ファイル検索
		if( !strcmp( fName, pfi.filename ) ) {	// ファイルが見つかった場合
			size = pfi.length;
			if( strstr( fName, ".pvn" ) != NULL ) { size++; }
			if( ( ret = pceHeapAlloc( size ) ) == NULL ) { return NULL; }	// メモリの確保
			pceFileOpen( &pfa, pfi.filename, FOMD_RD );	// ファイルの読み出し
			while( pceFileReadSct( &pfa, ret+i*4096, i, 4096 ) ) { i++; }
			pceFileClose( &pfa );
			if( strstr( fName, ".pvn" ) != NULL ) { *(ret+size-1) = '\0'; }
			break;
		}
	}
	pceFileFindClose( &pfi );	//ファイル検索を終了

	return( ret );
}


//=============================================================================
//  リソース取得関数
//=============================================================================
unsigned char* pvns_GetFileData( const char* fName )
{
	unsigned char *ret = NULL;
	FPKENTRY fpkEntry;

	if(fpkGetFileInfoS(fpk, fName, &fpkEntry)) {
		ret = fpkExtractToBuffer(fpk, &fpkEntry);
	}
	else {
		ret = pvns_LoadFile( fName );
	}

	return( ret );
}


//=============================================================================
//  ヘッダ照合＆バージョン検出
//=============================================================================
short pvns_CheckScriptHeader( const unsigned char* script )
{
	if( !strncmp( script, _PVNSCR_HEADER_, _HEADER_LENGTH_ ) ) { return( _VER_PVNSCR_ ); }
	if( !strncmp( script, _PVNS2_HEADER_, _HEADER_LENGTH_ ) )  { return( _VER_PVNS2_ ); }
	if( !strncmp( script, _PVNS1_HEADER_, _HEADER_LENGTH_ ) )  { return( _VER_PVNS1_ ); }
	return( 0 );	// 該当しない
}

//=============================================================================
//  スクリプトファイル読み込み兼ヘッダ照合
//=============================================================================
unsigned char* pvns_LoadScriptFile( const char* fName )
{
	unsigned char *ret = NULL;
	char          scriptName[16] = "";
	
	fpkCloseArchive(fpk);
	if( strstr( fName, ".pva" ) != NULL ||\
		strstr( fName, ".par" ) != NULL ||\
		strstr( fName, ".fpk" ) != NULL ) {		// pvaまたはparまたはfpk
		fpk = fpkOpenArchive(fName);
		strcpy( scriptName, fName );
		strcpy( strchr( scriptName, '.' ), ".pvn" );	// pvnファイル名を生成
	} else {
		if( strstr( fName, ".pvn" ) != NULL ) {	// pvn
			strcpy( scriptName, fName );
		} else {								// それ以外
			return NULL;
		}
	}
	if( ( ret = pvns_GetFileData( scriptName ) ) == NULL ) { return NULL; }
	if( !pvns_CheckScriptHeader( ret ) ) {	// ヘッダ照合失敗の場合
		pceHeapFree( ret );
		return NULL;
	}
	return( ret );
}


//=============================================================================
//  画像、SE、BGMファイル読み込み兼ヘッダ照合
//=============================================================================
unsigned char* pvns_LoadResourceFile( const char *fName, short type )
{
	unsigned char *ret = NULL;
	char          resourceName[16] = "";
	
	strcpy( resourceName, fName );
	if( strstr( resourceName, "." ) == NULL ) {	// 拡張子省略の場合補完
		switch( type ) {
			case _TYPE_PGD_:
				strcat( resourceName, ".pgd" );
				break;
			case _TYPE_PPD_:
				strcat( resourceName, ".ppd" );
				break;
			case _TYPE_PMD_:
				strcat( resourceName, ".pmd" );
				break;
			default:
				return NULL;
				break;
		}
	}
	if( ( ret = pvns_GetFileData( resourceName ) ) == NULL ) { return NULL; }
	switch( type ) {	// ヘッダ照合失敗ならNULLを返す
		case _TYPE_PGD_:
			if( strncmp( ret, _PBMP_HEADER_, _HEADER_LENGTH_ ) ) {
				pceHeapFree( ret );
				return NULL;
			}
			break;
		case _TYPE_PPD_:
			if( strncmp( ret, _PPCM_HEADER_, _HEADER_LENGTH_ ) ) {
				pceHeapFree( ret );
				return NULL;
			}
			break;
	}
	
	return( ret );
}


//=============================================================================
//  （ランチャ用）スクリプトファイル番号列取得関数
//=============================================================================
short pvns_LaunchGetFileArray( short* array )
{
	short         ret = 0;	// 戻り値
	FILEINFO      pfi;		// ファイル情報構造体
	unsigned char *tmp;		// スクリプトファイル読み込み用
	short i = 0;

	pceFileFindOpen( &pfi );	// ファイル検索の準備
	while( pceFileFindNext( &pfi ) ) {	// ファイルを検索
		if( ( tmp = pvns_LoadScriptFile( pfi.filename ) ) != NULL ) {	// ヘッダ照合
			pceHeapFree( tmp );
			array[ret] = i;
			ret++;
		}
		i++;
	}
	pceFileFindClose( &pfi );	//ファイル検索を終了

	array[ret] = -1;
	return( ret );
}


//=============================================================================
//  （ランチャ用）ファイル情報取得関数（失敗した場合の保証なし）
//=============================================================================
FILEINFO pvns_LaunchGetFileInfo( int num )
{
	FILEINFO pfi;	// 戻り値
	short    i = 0;

	pceFileFindOpen( &pfi );	// ファイル検索の準備
	while( pceFileFindNext( &pfi ) ) {	// ファイルを検索
		if( i == num ) { break; }
		i++;
	}
	pceFileFindClose( &pfi );	//ファイル検索を終了

	return( pfi );
}


//=============================================================================
//  セーブファイルを定義する
//=============================================================================
BOOL pvns_SaveFileDefine( const char* fName, SAVE_DATA* sd )
{
	unsigned char* sData;
	FILEACC        pfa;
	
	strcpy( saveFileName, fName );	// セーブファイル名登録
	// すでにファイルがある場合はグローバル変数を読み込み
	if( !pceFileOpen( &pfa, saveFileName, FOMD_RD ) ) {	// 読み込み
		pceFileReadSct(&pfa, NULL, 0, 0);			// ポインタ取得
		if( strncmp( pfa.aptr, _SAVE_HEADER_, _HEADER_LENGTH_ ) ) {
			pceFileClose(&pfa);
			return FALSE;
		}
		memcpy(&(sd->variable[_SIZEOF_LOCAL_]), pfa.aptr + _HEADER_LENGTH_, sizeof(short)*_SIZEOF_GLOBAL_);
		pceFileClose(&pfa);
	}
	else {	// ファイルがない場合は新規作成
		if( pceFileCreate( saveFileName, 4096 ) ) { return FALSE; }
		if( ( sData = pceHeapAlloc( 4096 ) ) == NULL ) { return FALSE; }
		memset( sData, 0, 4096 );
		memcpy( sData, _SAVE_HEADER_, _HEADER_LENGTH_ );	// ヘッダ
		pceFileOpen( &pfa, saveFileName, FOMD_WR );				// 書き込み
		pceFileWriteSct( &pfa, sData, 0, 4096 );
		pceFileClose( &pfa );
		pceHeapFree( sData );
	}
	
	return TRUE;
}


//=============================================================================
//  セーブファイルの定義を消去する
//=============================================================================
void pvns_SaveFileClear( void )
{
	*saveFileName = '\0';
}


//=============================================================================
//  セーブファイルの存在を問う
//=============================================================================
BOOL pvns_IsSaveFileExist( void )
{
	return( *saveFileName != '\0' );
}


//=============================================================================
//  セーブファイルの情報取得（セーブ・ロード画面表示用）
//=============================================================================
void pvns_SaveFileInfo(short id, PCETIME* pTime, char* msg)
{
	PCETIME* tptr;
	SAVE_DATA* sptr;
	FILEACC pfa;
	
	pceFileOpen(&pfa, saveFileName, FOMD_RD);	// 読み込み
	pceFileReadSct(&pfa, NULL, 0, 0);			// ポインタ取得
	tptr = (PCETIME*)(pfa.aptr + 256 + id * _SAVE_DATA_SIZE_);
	*pTime = *tptr;
	sptr = (SAVE_DATA*)(pfa.aptr + 256 + id * _SAVE_DATA_SIZE_ + sizeof(PCETIME));
	strncpy(msg, sptr->msg, 16);
	*(msg+16) = '\0';
	
	pceFileClose(&pfa);
}
//=============================================================================
//  セーブファイルを読み込む（ファイルが存在する前提）
//=============================================================================
void pvns_SaveFileLoad(short id, SAVE_DATA* sd)
{
	SAVE_DATA* sptr;
	FILEACC pfa;
	
	pceFileOpen(&pfa, saveFileName, FOMD_RD);	// 読み込み
	pceFileReadSct(&pfa, NULL, 0, 0);			// ポインタ取得
	sptr = (SAVE_DATA*)(pfa.aptr + 256 + id * _SAVE_DATA_SIZE_ + sizeof(PCETIME));
	*sd = *sptr;
	pceFileClose(&pfa);
}


//=============================================================================
//  セーブファイルを書き込む（ファイルが存在する前提）
//=============================================================================
void pvns_SaveFileSave(short id, SAVE_DATA* sd)
{
	unsigned char* sData;
	PCETIME* tptr;
	SAVE_DATA* sptr;
	FILEACC pfa;
	
	if((sData = pceHeapAlloc(4096)) != NULL) {
		pceFileOpen(&pfa, saveFileName, FOMD_RD);	// 読み込み
		pceFileReadSct(&pfa, sData, 0, 4096);
		pceFileClose(&pfa);
		tptr = (PCETIME*)(sData +  256 + id * _SAVE_DATA_SIZE_);
		pceTimeGet(tptr);
		sptr = (SAVE_DATA*)(sData + 256 + id * _SAVE_DATA_SIZE_ + sizeof(PCETIME));
		*sptr = *sd;
		pceFileOpen(&pfa, saveFileName, FOMD_WR);	// 書き込み
		pceFileWriteSct(&pfa, sData, 0, 4096);
		pceFileClose(&pfa);
		pceHeapFree(sData);
	}
}


//=============================================================================
//  グローバル変数読み込み
//=============================================================================
void pvns_GlobalLoad(SAVE_DATA* sd)
{
	FILEACC pfa;
	
	if(pvns_IsSaveFileExist()) {
		pceFileOpen(&pfa, saveFileName, FOMD_RD);	// 読み込み
		pceFileReadSct(&pfa, NULL, 0, 0);			// ポインタ取得
		memcpy(&(sd->variable[_SIZEOF_LOCAL_]), pfa.aptr + _HEADER_LENGTH_, sizeof(short)*_SIZEOF_GLOBAL_);
		pceFileClose(&pfa);
	}
}

//=============================================================================
//  グローバル変数書き換え
//=============================================================================
void pvns_GlobalSave(SAVE_DATA* sd)
{
	unsigned char* sData;
	FILEACC pfa;
	
	if(pvns_IsSaveFileExist()) {
		if((sData = pceHeapAlloc(4096)) != NULL) {
			pceFileOpen(&pfa, saveFileName, FOMD_RD);	// 読み込み
			pceFileReadSct(&pfa, sData, 0, 4096);
			pceFileClose(&pfa);
			memcpy(sData + _HEADER_LENGTH_, &(sd->variable[_SIZEOF_LOCAL_]), sizeof(short)*_SIZEOF_GLOBAL_);
			pceFileOpen(&pfa, saveFileName, FOMD_WR);	// 書き込み
			pceFileWriteSct(&pfa, sData, 0, 4096);
			pceFileClose(&pfa);
			pceHeapFree(sData);
		}
	}
}
