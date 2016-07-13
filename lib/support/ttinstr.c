/* Copyright (C) 2001-2012 by George Williams */
/* Copyright (C) 2016 by Belleve Invis */

/*
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * The name of the author may not be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ttinstr.h"
#include <ctype.h>

const char *ff_ttf_instrnames[] = {"SVTCA[y-axis]",
                                   "SVTCA[x-axis]",
                                   "SPVTCA[y-axis]",
                                   "SPVTCA[x-axis]",
                                   "SFVTCA[y-axis]",
                                   "SFVTCA[x-axis]",
                                   "SPVTL[parallel]",
                                   "SPVTL[orthog]",
                                   "SFVTL[parallel]",
                                   "SFVTL[orthog]",
                                   "SPVFS",
                                   "SFVFS",
                                   "GPV",
                                   "GFV",
                                   "SFVTPV",
                                   "ISECT",
                                   "SRP0",
                                   "SRP1",
                                   "SRP2",
                                   "SZP0",
                                   "SZP1",
                                   "SZP2",
                                   "SZPS",
                                   "SLOOP",
                                   "RTG",
                                   "RTHG",
                                   "SMD",
                                   "ELSE",
                                   "JMPR",
                                   "SCVTCI",
                                   "SSWCI",
                                   "SSW",
                                   "DUP",
                                   "POP",
                                   "CLEAR",
                                   "SWAP",
                                   "DEPTH",
                                   "CINDEX",
                                   "MINDEX",
                                   "ALIGNPTS",
                                   "Unknown28",
                                   "UTP",
                                   "LOOPCALL",
                                   "CALL",
                                   "FDEF",
                                   "ENDF",
                                   "MDAP[no-rnd]",
                                   "MDAP[rnd]",
                                   "IUP[y]",
                                   "IUP[x]",
                                   "SHP[rp2]",
                                   "SHP[rp1]",
                                   "SHC[rp2]",
                                   "SHC[rp1]",
                                   "SHZ[rp2]",
                                   "SHZ[rp1]",
                                   "SHPIX",
                                   "IP",
                                   "MSIRP[no-rp0]",
                                   "MSIRP[rp0]",
                                   "ALIGNRP",
                                   "RTDG",
                                   "MIAP[no-rnd]",
                                   "MIAP[rnd]",
                                   "NPUSHB",
                                   "NPUSHW",
                                   "WS",
                                   "RS",
                                   "WCVTP",
                                   "RCVT",
                                   "GC[cur]",
                                   "GC[orig]",
                                   "SCFS",
                                   "MD[grid]",
                                   "MD[orig]",
                                   "MPPEM",
                                   "MPS",
                                   "FLIPON",
                                   "FLIPOFF",
                                   "DEBUG",
                                   "LT",
                                   "LTEQ",
                                   "GT",
                                   "GTEQ",
                                   "EQ",
                                   "NEQ",
                                   "ODD",
                                   "EVEN",
                                   "IF",
                                   "EIF",
                                   "AND",
                                   "OR",
                                   "NOT",
                                   "DELTAP1",
                                   "SDB",
                                   "SDS",
                                   "ADD",
                                   "SUB",
                                   "DIV",
                                   "MUL",
                                   "ABS",
                                   "NEG",
                                   "FLOOR",
                                   "CEILING",
                                   "ROUND[Grey]",
                                   "ROUND[Black]",
                                   "ROUND[White]",
                                   "ROUND[Undef4]",
                                   "NROUND[Grey]",
                                   "NROUND[Black]",
                                   "NROUND[White]",
                                   "NROUND[Undef4]",
                                   "WCVTF",
                                   "DELTAP2",
                                   "DELTAP3",
                                   "DELTAC1",
                                   "DELTAC2",
                                   "DELTAC3",
                                   "SROUND",
                                   "S45ROUND",
                                   "JROT",
                                   "JROF",
                                   "ROFF",
                                   "Unknown7B",
                                   "RUTG",
                                   "RDTG",
                                   "SANGW",
                                   "AA",
                                   "FLIPPT",
                                   "FLIPRGON",
                                   "FLIPRGOFF",
                                   "Unknown83",
                                   "Unknown84",
                                   "SCANCTRL",
                                   "SDPVTL[parallel]",
                                   "SDPVTL[orthog]",
                                   "GETINFO",
                                   "IDEF",
                                   "ROLL",
                                   "MAX",
                                   "MIN",
                                   "SCANTYPE",
                                   "INSTCTRL",
                                   "Unknown8F",
                                   "Unknown90",
                                   "Unknown91",
                                   "Unknown92",
                                   "Unknown93",
                                   "Unknown94",
                                   "Unknown95",
                                   "Unknown96",
                                   "Unknown97",
                                   "Unknown98",
                                   "Unknown99",
                                   "Unknown9A",
                                   "Unknown9B",
                                   "Unknown9C",
                                   "Unknown9D",
                                   "Unknown9E",
                                   "Unknown9F",
                                   "UnknownA0",
                                   "UnknownA1",
                                   "UnknownA2",
                                   "UnknownA3",
                                   "UnknownA4",
                                   "UnknownA5",
                                   "UnknownA6",
                                   "UnknownA7",
                                   "UnknownA8",
                                   "UnknownA9",
                                   "UnknownAA",
                                   "UnknownAB",
                                   "UnknownAC",
                                   "UnknownAD",
                                   "UnknownAE",
                                   "UnknownAF",
                                   "PUSHB_1",
                                   "PUSHB_2",
                                   "PUSHB_3",
                                   "PUSHB_4",
                                   "PUSHB_5",
                                   "PUSHB_6",
                                   "PUSHB_7",
                                   "PUSHB_8",
                                   "PUSHW_1",
                                   "PUSHW_2",
                                   "PUSHW_3",
                                   "PUSHW_4",
                                   "PUSHW_5",
                                   "PUSHW_6",
                                   "PUSHW_7",
                                   "PUSHW_8",
                                   "MDRP[grey]",
                                   "MDRP[black]",
                                   "MDRP[white]",
                                   "MDRP03",
                                   "MDRP[rnd,grey]",
                                   "MDRP[rnd,black]",
                                   "MDRP[rnd,white]",
                                   "MDRP07",
                                   "MDRP[min,grey]",
                                   "MDRP[min,black]",
                                   "MDRP[min,white]",
                                   "MDRP0b",
                                   "MDRP[min,rnd,grey]",
                                   "MDRP[min,rnd,black]",
                                   "MDRP[min,rnd,white]",
                                   "MDRP0f",
                                   "MDRP[rp0,grey]",
                                   "MDRP[rp0,black]",
                                   "MDRP[rp0,white]",
                                   "MDRP13",
                                   "MDRP[rp0,rnd,grey]",
                                   "MDRP[rp0,rnd,black]",
                                   "MDRP[rp0,rnd,white]",
                                   "MDRP17",
                                   "MDRP[rp0,min,grey]",
                                   "MDRP[rp0,min,black]",
                                   "MDRP[rp0,min,white]",
                                   "MDRP1b",
                                   "MDRP[rp0,min,rnd,grey]",
                                   "MDRP[rp0,min,rnd,black]",
                                   "MDRP[rp0,min,rnd,white]",
                                   "MDRP1f",
                                   "MIRP[grey]",
                                   "MIRP[black]",
                                   "MIRP[white]",
                                   "MIRP03",
                                   "MIRP[rnd,grey]",
                                   "MIRP[rnd,black]",
                                   "MIRP[rnd,white]",
                                   "MIRP07",
                                   "MIRP[min,grey]",
                                   "MIRP[min,black]",
                                   "MIRP[min,white]",
                                   "MIRP0b",
                                   "MIRP[min,rnd,grey]",
                                   "MIRP[min,rnd,black]",
                                   "MIRP[min,rnd,white]",
                                   "MIRP0f",
                                   "MIRP[rp0,grey]",
                                   "MIRP[rp0,black]",
                                   "MIRP[rp0,white]",
                                   "MIRP13",
                                   "MIRP[rp0,rnd,grey]",
                                   "MIRP[rp0,rnd,black]",
                                   "MIRP[rp0,rnd,white]",
                                   "MIRP17",
                                   "MIRP[rp0,min,grey]",
                                   "MIRP[rp0,min,black]",
                                   "MIRP[rp0,min,white]",
                                   "MIRP1b",
                                   "MIRP[rp0,min,rnd,grey]",
                                   "MIRP[rp0,min,rnd,black]",
                                   "MIRP[rp0,min,rnd,white]",
                                   "MIRP1f"};

static int strnmatch(const char *str1, const char *str2, int n) {
	int ch1, ch2;
	for (; n-- > 0;) {
		ch1 = *str1++;
		ch2 = *str2++;
		ch1 = tolower(ch1);
		ch2 = tolower(ch2);
		if (ch1 != ch2 || ch1 == '\0') return (ch1 - ch2);
	}
	return (0);
}

static uint8_t *parse_instrs(char *text, int *len, void *context,
                             void (*IVError)(void *context, char *, int)) {
	short numberstack[256];
	int npos = 0, nread, i;
	int push_left = 0, push_size = 0;
	char *pt;
	char *end, *bend, *brack;
	int icnt = 0, imax = (int)(strlen(text) / 2), val;
	uint8_t *instrs = malloc(imax);

	for (pt = text; *pt; ++pt) {
		npos = 0;
		while (npos < 256) {
			while (*pt == ' ' || *pt == '\t') ++pt;
			if (isdigit(*pt) || *pt == '-') {
				val = strtol(pt, &end, 0);
				if (val > 32767 || val < -32768) {
					IVError(context, "A value must be between [-32768,32767]", (int)(pt - text));
					return (NULL);
				}
				pt = end;
				numberstack[npos++] = val;
			} else
				break;
		}
		while (*pt == ' ' || *pt == '\t') ++pt;
		if (npos == 0 && (*pt == '\r' || *pt == '\n' || *pt == '\0')) continue;
		nread = 0;
		if (push_left == -1) {
			/* we need a push count */
			if (npos == 0)
				IVError(context, "Expected a number for a push count", (int)(pt - text));
			else if (numberstack[0] > 255 || numberstack[0] <= 0) {
				IVError(context, "The push count must be a number between 0 and 255",
				        (int)(pt - text));
				return (NULL);
			} else {
				nread = 1;
				instrs[icnt++] = numberstack[0];
				push_left = numberstack[0];
			}
		}
		if (push_left != 0 && push_left < npos - nread &&
		    (*pt == '\r' || *pt == '\n' || *pt == '\0')) {
			IVError(context, "More pushes specified than needed", (int)(pt - text));
			return (NULL);
		}
		while (push_left > 0 && nread < npos) {
			if (push_size == 2) {
				instrs[icnt++] = numberstack[nread] >> 8;
				instrs[icnt++] = numberstack[nread++] & 0xff;
			} else if (numberstack[0] > 255 || numberstack[0] < 0) {
				IVError(context, "A value to be pushed by a byte push must be between 0 and 255",
				        (int)(pt - text));
				return (NULL);
			} else
				instrs[icnt++] = numberstack[nread++];
			--push_left;
		}
		if (nread < npos && push_left == 0 && (*pt == '\r' || *pt == '\n' || *pt == '\0')) {
			IVError(context, "Unexpected number", (int)(pt - text));
			return (NULL);
		}
		if (*pt == '\r' || *pt == '\n' || *pt == '\0') continue;
		if (push_left > 0) {
			IVError(context, "Missing pushes", (int)(pt - text));
			return (NULL);
		}
		while (nread < npos) {
			i = nread;
			if (numberstack[nread] >= 0 && numberstack[nread] <= 255) {
				while (i < npos && numberstack[i] >= 0 && numberstack[i] <= 255) ++i;
				if (i - nread <= 8)
					instrs[icnt++] = ttf_pushb + (i - nread) - 1;
				else {
					instrs[icnt++] = ttf_npushb;
					instrs[icnt++] = i - nread;
				}
				while (nread < i) instrs[icnt++] = numberstack[nread++];
			} else {
				while (i < npos && (numberstack[i] < 0 || numberstack[i] > 255)) ++i;
				if (i - nread <= 8)
					instrs[icnt++] = ttf_pushw + (i - nread) - 1;
				else {
					instrs[icnt++] = ttf_npushw;
					instrs[icnt++] = i - nread;
				}
				while (nread < i) {
					instrs[icnt++] = numberstack[nread] >> 8;
					instrs[icnt++] = numberstack[nread++] & 0xff;
				}
			}
		}
		brack = NULL;
		for (end = pt; *end != '\r' && *end != '\n' && *end != ' ' && *end != '\0'; ++end)
			if (*end == '[' || *end == '_') brack = end;
		for (i = 0; i < 256; ++i)
			if (strnmatch(pt, ff_ttf_instrnames[i], (int)(end - pt)) == 0 &&
			    sizeof(char) * (end - pt) == strlen(ff_ttf_instrnames[i]))
				break;
		if (i == 256 && brack != NULL) {
			for (i = 0; i < 256; ++i)
				if (strnmatch(pt, ff_ttf_instrnames[i], (int)(brack - pt + 1)) == 0) break;
			val = strtol(brack + 1, &bend, 2); /* Stuff in brackets should be in binary */
			while (*bend == ' ' || *bend == '\t') ++bend;
			if (*bend != ']') {
				IVError(context,
				        "Missing right bracket in command (or bad binary value in bracket)",
				        (int)(pt - text));
				return (NULL);
			}
			if (val >= 32) {
				IVError(context, "Bracketted value is too large", (int)(pt - text));
				return (NULL);
			}
			i += val;
		}
		pt = end;
		instrs[icnt++] = i;
		if (i == ttf_npushb || i == ttf_npushw || (i >= ttf_pushb && i <= ttf_pushw + 7)) {
			push_size = (i == ttf_npushb || (i >= ttf_pushb && i <= ttf_pushb + 7)) ? 1 : 2;
			if (i == ttf_npushb || i == ttf_npushw)
				push_left = -1;
			else if (i >= ttf_pushb && i <= ttf_pushb + 7)
				push_left = i - ttf_pushb + 1;
			else
				push_left = i - ttf_pushw + 1;
		}
		if (*pt == '\0') break;
	}
	*len = icnt;
	return (realloc(instrs, icnt == 0 ? 1 : icnt)); /* some versions of realloc abort on 0 */
}

