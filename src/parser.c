/*
	parser.c

	original code: SASANO Takayoshi
	--- public domain, no warranty.
*/

#include "common.h"

#define	TAB	0x0009
#define	LF	0x000a
#define	CR	0x000d

LOCAL	W	VOBJpos;
LOCAL	W	HankakuState;

/* $BJ8;zNs$N=PNO(B */
LOCAL	W	output_string(W fd, W len, TC *tc, W hankaku)
{
	W	err, n, pos;
	UH	sjis;
	UB	*buf;

	if (len < 1) {
		err = ER_OK;
		goto fin0;
	}

	/* $BF~NO%5%$%:$rD6$($k$3$H$O7h$7$F$J$$(B */
	buf = malloc(len);
	if (buf == NULL) {
		P(("output_string: malloc NULL\n"));
		err = ER_NOMEM;
		goto fin0;
	}

	len /= 2;
	pos = 0;
	while (len-- > 0) {
		switch (*tc) {
		case	TAB:
			buf[pos++] = TAB;
			break;

		case	LF:
		case	CR:
			buf[pos++] = CR;
			buf[pos++] = LF;
			break;

		default:
			/* $BH>3Q2=2DG=$JJ*$OH>3Q$G=PNO(B */
			if (hankaku) {
				n = zen2han(*tc, &buf[pos]);
				if (n > 0) {
					pos += n;
					break;
				}
			}

			sjis = _mbcjistojms(*tc);
			buf[pos++] = sjis >> 8;
			buf[pos++] = sjis;
			break;
		}

		tc++;
	}

	err = wri_rec(fd, -1, buf, pos, NULL, NULL, 0);
	if (err < ER_OK) {
		P(("output_string: wri_rec %d\n", err));
		goto fin1;
	}

fin1:
	free(buf);
fin0:
	return err;
}

/* $B<B?HL>$N=PNO(B */
LOCAL	W	output_objname(W fd, TC *tc, UW attr)
{
#define	TMPSZ	512

	W	i, err, hankaku, pos, len, n;
	UH	sjis;
	UB	name[(L_FNM + 1) * sizeof(TC)];
	UB	tmp[TMPSZ];

	if (attr & OPTION_HANKAKU) hankaku = TRUE;
	else if (attr & OPTION_ZENKAKU) hankaku = FALSE;
	else hankaku = HankakuState;

	/* $B<B?HL>$r(BSJIS$B$KJQ49(B */
	pos = 0;
	len = tc_strlen(tc);
	for (i = 0; i < len; i++) {
		if (hankaku) {
			n = zen2han(*tc, &name[pos]);
			if (n > 0) {
				pos += n;
				tc++;
				continue;
			}
		}

		sjis = _mbcjistojms(*tc++);
		name[pos++] = sjis >> 8;
		name[pos++] = sjis;
	}
	name[pos] = '\0';

	/* $B@07A(B */
	// snprintf()$B$,L5$$$N$G=PNO7k2L$N%5%$%:$KCm0U(B
	if (attr & OPTION_TAG_HTML) {
		sprintf(tmp, "<a href=\"%s\">%s</a>", name, name);
	} else if (attr & OPTION_TAG_WIKI) {
		sprintf(tmp, "[[%s]]", name);
	} else {
		sprintf(tmp, "%s", name);
	}

	/* $B=PNO(B */
	err = wri_rec(fd, -1, tmp, strlen(tmp), NULL, NULL, 0);
	if (err < ER_OK) {
		P(("output_objname: wri_rec %d\n", err));
	}

	return err;
}

/* $B%;%0%a%s%H$N2r<a(B */
LOCAL	W	parse_segment(W fd, W id, W len, VP dt, UW attr)
{
	W	err;
	UH	*d;

	switch (id) {
	default:
		/* $B2?$b$7$J$$(B */
		err = ER_OK;
		break;

	case	TS_TFONT:
		/* $BJ8;z;XDjIUd5(B */
		d = dt;
		if (d[0] == 0x0300 && len >= 6) { // $BJ8;z3HBg(B/$B=L>.;XDjIUd5(B
			HankakuState = (d[2] == 0x0102);
		}
		err = ER_OK;
		break;

	case	TS_VOBJ:
		/* $B2>?H(B */
		if (attr & OPTION_VOBJNAME) {
			if (VOBJpos < OBJentry) {
				err = output_objname(fd, OBJname[VOBJpos++].tc,
						     attr);
				if (err < ER_OK) {
					P(("parse_segment: output_objname %d\n", err));
				}
			} else {
				P(("parse_segment: objname not found, ignored\n"));
				err = ER_OK;
			}
		} else {
			/* $B<B?HL>$rE83+$7$J$$(B */
			err = ER_OK;
		}

		break;
	}

	return err;
}

/* $BHs%;%0%a%s%HItJ,$ND9$5$r5a$a$k(B */
LOCAL	W	nonseg_length(UH *buf, W size)
{
	W	i;

	for (i = 0; i < size; i++) {
		if (*buf++ >= 0xff00) break;
	}

	return i * sizeof(UH);	// TAD$B%G!<%?$KJo$$!"La$jCM$O(Bbyte$BC10L$H$9$k(B
}

/* TAD$B%G!<%?$r%;%0%a%s%HC10L$G%Q!<%9(B */
EXPORT	W	parse(W fd, UH *buf, W size, UW attr)
{
	W	err, i, pos, len;
	UH	id;

	VOBJpos = 0;
	HankakuState = 0;
	size /= 2;
	err = ER_OK;

	i = 0;
	while (i < size) {
		id = buf[pos = i];

		/* TAD$BJ8;zNs(B */
		if (id < 0xff00) {
			len = nonseg_length(&buf[pos], size - pos);
			err = output_string(fd, len, &buf[pos], HankakuState);
			if (err < ER_OK) {
				P(("parse: output_string %d\n", err));
				goto fin0;
			}
			i += len / 2;
			continue;
		}

		/* TAD$B%;%0%a%s%H(B */
		//       (normal)	(large)
		// pos+0 ID		ID
		// pos+1 length		0xffff
		// pos+2		length(low)
		// pos+3		length(high)

		/* ID$B$H(Blength$B$,FI$_=P$;$k$3$H(B */
		if (pos + 2 > size) break;
		len = buf[pos + 1];

		/* $B%i!<%8%;%0%a%s%H$N>l9g(B */
		if (len == 0xffff) {
			pos += 2;
			i += 2;
			if (pos + 2 > size) break;
			len = (buf[pos + 1] << 16) | buf[pos + 0];
		}

		/* $B%;%0%a%s%HK\BN$rA4$FFI$_=P$;$k$3$H(B */
		if (pos + 2 + len / 2 > size) break;
		err = parse_segment(fd, id & 0xff, len, &buf[pos + 2], attr);
		if (err < ER_OK) {
			P(("parse: parse_segment %d\n", err));
			goto fin0;
		}
		i += 2 + len / 2;
	}

fin0:
	return err;
}
