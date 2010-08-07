/*
	main.c

	original code: SASANO Takayoshi
	--- public domain, no warranty.
*/

#include "common.h"

/* convert pathname */
LOCAL	void	convert_path(TC *out, TC *in, W maxlen)
{
	W	len;

	len = tc_strlen(in);
	if (len > maxlen - 1) len = maxlen - 1;

	while (len-- > 0) {
		switch (*in) {
		case	TK_SLSH:
			*out = TC_FDLM;
			break;
		case	TK_COLN:
			*out = TC_FSEP;
			break;
		case	TK_PCNT:
			in++;
			len--;
			if (len > 0) *out = *in;
			break;
		default:
			*out = *in;
			break;
		}
		in++;
		out++;
	}

	*out = TC_NULL;

	return;
}

/* main entry */
EXPORT	W	main(W ac, TC *av[])
{
	W	err, fd, attr;
	LINK	l;
	B	dummy;
	TC	path[L_PATHNM];

	/* ヘルプの表示 */
	if (ac < 4) {
		P(("usage: %S [option] [TADfile] [SJISfile]\n", av[0]));
		P(("	option:	t	text only\n"));
		P(("		x	expand vobjname\n"));
		P(("		h	expand vobjname, hankaku\n"));
		P(("		z	expand vobjname, zenkaku\n"));
		err = ER_PAR;
		goto fin0;
	}

	/* オプションの設定 */
	attr = 0;
	switch (*av[1]) {
	case	TK_h:	attr |= OPTION_VOBJNAME | OPTION_HANKAKU; break;
	case	TK_z:	attr |= OPTION_VOBJNAME | OPTION_ZENKAKU; break;
	case	TK_x:	attr |= OPTION_VOBJNAME; break;
	default:	/* do nothing */ break;
	}

	switch (av[0][tc_strlen(av[0]) - 1]) {
	case	TK_H:	attr |= OPTION_TAG_HTML; break;
	case	TK_W:	attr |= OPTION_TAG_WIKI; break;
	default:	/* do nothing */ break;
	}

	convert_path(path, av[2], L_PATHNM);

	/* TADデータの読み出し */
	err = load_file(path);
	if (err < ER_OK) goto fin0;

	/* リンクを獲得	*/
	err = get_lnk(NULL, &l, F_NORM);
	if (err < ER_OK) {
		P(("main: get_lnk %d\n", err));
		goto fin1;
	}

	/* 出力ファイルの作成 */
	err = cre_fil(&l, av[3], NULL, 1, F_FIX);
	if (err < ER_OK) {
		P(("main: cre_fil %d\n", err));
		goto fin1;
	}
	fd = err;

	/* 空のレコードを出力 */
	err = apd_rec(fd, &dummy, 0, 31, 0, 0);
	if (err < ER_OK) {
		P(("main: apd_rec %d\n", err));
		goto fin2;
	}

	/* 終端レコードの一つ前に移動する */
	err = see_rec(fd, -1, -1, NULL);
	if (err < ER_OK) {
		P(("main: see_rec %d\n", err));
		goto fin2;
	}

	/* 解析・結果出力 */
	err = parse(fd, TADdata, TADsize, attr);
	if (err < ER_OK) {
		P(("main: parse %d\n", err));
		goto fin2;
	}

	err = ER_OK;
fin2:
	cls_fil(fd);
fin1:
	load_file(NULL);
fin0:
	return err;
}