static int instr_typify(struct instrdata *id) {
	int i, len = id->instr_cnt, cnt, j, lh;
	uint8_t *instrs = id->instrs;
	uint8_t *bts;

	if (id->bts == NULL) id->bts = malloc(len + 1);
	bts = id->bts;
	for (i = lh = 0; i < len; ++i) {
		bts[i] = bt_instr;
		++lh;
		if (instrs[i] == ttf_npushb) {
			/* NPUSHB */
			bts[++i] = bt_cnt;
			cnt = instrs[i];
			for (j = 0; j < cnt; ++j) bts[++i] = bt_byte;
			lh += 1 + cnt;
		} else if (instrs[i] == ttf_npushw) {
			/* NPUSHW */
			bts[++i] = bt_cnt;
			++lh;
			cnt = instrs[i];
			for (j = 0; j < cnt; ++j) {
				bts[++i] = bt_wordhi;
				bts[++i] = bt_wordlo;
			}
			lh += 1 + cnt;
		} else if ((instrs[i] & 0xf8) == 0xb0) {
			/* PUSHB[n] */
			cnt = (instrs[i] & 7) + 1;
			for (j = 0; j < cnt; ++j) bts[++i] = bt_byte;
			lh += cnt;
		} else if ((instrs[i] & 0xf8) == 0xb8) {
			/* PUSHW[n] */
			cnt = (instrs[i] & 7) + 1;
			for (j = 0; j < cnt; ++j) {
				bts[++i] = bt_wordhi;
				bts[++i] = bt_wordlo;
			}
			lh += cnt;
		}
	}
	bts[i] = bt_impliedreturn;
	return (lh);
}

