#include "common.h"
#include "libfpk\libfpk.h"

HFPK fpk;
char saveFileName[16];

//=============================================================================
//  �t�@�C���ǂݏo���֐�
//=============================================================================
unsigned char* pvns_LoadFile( const char* fName )
{
	unsigned char *ret = NULL;	// �߂�l
	FILEINFO      pfi;			// �t�@�C�����\����
	FILEACC       pfa;			// �t�@�C���\����
	unsigned long size;
	short         i = 0;

	pceFileFindOpen( &pfi );	// �t�@�C�������̏���
	while( pceFileFindNext( &pfi ) ) {	// �t�@�C������
		if( !strcmp( fName, pfi.filename ) ) {	// �t�@�C�������������ꍇ
			size = pfi.length;
			if( strstr( fName, ".pvn" ) != NULL ) { size++; }
			if( ( ret = pceHeapAlloc( size ) ) == NULL ) { return NULL; }	// �������̊m��
			pceFileOpen( &pfa, pfi.filename, FOMD_RD );	// �t�@�C���̓ǂݏo��
			while( pceFileReadSct( &pfa, ret+i*4096, i, 4096 ) ) { i++; }
			pceFileClose( &pfa );
			if( strstr( fName, ".pvn" ) != NULL ) { *(ret+size-1) = '\0'; }
			break;
		}
	}
	pceFileFindClose( &pfi );	//�t�@�C���������I��

	return( ret );
}


//=============================================================================
//  ���\�[�X�擾�֐�
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
//  �w�b�_�ƍ����o�[�W�������o
//=============================================================================
short pvns_CheckScriptHeader( const unsigned char* script )
{
	if( !strncmp( script, _PVNSCR_HEADER_, _HEADER_LENGTH_ ) ) { return( _VER_PVNSCR_ ); }
	if( !strncmp( script, _PVNS2_HEADER_, _HEADER_LENGTH_ ) )  { return( _VER_PVNS2_ ); }
	if( !strncmp( script, _PVNS1_HEADER_, _HEADER_LENGTH_ ) )  { return( _VER_PVNS1_ ); }
	return( 0 );	// �Y�����Ȃ�
}

//=============================================================================
//  �X�N���v�g�t�@�C���ǂݍ��݌��w�b�_�ƍ�
//=============================================================================
unsigned char* pvns_LoadScriptFile( const char* fName )
{
	unsigned char *ret = NULL;
	char          scriptName[16] = "";
	
	fpkCloseArchive(fpk);
	if( strstr( fName, ".pva" ) != NULL ||\
		strstr( fName, ".par" ) != NULL ||\
		strstr( fName, ".fpk" ) != NULL ) {		// pva�܂���par�܂���fpk
		fpk = fpkOpenArchive(fName);
		strcpy( scriptName, fName );
		strcpy( strchr( scriptName, '.' ), ".pvn" );	// pvn�t�@�C�����𐶐�
	} else {
		if( strstr( fName, ".pvn" ) != NULL ) {	// pvn
			strcpy( scriptName, fName );
		} else {								// ����ȊO
			return NULL;
		}
	}
	if( ( ret = pvns_GetFileData( scriptName ) ) == NULL ) { return NULL; }
	if( !pvns_CheckScriptHeader( ret ) ) {	// �w�b�_�ƍ����s�̏ꍇ
		pceHeapFree( ret );
		return NULL;
	}
	return( ret );
}


