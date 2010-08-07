/*
	loader.c

	original code: SASANO Takayoshi
	--- public domain, no warranty.
*/

#include "common.h"

EXPORT	TC		*TADdata = NULL;
EXPORT	W		TADsize;
EXPORT	FNAME		*OBJname = NULL;
EXPORT	W		OBJentry;

/* $B%U%!%$%k$N%m!<%I(B */
EXPORT	ERR	load_file(TC *filename)
{
	W	err, i, fd;
	LINK	l, v;
	F_LINK	stat;
	F_STATE	fs;

	/* $B%"%s%m!<%I=hM}(B */
	if (filename == NULL) {
		err = ER_OK;
		fd = ER_NOEXS;
		goto fin3;
	}

	/* $BF~NO%U%!%$%k$X$N%j%s%/<h$j=P$7(B */
	err = get_lnk(filename, &l, F_NORM);
	if (err < ER_OK) {
		P(("load_file: get_lnk %d\n", err));
		goto fin0;
	}

	/* $BF~NO%U%!%$%k$N%*!<%W%s(B - $B8=:_%l%3!<%I$O%U%!%$%k$N@hF,(B */
	err = opn_fil(&l, F_READ | F_EXCL, NULL);
	if (err < ER_OK) {
		P(("load_file: opn_fil %d\n", err));
		goto fin0;
	}
	fd = err;

	/* $B%j%s%/?t$N3MF@(B */
	err = ofl_sts(fd, NULL, &fs, NULL);
	if (err < ER_OK) {
		P(("load_file: ofl_sts %d\n", err));
		goto fin1;
	}

	/* $B2>?HL>MQNN0h$N3NJ](B */
	OBJentry = fs.f_nlink;
	if (OBJentry) {
		OBJname = calloc(OBJentry, sizeof(FNAME));
		if (OBJname == NULL) {
			P(("load_file: calloc NULL\n"));
			err = ER_NOMEM;
			goto fin1;
		}
	} else {
		OBJname = NULL;
	}

	/* $BA4%j%s%/$NFI$_=P$7(B */
	for (i = 0; i < OBJentry; i++) {
		err = fnd_lnk(fd, F_FWD, NULL, 0, NULL);
		if (err < ER_OK) {
			P(("load_file: fnd_lnk %d\n", err));
			goto fin2;
		}

		err = rea_rec(fd, 0, (B *)&v, sizeof(VLINK),
			      NULL, NULL);
		if (err < ER_OK) {
			P(("load_file: rea_rec %d\n", err));
			goto fin2;
		}

		/* $B%j%s%/%U%!%$%k(B/$B%j%s%/%l%3!<%I$K1~$8$F=hM}(B */
		err = lnk_sts((LINK *)&v, &stat);
		if (err >= ER_OK) {
			memcpy(OBJname[i].tc, stat.f_name, sizeof(stat.f_name));
		} else if (err == ER_NOLNK) {
			err = fil_sts((LINK *)&v, OBJname[i].tc, NULL, NULL);
		}
		if (err < ER_OK) {
			P(("load_file: fil_sts %d\n", err));
			P(("load_file: warning, this object is ignored\n"));
			/* goto fin2; */
		}

		err = see_rec(fd, 1, 0, NULL);
		if (err < ER_OK) {
			P(("load_flie: see_rec %d\n", err));
			goto fin2;
		}
	}

	/* $B<g(BTAD$B%l%3!<%I$X0\F0(B */
	err = fnd_rec(fd, F_ENDTOP, (1 << 1), 0, NULL);
	if (err < ER_OK) {
		P(("load_file: fnd_rec %d\n", err));
		goto fin2;
	}

	/* $B%l%3!<%I%5%$%:$N<hF@(B */
	err = rea_rec(fd, 0, NULL, 0, &TADsize, NULL);
	if (err < ER_OK) {
		P(("load_file: rea_rec %d\n", err));
		goto fin2;
	}

	/* $B%a%b%j$N3NJ](B */
	TADdata = malloc(TADsize);
	if (TADdata == NULL) {
		P(("load_file: malloc NULL\n"));
		err = ER_NOMEM;
		goto fin2;
	}

	/* $B%l%3!<%IA4BN$NFI$_9~$_(B */
	err = rea_rec(fd, 0, (B *)TADdata, TADsize, NULL, NULL);
	if (err < ER_OK) {
		P(("load_file: rea_rec %d\n", err));
		goto fin3;
	}

	DP(("load_file: TADsize %d, OBJentry %d\n", TADsize, OBJentry));

	/* $B%U%!%$%k$r%/%m!<%:$7$F=*N;(B */
	goto fin1;	

fin3:
	if (TADdata != NULL) {
		free(TADdata);
		TADdata = NULL;
	}
fin2:
	if (OBJname != NULL) {
		free(OBJname);
		OBJname = NULL;
	}
fin1:
	if (fd >= 0) cls_fil(fd);
fin0:
	return err;
}