json_value *instr_to_json(uint8_t *instructions, uint32_t length, caryll_dump_options *dumpopts) {
	if (dumpopts->instr_as_bytes) {
		size_t len = 0;
		uint8_t *buf = base64_encode(instructions, length, &len);
		return json_string_new_nocopy((uint32_t)len, (char *)buf);
	} else {
		struct instrdata id;

		memset(&id, 0, sizeof(id));
		id.instr_cnt = length;
		id.instrs = instructions;
		instr_typify(&id);
		json_value *ret = json_array_new(id.instr_cnt);
		for (uint32_t i = 0; i < id.instr_cnt; ++i) {
			if (id.bts[i] == bt_wordhi) {
				json_array_push(
				    ret, json_integer_new((int16_t)((id.instrs[i] << 8) | id.instrs[i + 1])));
				++i;
			} else if (id.bts[i] == bt_cnt || id.bts[i] == bt_byte) {
				json_array_push(ret, json_integer_new(id.instrs[i]));
			} else {
				json_array_push(ret, json_string_new(ff_ttf_instrnames[id.instrs[i]]));
			}
		}
		free(id.bts);
		return preserialize(ret);
	}
}

void instr_from_json(json_value *col, void *context, void (*Make)(void *, uint8_t *, uint32_t),
                     void (*Wrong)(void *, char *, int)) {
	if (!col) {
		Make(context, NULL, 0);
	} else if (col->type == json_string) {
		size_t instrlen;
		uint8_t *instructions =
		    base64_decode((uint8_t *)col->u.string.ptr, col->u.string.length, &instrlen);
		Make(context, instructions, (uint32_t)instrlen);
	} else if (col->type == json_array) {
		size_t istrlen = 0;
		for (uint32_t j = 0; j < col->u.array.length; j++) {
			json_value *record = col->u.array.values[j];
			if (record->type == json_string) {
				istrlen += record->u.string.length + 1;
			} else if (record->type == json_integer) {
				istrlen += 1 + 20;
			} else {
				Make(context, NULL, 0);
				return;
			}
		}
		sds instrString = sdsnewlen(NULL, istrlen + 1);
		char *head = instrString;
		for (uint32_t j = 0; j < col->u.array.length; j++) {
			json_value *record = col->u.array.values[j];
			if (record->type == json_string) {
				memcpy(head, record->u.string.ptr, sizeof(char) * record->u.string.length);
				head += record->u.string.length;
			} else if (record->type == json_integer) {
				int n = snprintf(head, 20, "%d", (int)record->u.integer);
				head += n;
			}
			*head = '\n';
			head++;
		}
		int instrLength = 0;
		uint8_t *instructions = parse_instrs(instrString, &instrLength, context, Wrong);
		sdsfree(instrString);
		if (instructions && instrLength) {
			Make(context, instructions, instrLength);
		} else {
			Make(context, NULL, 0);
		}
	} else {
		Make(context, NULL, 0);
	}
}
