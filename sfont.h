
typedef struct SFONT {
	int x;							//�\���ʒu�����W
	int y;							//�\���ʒu�c���W
	int xMin;						//�\���͈͉����ŏ��l
	int xMax;						//�\���͈͉����ő�l
	int yMin;						//�\���͈͏c���ŏ��l
	int yMax;						//�\���͈͏c���ő�l
	int spr;						//�����F/�w�i�F/����F/����
									/*
									0:��    /��    /�Ȃ�  /�Ȃ�
									1:��    /��    /�Ȃ�  /����
									2:��    /��    /�Ȃ�  /�Ȃ�
									3:��    /��    /�Ȃ�  /����
									5:��    /��    /��    /����
									7:��    /��    /��    /����
									*/
} SFONT;

extern SFONT sFontStatus;

////////////////////////////////////////////////////////////////////
//	MMC�J�[�l���֘A�iMMC�J�[�l���ŕ�����������̂�h���j
//	2005/04/09	�d��Chu
void sfont_mmckn_VersionCheck(void);

const unsigned char *sFontGetAdrs( unsigned short code );
unsigned short sFontPut( int x, int y, unsigned short code );
char *sFontPutStr( const char *p );
int sFontPrintf(const char* fmt, ...);
