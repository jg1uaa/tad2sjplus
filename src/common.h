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

#define	OPTION_VOBJNAME	(1 << 0)	// 実身名展開
#define	OPTION_HANKAKU	(1 << 1)	// 実身名を全角に固定
#define	OPTION_ZENKAKU	(1 << 2)	// 実身名を半角に固定

#define	OPTION_TAG_HTML	(1 << 24)	// 特殊：HTMLタグを付加
#define	OPTION_TAG_WIKI	(1 << 25)	// 特殊：Wikiタグを付加

/* loader.c */
IMPORT	TC		*TADdata;	// TADデータ格納領域
IMPORT	W		TADsize;	// TADデータサイズ
IMPORT	FNAME		*OBJname;	// 実身名格納領域
IMPORT	W		OBJentry;	// 仮身数

IMPORT	ERR	load_file(TC *filename);

/* parser.c */
IMPORT	W	parse(W fd, UH *buf, W size, UW attr);

/* jis2sjis.c */
IMPORT	W	zen2han(UH jis, UB *buf);
IMPORT	UH	_mbcjistojms(UH jis);

