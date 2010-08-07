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

/* 文字列の出力 */
LOCAL	W	output_string(W fd, W len, TC *tc, W hankaku)
{
	W	err, n, pos;
	UH	sjis;
	UB	*buf;

	if (len < 1) {
		err = ER_OK;
		goto fin0;
	}

	/* 入力サイズを超えることは決してない */
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
			/* 半角化可能な物は半角で出力 */
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

/* 実身名の出力 */
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

	/* 実身名をSJISに変換 */
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

	/* 整形 */
	// snprintf()が無いので出力結果のサイズに注意
	if (attr & OPTION_TAG_HTML) {
		sprintf(tmp, "<a href=\"%s\">%s</a>", name, name);
	} else if (attr & OPTION_TAG_WIKI) {
		sprintf(tmp, "[[%s]]", name);
	} else {
		sprintf(tmp, "%s", name);
	}

	/* 出力 */
	err = wri_rec(fd, -1, tmp, strlen(tmp), NULL, NULL, 0);
	if (err < ER_OK) {
		P(("output_objname: wri_rec %d\n", err));
	}

	return err;
}

/* セグメントの解釈 */
LOCAL	W	parse_segment(W fd, W id, W len, VP dt, UW attr)
{
	W	err;
	UH	*d;

	switch (id) {
	default:
		/* 何もしない */
		err = ER_OK;
		break;

	case	TS_TFONT:
		/* 文字指定付箋 */
		d = dt;
		if (d[0] == 0x0300 && len >= 6) { // 文字拡大/縮小指定付箋
			HankakuState = (d[2] == 0x0102);
		}
		err = ER_OK;
		break;

	case	TS_VOBJ:
		/* 仮身 */
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
			/* 実身名を展開しない */
			err = ER_OK;
		}

		break;
	}

	return err;
}

/* 非セグメント部分の長さを求める */
LOCAL	W	nonseg_length(UH *buf, W size)
{
	W	i;

	for (i = 0; i < size; i++) {
		if (*buf++ >= 0xff00) break;
	}

	return i * sizeof(UH);	// TADデータに倣い、戻り値はbyte単位とする
}

/* TADデータをセグメント単位でパース */
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

		/* TAD文字列 */
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

		/* TADセグメント */
		//       (normal)	(large)
		// pos+0 ID		ID
		// pos+1 length		0xffff
		// pos+2		length(low)
		// pos+3		length(high)

		/* IDとlengthが読み出せること */
		if (pos + 2 > size) break;
		len = buf[pos + 1];

		/* ラージセグメントの場合 */
		if (len == 0xffff) {
			pos += 2;
			i += 2;
			if (pos + 2 > size) break;
			len = (buf[pos + 1] << 16) | buf[pos + 0];
		}

		/* セグメント本体を全て読み出せること */
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
