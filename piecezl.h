
typedef struct _zlibIO {
	unsigned char *ptr;
	unsigned char *ptre;
	unsigned char *ptr0;
	union {
		int (*fil)(struct _zlibIO *);
		void (*fls)(struct _zlibIO *);
	} fn;
} zlibIO;

#define zlgetc(_stream) (((_stream)->ptr < (_stream)->ptre) ? *((_stream)->ptr++) : (_stream)->fn.fil(_stream))


int pceZlibExpand( zlibIO *zlIn, zlibIO *zlOut, void *works );


