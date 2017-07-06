int unpack( char *pArcData, char *pOutBuff );
BOOL ppack_checkHeader( unsigned char* arcData );
long ppack_getExpandSize( unsigned char* arcData );
unsigned char* ppack_heapUnpack( unsigned char* arcData );
unsigned char* ppack_findPackDataEx( const char *fpkName, const char *fName );
