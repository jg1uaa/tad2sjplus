/*
	common.h

	original code: SASANO Takayoshi
	--- public domain, no warranty.
*/

#include <btron/btron.h>
#include <btron/hmi.h>
#include <errcode.h>
#include <bstdio.h>
#include <bstdlib.h>
#include <bstring.h>
#include <tstring.h>
#include <tcode.h>

#define	P(x)	printf x

#ifdef	DEBUG
#define	DP(x)	printf x
#else
#define	DP(x)	/* */
#endif

typedef struct _fname {
	TC	tc[L_FNM + 1];
} FNAME;

#define	OPTION_VOBJNAME	(1 << 0)	// $B<B?HL>E83+(B
#define	OPTION_HANKAKU	(1 << 1)	// $B<B?HL>$rA43Q$K8GDj(B
#define	OPTION_ZENKAKU	(1 << 2)	// $B<B?HL>$rH>3Q$K8GDj(B

#define	OPTION_TAG_HTML	(1 << 24)	// $BFC<l!'(BHTML$B%?%0$rIU2C(B
#define	OPTION_TAG_WIKI	(1 << 25)	// $BFC<l!'(BWiki$B%?%0$rIU2C(B

/* loader.c */
IMPORT	TC		*TADdata;	// TAD$B%G!<%?3JG<NN0h(B
IMPORT	W		TADsize;	// TAD$B%G!<%?%5%$%:(B
IMPORT	FNAME		*OBJname;	// $B<B?HL>3JG<NN0h(B
IMPORT	W		OBJentry;	// $B2>?H?t(B

IMPORT	ERR	load_file(TC *filename);

/* parser.c */
IMPORT	W	parse(W fd, UH *buf, W size, UW attr);

/* jis2sjis.c */
IMPORT	W	zen2han(UH jis, UB *buf);
IMPORT	UH	_mbcjistojms(UH jis);