//=============================================================================
//  �摜�ASE�ABGM�t�@�C���ǂݍ��݌��w�b�_�ƍ�
//=============================================================================
unsigned char* pvns_LoadResourceFile( const char *fName, short type )
{
	unsigned char *ret = NULL;
	char          resourceName[16] = "";
	
	strcpy( resourceName, fName );
	if( strstr( resourceName, "." ) == NULL ) {	// �g���q�ȗ��̏ꍇ�⊮
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
	switch( type ) {	// �w�b�_�ƍ����s�Ȃ�NULL��Ԃ�
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
//  �i�����`���p�j�X�N���v�g�t�@�C���ԍ���擾�֐�
//=============================================================================
short pvns_LaunchGetFileArray( short* array )
{
	short         ret = 0;	// �߂�l
	FILEINFO      pfi;		// �t�@�C�����\����
	unsigned char *tmp;		// �X�N���v�g�t�@�C���ǂݍ��ݗp
	short i = 0;

	pceFileFindOpen( &pfi );	// �t�@�C�������̏���
	while( pceFileFindNext( &pfi ) ) {	// �t�@�C��������
		if( ( tmp = pvns_LoadScriptFile( pfi.filename ) ) != NULL ) {	// �w�b�_�ƍ�
			pceHeapFree( tmp );
			array[ret] = i;
			ret++;
		}
		i++;
	}
	pceFileFindClose( &pfi );	//�t�@�C���������I��

	array[ret] = -1;
	return( ret );
}


//=============================================================================
//  �i�����`���p�j�t�@�C�����擾�֐��i���s�����ꍇ�̕ۏ؂Ȃ��j
//=============================================================================
FILEINFO pvns_LaunchGetFileInfo( int num )
{
	FILEINFO pfi;	// �߂�l
	short    i = 0;

	pceFileFindOpen( &pfi );	// �t�@�C�������̏���
	while( pceFileFindNext( &pfi ) ) {	// �t�@�C��������
		if( i == num ) { break; }
		i++;
	}
	pceFileFindClose( &pfi );	//�t�@�C���������I��

	return( pfi );
}


//=============================================================================
//  �Z�[�u�t�@�C�����`����
//=============================================================================
BOOL pvns_SaveFileDefine( const char* fName, SAVE_DATA* sd )
{
	unsigned char* sData;
	FILEACC        pfa;
	
	strcpy( saveFileName, fName );	// �Z�[�u�t�@�C�����o�^
	// ���łɃt�@�C��������ꍇ�̓O���[�o���ϐ���ǂݍ���
	if( !pceFileOpen( &pfa, saveFileName, FOMD_RD ) ) {	// �ǂݍ���
		pceFileReadSct(&pfa, NULL, 0, 0);			// �|�C���^�擾
		if( strncmp( pfa.aptr, _SAVE_HEADER_, _HEADER_LENGTH_ ) ) {
			pceFileClose(&pfa);
			return FALSE;
		}
		memcpy(&(sd->variable[_SIZEOF_LOCAL_]), pfa.aptr + _HEADER_LENGTH_, sizeof(short)*_SIZEOF_GLOBAL_);
		pceFileClose(&pfa);
	}
	else {	// �t�@�C�����Ȃ��ꍇ�͐V�K�쐬
		if( pceFileCreate( saveFileName, 4096 ) ) { return FALSE; }
		if( ( sData = pceHeapAlloc( 4096 ) ) == NULL ) { return FALSE; }
		memset( sData, 0, 4096 );
		memcpy( sData, _SAVE_HEADER_, _HEADER_LENGTH_ );	// �w�b�_
		pceFileOpen( &pfa, saveFileName, FOMD_WR );				// ��������
		pceFileWriteSct( &pfa, sData, 0, 4096 );
		pceFileClose( &pfa );
		pceHeapFree( sData );
	}
	
	return TRUE;
}


//=============================================================================
//  �Z�[�u�t�@�C���̒�`����������
//=============================================================================
void pvns_SaveFileClear( void )
{
	*saveFileName = '\0';
}


//=============================================================================
//  �Z�[�u�t�@�C���̑��݂�₤
//=============================================================================
BOOL pvns_IsSaveFileExist( void )
{
	return( *saveFileName != '\0' );
}


//=============================================================================
//  �Z�[�u�t�@�C���̏��擾�i�Z�[�u�E���[�h��ʕ\���p�j
//=============================================================================
void pvns_SaveFileInfo(short id, PCETIME* pTime, char* msg)
{
	PCETIME* tptr;
	SAVE_DATA* sptr;
	FILEACC pfa;
	
	pceFileOpen(&pfa, saveFileName, FOMD_RD);	// �ǂݍ���
	pceFileReadSct(&pfa, NULL, 0, 0);			// �|�C���^�擾
	tptr = (PCETIME*)(pfa.aptr + 256 + id * _SAVE_DATA_SIZE_);
	*pTime = *tptr;
	sptr = (SAVE_DATA*)(pfa.aptr + 256 + id * _SAVE_DATA_SIZE_ + sizeof(PCETIME));
	strncpy(msg, sptr->msg, 16);
	*(msg+16) = '\0';
	
	pceFileClose(&pfa);
}
//=============================================================================
//  �Z�[�u�t�@�C����ǂݍ��ށi�t�@�C�������݂���O��j
//=============================================================================
void pvns_SaveFileLoad(short id, SAVE_DATA* sd)
{
	SAVE_DATA* sptr;
	FILEACC pfa;
	
	pceFileOpen(&pfa, saveFileName, FOMD_RD);	// �ǂݍ���
	pceFileReadSct(&pfa, NULL, 0, 0);			// �|�C���^�擾
	sptr = (SAVE_DATA*)(pfa.aptr + 256 + id * _SAVE_DATA_SIZE_ + sizeof(PCETIME));
	*sd = *sptr;
	pceFileClose(&pfa);
}


//=============================================================================
//  �Z�[�u�t�@�C�����������ށi�t�@�C�������݂���O��j
//=============================================================================
void pvns_SaveFileSave(short id, SAVE_DATA* sd)
{
	unsigned char* sData;
	PCETIME* tptr;
	SAVE_DATA* sptr;
	FILEACC pfa;
	
	if((sData = pceHeapAlloc(4096)) != NULL) {
		pceFileOpen(&pfa, saveFileName, FOMD_RD);	// �ǂݍ���
		pceFileReadSct(&pfa, sData, 0, 4096);
		pceFileClose(&pfa);
		tptr = (PCETIME*)(sData +  256 + id * _SAVE_DATA_SIZE_);
		pceTimeGet(tptr);
		sptr = (SAVE_DATA*)(sData + 256 + id * _SAVE_DATA_SIZE_ + sizeof(PCETIME));
		*sptr = *sd;
		pceFileOpen(&pfa, saveFileName, FOMD_WR);	// ��������
		pceFileWriteSct(&pfa, sData, 0, 4096);
		pceFileClose(&pfa);
		pceHeapFree(sData);
	}
}


//=============================================================================
//  �O���[�o���ϐ��ǂݍ���
//=============================================================================
void pvns_GlobalLoad(SAVE_DATA* sd)
{
	FILEACC pfa;
	
	if(pvns_IsSaveFileExist()) {
		pceFileOpen(&pfa, saveFileName, FOMD_RD);	// �ǂݍ���
		pceFileReadSct(&pfa, NULL, 0, 0);			// �|�C���^�擾
		memcpy(&(sd->variable[_SIZEOF_LOCAL_]), pfa.aptr + _HEADER_LENGTH_, sizeof(short)*_SIZEOF_GLOBAL_);
		pceFileClose(&pfa);
	}
}

//=============================================================================
//  �O���[�o���ϐ���������
//=============================================================================
void pvns_GlobalSave(SAVE_DATA* sd)
{
	unsigned char* sData;
	FILEACC pfa;
	
	if(pvns_IsSaveFileExist()) {
		if((sData = pceHeapAlloc(4096)) != NULL) {
			pceFileOpen(&pfa, saveFileName, FOMD_RD);	// �ǂݍ���
			pceFileReadSct(&pfa, sData, 0, 4096);
			pceFileClose(&pfa);
			memcpy(sData + _HEADER_LENGTH_, &(sd->variable[_SIZEOF_LOCAL_]), sizeof(short)*_SIZEOF_GLOBAL_);
			pceFileOpen(&pfa, saveFileName, FOMD_WR);	// ��������
			pceFileWriteSct(&pfa, sData, 0, 4096);
			pceFileClose(&pfa);
			pceHeapFree(sData);
		}
	}
}
