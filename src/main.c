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

	/* $B%X%k%W$NI=<((B */
	if (ac < 4) {
		P(("usage: %S [option] [TADfile] [SJISfile]\n", av[0]));
		P(("	option:	t	text only\n"));
		P(("		x	expand vobjname\n"));
		P(("		h	expand vobjname, hankaku\n"));
		P(("		z	expand vobjname, zenkaku\n"));
		err = ER_PAR;
		goto fin0;
	}

	/* $B%*%W%7%g%s$N@_Dj(B */
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

	/* TAD$B%G!<%?$NFI$_=P$7(B */
	err = load_file(path);
	if (err < ER_OK) goto fin0;

	/* $B%j%s%/$r3MF@(B	*/
	err = get_lnk(NULL, &l, F_NORM);
	if (err < ER_OK) {
		P(("main: get_lnk %d\n", err));
		goto fin1;
	}

	/* $B=PNO%U%!%$%k$N:n@.(B */
	err = cre_fil(&l, av[3], NULL, 1, F_FIX);
	if (err < ER_OK) {
		P(("main: cre_fil %d\n", err));
		goto fin1;
	}
	fd = err;

	/* $B6u$N%l%3!<%I$r=PNO(B */
	err = apd_rec(fd, &dummy, 0, 31, 0, 0);
	if (err < ER_OK) {
		P(("main: apd_rec %d\n", err));
		goto fin2;
	}

	/* $B=*C<%l%3!<%I$N0l$DA0$K0\F0$9$k(B */
	err = see_rec(fd, -1, -1, NULL);
	if (err < ER_OK) {
		P(("main: see_rec %d\n", err));
		goto fin2;
	}

	/* $B2r@O!&7k2L=PNO(B */
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
