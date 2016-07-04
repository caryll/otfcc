/*
  Parser/Linter of CFF font, notable references:
    * Technical Note #5176: The Compact Font Format Specification
    * Technical Note #5177: The Type 2 Charstring Format
    * Adobe TinTin
*/

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cff_io.h"

static inline uint32_t gu1(uint8_t *s, uint32_t p) {
	uint32_t b0 = *(s + p);
	return b0;
}

static inline uint32_t gu2(uint8_t *s, uint32_t p) {
	uint32_t b0 = *(s + p) << 8;
	uint32_t b1 = *(s + p + 1);
	return b0 | b1;
}

static inline uint32_t gu3(uint8_t *s, uint32_t p) {
	uint32_t b0 = *(s + p) << 16;
	uint32_t b1 = *(s + p + 1) << 8;
	uint32_t b2 = *(s + p + 2);
	return b0 | b1 | b2;
}
static inline uint32_t gu4(uint8_t *s, uint32_t p) {
	uint32_t b0 = *(s + p) << 24;
	uint32_t b1 = *(s + p + 1) << 16;
	uint32_t b2 = *(s + p + 2) << 8;
	uint32_t b3 = *(s + p + 3);
	return b0 | b1 | b2 | b3;
}

static uint32_t count_index(CFF_INDEX i) {
	if (i.count != 0)
		return 3 + (i.offset[i.count] - 1) + ((i.count + 1) * i.offSize);
	else
		return 3;
}

void esrap_index(CFF_INDEX in) {
	if (in.offset != NULL) free(in.offset);
	if (in.data != NULL) free(in.data);
}

void empty_index(CFF_INDEX *in) {
	in->count = 0;
	in->offSize = 0;
	in->offset = NULL;
	in->data = NULL;
}

static void parse_index(uint8_t *data, uint32_t pos, CFF_INDEX *in) {
	in->count = gu2(data, pos);
	in->offSize = gu1(data, pos + 2);

	if (in->count > 0) {
		in->offset = calloc(in->count + 1, sizeof(uint32_t));

		for (int i = 0; i <= in->count; i++) {
			switch (in->offSize) {
				case 1:
					in->offset[i] = gu1(data, pos + 3 + (i * in->offSize));
					break;
				case 2:
					in->offset[i] = gu2(data, pos + 3 + (i * in->offSize));
					break;
				case 3:
					in->offset[i] = gu3(data, pos + 3 + (i * in->offSize));
					break;
				case 4:
					in->offset[i] = gu4(data, pos + 3 + (i * in->offSize));
					break;
			}
		}

		in->data = calloc(in->offset[in->count] - 1, sizeof(uint8_t));
		memcpy(in->data, data + pos + 3 + (in->count + 1) * in->offSize, in->offset[in->count] - 1);
	} else {
		in->offset = NULL;
		in->data = NULL;
	}
}

static void printf_cff_val(CFF_Value val) {
	if (val.t == CFF_INTEGER) printf("%d", val.i);

	if (val.t == CFF_DOUBLE) printf("%f", val.d);
}

void esrap_dict(CFF_Dict *d) {
	uint32_t i;

	if (d != NULL) {
		for (i = 0; i < d->count; i++) {
			if (d->ents[i].vals != NULL) free(d->ents[i].vals);
		}

		free(d->ents);
		free(d);
	}
}

CFF_Dict *parse_dict(uint8_t *data, uint32_t len) {
	CFF_Dict *dict = calloc(1, sizeof(CFF_Dict));
	uint32_t index = 0, advance;
	CFF_Value val, stack[48];
	uint8_t *temp = data;

	while (temp < data + len) {
		advance = decode_cff_token(temp, &val);

		switch (val.t) {
			case CFF_OPERATOR:
				dict->ents = realloc(dict->ents, sizeof(CFF_Dict_Entry) * (dict->count + 1));
				dict->ents[dict->count].op = val.i;
				dict->ents[dict->count].cnt = index;
				dict->ents[dict->count].vals = calloc(index, sizeof(CFF_Value));
				memcpy(dict->ents[dict->count].vals, stack, sizeof(CFF_Value) * index);
				dict->count++;
				index = 0;
				break;
			case CFF_INTEGER:
			case CFF_DOUBLE:
				stack[index++] = val;
				break;
		}

		temp += advance;
	}

	return dict;
}

void print_dict(uint8_t *data, uint32_t len) {
	uint32_t index = 0, advance;
	CFF_Value val, stack[48];
	uint8_t *temp = data;

	while (temp < data + len) {
		advance = decode_cff_token(temp, &val);

		switch (val.t) {
			case CFF_OPERATOR:
				for (uint32_t i = 0; i < index; i++) printf_cff_val(stack[i]), printf(" ");
				printf("%s\n", op_cff_name(val.i));
				index = 0;
				break;

			case CFF_INTEGER:
			case CFF_DOUBLE:
				stack[index++] = val;
				break;
		}

		temp += advance;
	}
}

CFF_Value parse_dict_key(uint8_t *data, uint32_t len, uint32_t op, uint32_t idx) {
	uint32_t index = 0, advance;
	CFF_Value val, res, stack[48];
	uint8_t *temp = data;

	res.t = 0;
	res.i = -1;

	if (data == NULL) return res;

	while (temp < data + len) {
		advance = decode_cff_token(temp, &val);

		switch (val.t) {
			case CFF_OPERATOR:
				if (val.i == (int32_t)op && idx <= index) res = stack[idx];
				index = 0;
				break;

			case CFF_INTEGER:
			case CFF_DOUBLE:
				stack[index++] = val;
				break;
		}

		temp += advance;
	}

	return res;
}

static void parse_encoding(CFF_File *cff, int32_t offset, CFF_Encoding *enc) {
	uint8_t *data = cff->raw_data;

	if (offset == CFF_ENC_STANDARD)
		enc->t = CFF_ENC_STANDARD;
	else if (offset == CFF_ENC_EXPERT)
		enc->t = CFF_ENC_EXPERT;
	else {
		switch (data[offset]) {
			case 0:
				enc->t = CFF_ENC_FORMAT0;
				{
					enc->f0.format = 0;
					enc->f0.ncodes = data[offset + 1];
					enc->f0.code = calloc(enc->f0.ncodes, sizeof(uint8_t));

					for (uint32_t i = 0; i < enc->f0.ncodes; i++)
						enc->f0.code[i] = data[offset + 2 + i];
				}
				break;
			case 1:
				enc->t = CFF_ENC_FORMAT1;
				{
					enc->f1.format = 1;
					enc->f1.nranges = data[offset + 1];
					enc->f1.range1 = calloc(enc->f1.nranges, sizeof(enc_range1));

					for (uint32_t i = 0; i < enc->f1.nranges; i++)
						enc->f1.range1[i].first = data[offset + 2 + i * 2],
						enc->f1.range1[i].nleft = data[offset + 3 + i * 2];
				}
				break;
			default:
				enc->t = CFF_ENC_FORMAT_SUPPLEMENT;
				{
					enc->ns.nsup = data[offset];
					enc->ns.supplement = calloc(enc->ns.nsup, sizeof(enc_supplement));

					for (uint32_t i = 0; i < enc->ns.nsup; i++)
						enc->ns.supplement[i].code = data[offset + 1 + i * 3],
						enc->ns.supplement[i].glyph = gu2(data, offset + 2 + i * 3);
				}
				break;
		}
	}
}

static void parse_charset(CFF_File *cff, int32_t offset, CFF_Charset *charsets) {
	uint32_t i;

	if (offset == CFF_CHARSET_ISOADOBE)
		charsets->t = CFF_CHARSET_ISOADOBE;
	else if (offset == CFF_CHARSET_EXPERT)
		charsets->t = CFF_CHARSET_EXPERT;
	else if (offset == CFF_CHARSET_EXPERTSUBSET)
		charsets->t = CFF_CHARSET_EXPERTSUBSET;
	else {
		switch (cff->raw_data[offset]) {
			case 0:
				charsets->t = CFF_CHARSET_FORMAT0;
				{
					charsets->s = cff->char_strings.count - 1;
					charsets->f0.glyph = calloc(cff->char_strings.count - 1, sizeof(uint16_t));

					for (i = 0; i < cff->char_strings.count - 1; i++)
						charsets->f0.glyph[i] = gu2(cff->raw_data, offset + 1 + i * 2);
				}
				break;
			case 1:
				charsets->t = CFF_CHARSET_FORMAT1;
				{
					uint32_t size;

					for (i = 0;; i++)
						if (gu2(cff->raw_data, offset + 1 + i * 3) +
						        gu1(cff->raw_data, offset + 3 + i * 3) ==
						    cff->char_strings.count - 1)
							break;

					size = i + 1;
					charsets->s = size;
					charsets->f1.range1 = calloc(i + 1, sizeof(charset_range1));

					for (i = 0; i < size; i++)
						charsets->f1.range1[i].first = gu2(cff->raw_data, offset + 1 + i * 3),
						charsets->f1.range1[i].nleft = gu1(cff->raw_data, offset + 3 + i * 3);
				}
				break;
			case 2:
				charsets->t = CFF_CHARSET_FORMAT2;
				{
					uint32_t size;

					for (i = 0;; i++)
						if (gu2(cff->raw_data, offset + 1 + i * 4) +
						        gu2(cff->raw_data, offset + 3 + i * 4) ==
						    cff->char_strings.count - 1)
							break;

					size = i + 1;
					charsets->s = size;
					charsets->f2.range2 = calloc(i + 1, sizeof(charset_range2));

					for (i = 0; i < size; i++) {
						charsets->f1.range1[i].first = gu2(cff->raw_data, offset + 1 + i * 4);
						charsets->f1.range1[i].nleft = gu2(cff->raw_data, offset + 3 + i * 4);
					}
				}
				break;
		}
	}
}

static void parse_fdselect(CFF_File *cff, int32_t offset, CFF_FDSelect *fdselect) {
	uint8_t *data = cff->raw_data;
	fdselect->t = data[offset];

	switch (data[offset]) {
		case 0:
			fdselect->t = CFF_FDSELECT_FORMAT0;
			{
				fdselect->f0.format = 0;
				fdselect->s = cff->char_strings.count - 1;
				fdselect->f0.fds = calloc(cff->char_strings.count - 1, sizeof(uint8_t));

				for (uint32_t i = 0; i < cff->char_strings.count - 1; i++)
					fdselect->f0.fds[i] = gu1(data, offset + 1 + i);
			}
			break;
		case 3:
			fdselect->t = CFF_FDSELECT_FORMAT3;
			{
				fdselect->f3.format = 3;
				fdselect->f3.nranges = gu2(data, offset + 1);
				fdselect->f3.range3 = calloc(fdselect->f3.nranges, sizeof(fdselect_range3));

				for (uint32_t i = 0; i < fdselect->f3.nranges; i++) {
					fdselect->f3.range3[i].first = gu2(data, offset + 3 + i * 3);
					fdselect->f3.range3[i].fd = gu1(data, offset + 5 + i * 3);
				}

				fdselect->f3.sentinel = gu2(data, offset + (fdselect->f3.nranges + 1) * 3);
			}
			break;
	}
}

static void parse_cff_bytecode(CFF_File *cff) {
	uint32_t pos;
	int32_t offset;

	/* Header */
	cff->head.major = gu1(cff->raw_data, 0);
	cff->head.minor = gu1(cff->raw_data, 1);
	cff->head.hdrSize = gu1(cff->raw_data, 2);
	cff->head.offSize = gu1(cff->raw_data, 3);

	/* Name INDEX */
	pos = cff->head.hdrSize;
	parse_index(cff->raw_data, pos, &cff->name);

	/* Top Dict INDEX */
	pos = 4 + count_index(cff->name);
	parse_index(cff->raw_data, pos, &cff->top_dict);

	/** LINT CFF FONTSET **/
	if (cff->name.count != cff->top_dict.count)
		printf("Bad CFF font: (%d, name), (%d, top_dict).\n", cff->name.count, cff->top_dict.count);

	/* String INDEX */
	pos = 4 + count_index(cff->name) + count_index(cff->top_dict);
	parse_index(cff->raw_data, pos, &cff->string);

	/* Global Subr INDEX */
	pos = 4 + count_index(cff->name) + count_index(cff->top_dict) + count_index(cff->string);
	parse_index(cff->raw_data, pos, &cff->global_subr);

	if (cff->top_dict.data != NULL) {
		int32_t offset;

		// cff->topdict.dict = calloc(1, sizeof(CFF_Dict));
		// cff->topdict.dict = parse_dict(cff->top_dict.data, cff->top_dict.offset[1] -
		// cff->top_dict.offset[0]);

		/* CharString INDEX */
		offset =
		    parse_dict_key(cff->top_dict.data, cff->top_dict.offset[1] - cff->top_dict.offset[0],
		                   op_CharStrings, 0)
		        .i;

		if (offset != -1) {
			parse_index(cff->raw_data, offset, &cff->char_strings);
			cff->cnt_glyph = cff->char_strings.count;

			// cff->topdict.charstrings = calloc(1, sizeof(CFF_INDEX));
			// parse_index(cff->raw_data, offset, cff->topdict.charstrings);
		} else {
			empty_index(&cff->char_strings);
			// cff->topdict.charstrings = NULL;
			printf("Bad CFF font: no any glyph data.\n");
		}

		/* Encodings */
		offset = parse_dict_key(cff->top_dict.data,
		                        cff->top_dict.offset[1] - cff->top_dict.offset[0], op_Encoding, 0)
		             .i;

		if (offset != -1) {
			parse_encoding(cff, offset, &cff->encodings);

			// cff->topdict.encoding = calloc(sizeof(CFF_Encoding), 1);
			// parse_encoding(cff, offset, cff->topdict.encoding);
		} else {
			cff->encodings.t = CFF_ENC_UNSPECED;
			// cff->topdict.encoding = NULL;
		}

		/* Charsets */
		offset = parse_dict_key(cff->top_dict.data,
		                        cff->top_dict.offset[1] - cff->top_dict.offset[0], op_charset, 0)
		             .i;

		if (offset != -1) {
			parse_charset(cff, offset, &cff->charsets);

			// cff->topdict.charset = calloc(1, sizeof(CFF_Charset));
			// parse_charset(cff, offset, cff->topdict.charset);
		} else {
			cff->charsets.t = CFF_CHARSET_UNSPECED;
			// cff->topdict.charset = NULL;
		}

		/* FDSelect */
		offset = parse_dict_key(cff->top_dict.data,
		                        cff->top_dict.offset[1] - cff->top_dict.offset[0], op_FDSelect, 0)
		             .i;

		if (offset != -1) {
			parse_fdselect(cff, offset, &cff->fdselect);

			// cff->topdict.fdselect = calloc(1, sizeof(CFF_FDSelect));
			// parse_fdselect(cff, offset, cff->topdict.fdselect);
		} else {
			cff->fdselect.t = CFF_FDSELECT_UNSPECED;
			// cff->topdict.fdselect = NULL;
		}

		/* Font Dict INDEX */
		offset = parse_dict_key(cff->top_dict.data,
		                        cff->top_dict.offset[1] - cff->top_dict.offset[0], op_FDArray, 0)
		             .i;

		if (offset != -1) {
			parse_index(cff->raw_data, offset, &cff->font_dict);

			// cff->topdict.fdarray = calloc(1, sizeof(CFF_INDEX));
			// parse_index(cff->raw_data, offset, cff->topdict.fdarray);
		} else {
			empty_index(&cff->font_dict);
			// cff->topdict.fdarray = NULL;
		}
	}

	/* Private and Local Subr */
	{
		int32_t private_len = -1;
		int32_t private_off = -1;

		if (cff->top_dict.data != NULL)
			private_len =
			    parse_dict_key(cff->top_dict.data,
			                   cff->top_dict.offset[1] - cff->top_dict.offset[0], op_Private, 0)
			        .i,
			private_off =
			    parse_dict_key(cff->top_dict.data,
			                   cff->top_dict.offset[1] - cff->top_dict.offset[0], op_Private, 1)
			        .i;

		if (private_off != -1 && private_len != -1) {
			offset = parse_dict_key(cff->raw_data + private_off, private_len, op_Subrs, 0).i;

			if (offset != -1)
				parse_index(cff->raw_data, private_off + offset, &cff->local_subr);
			else
				empty_index(&cff->local_subr);
		} else
			empty_index(&cff->local_subr);
	}
}

CFF_File *CFF_stream_open(uint8_t *data, uint32_t len) {
	CFF_File *file = calloc(1, sizeof(CFF_File));

	file->raw_data = calloc(len, sizeof(uint8_t));
	memcpy(file->raw_data, data, len);
	file->raw_length = len;
	file->cnt_glyph = 0;
	parse_cff_bytecode(file);

	return file;
}

CFF_File *CFF_file_open(const char *fname) {
	CFF_File *file;
	FILE *fptr = fopen(fname, "rb");

	if (fptr != NULL) {
		file = calloc(1, sizeof(CFF_File));
		fseek(fptr, 0, SEEK_END);
		file->raw_length = ftell(fptr);
		file->raw_data = calloc(file->raw_length, sizeof(uint8_t));
		file->cnt_glyph = 0;
		fseek(fptr, 0, SEEK_SET);
		(void)fread(file->raw_data, sizeof(uint8_t), file->raw_length, fptr);
		parse_cff_bytecode(file);
		fclose(fptr);
	} else
		file = NULL;

	return file;
}

CFF_File *CFF_sfnt_open(const char *fname, uint32_t offset, uint32_t len) {
	CFF_File *file;
	FILE *fptr = fopen(fname, "rb");

	if (fptr != NULL) {
		file = calloc(1, sizeof(CFF_File));
		file->raw_data = calloc(len, sizeof(uint8_t));
		file->raw_length = len;
		file->cnt_glyph = 0;
		fseek(fptr, offset, SEEK_SET);
		(void)fread(file->raw_data, sizeof(uint8_t), len, fptr);
		parse_cff_bytecode(file);
		fclose(fptr);
	} else {
		file = NULL;
	}

	return file;
}

void CFF_close(CFF_File *file) {
	if (file != NULL) {
		if (file->raw_data != NULL) free(file->raw_data);

		esrap_index(file->name);
		esrap_index(file->top_dict);
		esrap_index(file->string);
		esrap_index(file->global_subr);
		esrap_index(file->char_strings);
		esrap_index(file->font_dict);
		esrap_index(file->local_subr);

		switch (file->encodings.t) {
			case CFF_ENC_STANDARD:
			case CFF_ENC_EXPERT:
			case CFF_ENC_UNSPECED:
				break;
			case CFF_ENC_FORMAT0:
				if (file->encodings.f0.code != NULL) free(file->encodings.f0.code);
				break;
			case CFF_ENC_FORMAT1:
				if (file->encodings.f1.range1 != NULL) free(file->encodings.f1.range1);
				break;
			case CFF_ENC_FORMAT_SUPPLEMENT:
				if (file->encodings.ns.supplement != NULL) free(file->encodings.ns.supplement);
				break;
		}

		switch (file->charsets.t) {
			case CFF_CHARSET_EXPERT:
			case CFF_CHARSET_EXPERTSUBSET:
			case CFF_CHARSET_ISOADOBE:
			case CFF_CHARSET_UNSPECED:
				break;
			case CFF_CHARSET_FORMAT0:
				if (file->charsets.f0.glyph != NULL) free(file->charsets.f0.glyph);
				break;
			case CFF_CHARSET_FORMAT1:
				if (file->charsets.f1.range1 != NULL) free(file->charsets.f1.range1);
				break;
			case CFF_CHARSET_FORMAT2:
				if (file->charsets.f2.range2 != NULL) free(file->charsets.f2.range2);
				break;
		}

		switch (file->fdselect.t) {
			case CFF_FDSELECT_FORMAT0:
				if (file->fdselect.f0.fds != NULL) free(file->fdselect.f0.fds);
				break;
			case CFF_FDSELECT_FORMAT3:
				if (file->fdselect.f3.range3 != NULL) free(file->fdselect.f3.range3);
				break;
			case CFF_FDSELECT_UNSPECED:
				break;
		}

		free(file);
	}
}

void parse_subr(uint16_t idx, uint8_t *raw, CFF_INDEX fdarray, CFF_FDSelect select,
                CFF_INDEX *subr) {
	uint8_t fd;
	int32_t off_private, len_private;
	int32_t off_subr;

	switch (select.t) {
		case CFF_FDSELECT_FORMAT0:
			fd = select.f0.fds[idx];
			break;
		case CFF_FDSELECT_FORMAT3:
			for (int i = 0; i < select.f3.nranges - 1; i++)
				if (idx >= select.f3.range3[i].first && idx < select.f3.range3[i + 1].first)
					fd = select.f3.range3[i].fd;
			if (idx >= select.f3.range3[select.f3.nranges - 1].first && idx < select.f3.sentinel)
				fd = select.f3.range3[select.f3.nranges - 1].fd;
			break;
		case CFF_FDSELECT_UNSPECED:
			fd = 0;
			break;
	}

	off_private = parse_dict_key(fdarray.data + fdarray.offset[fd] - 1,
	                             fdarray.offset[fd + 1] - fdarray.offset[fd], op_Private, 1)
	                  .i;
	len_private = parse_dict_key(fdarray.data + fdarray.offset[fd] - 1,
	                             fdarray.offset[fd + 1] - fdarray.offset[fd], op_Private, 0)
	                  .i;

	if (off_private != -1 && len_private != -1) {
		off_subr = parse_dict_key(raw + off_private, len_private, op_Subrs, 0).i;

		if (off_subr != -1)
			parse_index(raw, off_private + off_subr, subr);
		else
			empty_index(subr);
	} else
		empty_index(subr);
}

/*
  |- dx1 dy1 rmoveto (21) |-
  |- dx1 hmoveto (22) |-
  |- dy1 vmoveto (4) |-
  |- {dxa dya}+ rlineto (5) |-
  |- dx1 {dya dxb}* hlineto (6) |-
  |- {dxa dyb}+ hlineto (6) |-
  |- dy1 {dxa dyb}* vlineto (7) |-
  |- {dya dxb}+ vlineto (7) |-
  |- {dxa dya dxb dyb dxc dyc}+ rrcurveto (8) |-
  |- dy1? {dxa dxb dyb dxc}+ hhcurveto (27) |-
  |- dx1 dx2 dy2 dy3 {dya dxb dyb dxc dxd dxe dye dyf}* dxf? hvcurveto (31) |-
  |- {dxa dxb dyb dyc dyd dxe dye dxf}+ dyf? hvcurveto (31) |-
  |- {dxa dya dxb dyb dxc dyc}+ dxd dyd rcurveline (24) |-
  |- {dxa dya}+ dxb dyb dxc dyc dxd dyd rlinecurve (25) |-
  |- dy1 dx2 dy2 dx3 {dxa dxb dyb dyc dyd dxe dye dxf}* dyf? vhcurveto (30) |-
  |- {dya dxb dyb dxc dxd dxe dye dyf}+ dxf? vhcurveto (30) |-
  |- dx1? {dya dxb dyb dyc}+ vvcurveto (26) |-
  |- dx1 dy1 dx2 dy2 dx3 dy3 dx4 dy4 dx5 dy5 dx6 dy6 fd flex (12 35) |-
  |- dx1 dx2 dy2 dx3 dx4 dx5 dx6 hflex (12 34) |-
  |- dx1 dy1 dx2 dy2 dx3 dx4 dx5 dy5 dx6 hflex1 (12 36) |-
  |- dx1 dy1 dx2 dy2 dx3 dy3 dx4 dy4 dx5 dy5 d6 flex1 (12 37) |-

  |- y dy {dya dyb}* hstem (1) |-
  |- x dx {dxa dxb}* vstem (3) |-
  |- y dy {dya dyb}* hstemhm (18) |-
  |- x dx {dxa dxb}* vstemhm (23) |-
  |- hintmask (19 + mask) |-
  |- cntrmask (20 + mask) |-

  subr# callsubr (10) -
  globalsubr# callgsubr (29) -
  - return (11) -
*/

static void print_hintmask(uint8_t mask) {
	uint8_t val = 0x80;

	for (int i = 0; i < 8; i++) printf("%d", (mask & val) ? 1 : 0), val /= 2;
}

static void print_charstring(uint32_t op, CFF_Value *stack, uint32_t index) {
	for (uint32_t i = 0; i < index; i++) {
		if (stack[i].t == CS2_FRACTION)
			printf("%g ", stack[i].d);
		else
			printf("%d ", stack[i].i);
	}

	if (op == op_return)
		/* do nothing */;
	else if (op == op_hintmask || op == op_cntrmask)
		printf("%s ", op_cs2_name(op));
	else
		printf("%s\n", op_cs2_name(op));
}

static inline uint16_t compute_subr_bias(uint16_t cnt) {
	if (cnt < 1240)
		return 107;
	else if (cnt < 33900)
		return 1131;
	else
		return 32768;
}

void print_glyph(uint8_t *data, uint32_t len, CFF_INDEX gsubr, CFF_INDEX lsubr, CFF_Stack *stack) {
	uint16_t gsubr_bias = compute_subr_bias(gsubr.count);
	uint16_t lsubr_bias = compute_subr_bias(lsubr.count);
	uint8_t *start = data;
	uint32_t advance;
	CFF_Value val;

	while (start < data + len) {
		advance = decode_cs2_token(start, &val);

		switch (val.t) {
			case CS2_OPERATOR:
				switch (val.i) {
					case op_hstem:
					case op_vstem:
					case op_hstemhm:
					case op_vstemhm:
						stack->stem += stack->index / 2;
						print_charstring(val.i, stack->stack, stack->index);
						stack->index = 0;
						break;
					case op_hintmask:
					case op_cntrmask: {
						stack->stem += stack->index / 2;
						uint32_t hint = (stack->stem + 7) / 8;
						print_charstring(val.i, stack->stack, stack->index);

						for (uint32_t i = 0; i < hint; i++) print_hintmask(*(start + advance + i));

						printf("\n");

						advance += hint;
						stack->index = 0;
					} break;
					case op_vmoveto:
					case op_rlineto:
					case op_hlineto:
					case op_vlineto:
					case op_rrcurveto:
					case op_endchar:
					case op_rmoveto:
					case op_hmoveto:
					case op_rcurveline:
					case op_rlinecurve:
					case op_vvcurveto:
					case op_hhcurveto:
					case op_vhcurveto:
					case op_hvcurveto:
					case op_hflex:
					case op_flex:
					case op_hflex1:
					case op_flex1:
						print_charstring(val.i, stack->stack, stack->index);
						stack->index = 0;
						break;
					case op_and: {
						double num1 = stack->stack[stack->index - 1].d;
						double num2 = stack->stack[stack->index - 2].d;
						stack->stack[stack->index - 2].d = (num1 && num2) ? 1.0 : 0.0;
						stack->index -= 1;
					} break;
					case op_or: {
						double num1 = stack->stack[stack->index - 1].d;
						double num2 = stack->stack[stack->index - 2].d;
						stack->stack[stack->index - 2].d = (num1 || num2) ? 1.0 : 0.0;
						stack->index -= 1;
					} break;
					case op_not: {
						double num = stack->stack[stack->index - 1].d;
						stack->stack[stack->index - 1].d = num ? 0.0 : 1.0;
					} break;
					case op_abs: {
						double num = stack->stack[stack->index - 1].d;
						stack->stack[stack->index - 1].d = (num < 0.0) ? -num : num;
					} break;
					case op_add: {
						double num1 = stack->stack[stack->index - 1].d;
						double num2 = stack->stack[stack->index - 2].d;
						stack->stack[stack->index - 2].d = num1 + num2;
						stack->index -= 1;
					} break;
					case op_sub: {
						double num1 = stack->stack[stack->index - 1].d;
						double num2 = stack->stack[stack->index - 2].d;
						stack->stack[stack->index - 2].d = num1 - num2;
						stack->index -= 1;
					} break;
					case op_div: {
						double num1 = stack->stack[stack->index - 1].d;
						double num2 = stack->stack[stack->index - 2].d;
						stack->stack[stack->index - 2].d = num1 / num2;
						stack->index -= 1;
					} break;
					case op_neg: {
						double num = stack->stack[stack->index - 1].d;
						stack->stack[stack->index - 1].d = -num;
					} break;
					case op_eq: {
						double num1 = stack->stack[stack->index - 1].d;
						double num2 = stack->stack[stack->index - 2].d;
						stack->stack[stack->index - 2].d = (num1 == num2) ? 1.0 : 0.0;
						stack->index -= 1;
					} break;
					case op_drop:
						break;
					case op_put: {
						double val = stack->stack[stack->index - 2].d;
						int32_t i = (int32_t)stack->stack[stack->index - 1].d;
						stack->transient[i].d = val;
						stack->index -= 2;
					} break;
					case op_get: {
						int32_t i = (int32_t)stack->stack[stack->index - 1].d;
						stack->stack[stack->index - 1].d = stack->transient[i].d;
					} break;
					case op_ifelse: {
						double v2 = stack->stack[stack->index - 1].d;
						double v1 = stack->stack[stack->index - 2].d;
						double s2 = stack->stack[stack->index - 3].d;
						double s1 = stack->stack[stack->index - 4].d;
						stack->stack[stack->index - 4].d = (v1 <= v2) ? s1 : s2;
						stack->index -= 3;
					} break;
					case op_random:
						break;
					case op_mul: {
						double num1 = stack->stack[stack->index - 1].d;
						double num2 = stack->stack[stack->index - 2].d;
						stack->stack[stack->index - 2].d = num1 * num2;
						stack->index -= 1;
					} break;
					case op_sqrt: {
						double num = stack->stack[stack->index - 1].d;
						stack->stack[stack->index - 1].d = sqrt(num);
					} break;
					case op_dup:
						break;
					case op_exch: {
						double num1 = stack->stack[stack->index - 1].d;
						double num2 = stack->stack[stack->index - 2].d;
						stack->stack[stack->index - 1].d = num2;
						stack->stack[stack->index - 2].d = num1;
					} break;
					case op_index:
						break;
					case op_roll:
						break;
					case op_return:
						print_charstring(val.i, stack->stack, stack->index);
						break;
					case op_callsubr: {
						uint32_t subr = (uint32_t)stack->stack[--(stack->index)].d;
						print_glyph(lsubr.data + lsubr.offset[lsubr_bias + subr] - 1,
						            lsubr.offset[lsubr_bias + subr + 1] -
						                lsubr.offset[lsubr_bias + subr],
						            gsubr, lsubr, stack);
					} break;
					case op_callgsubr: {
						uint32_t subr = (uint32_t)stack->stack[--(stack->index)].d;
						print_glyph(gsubr.data + gsubr.offset[gsubr_bias + subr] - 1,
						            gsubr.offset[gsubr_bias + subr + 1] -
						                gsubr.offset[gsubr_bias + subr],
						            gsubr, lsubr, stack);
					} break;
				}
				break;
			case CS2_OPERAND:
			case CS2_FRACTION:
				stack->stack[(stack->index)++] = val;
				break;
		}

		start += advance;
	}
}

static void outline_put_contour(CFF_Outline *outline, uint16_t off) {
	outline->cnt_contour += 1;
	outline->c = realloc(outline->c, sizeof(uint16_t) * (outline->cnt_contour));
	outline->c[outline->cnt_contour - 1] = off;
}

static void outline_put_point(CFF_Outline *outline, double x, double y, uint8_t t) {
	outline->cnt_point += 1;
	outline->x = realloc(outline->x, sizeof(double) * (outline->cnt_point));
	outline->y = realloc(outline->y, sizeof(double) * (outline->cnt_point));
	outline->t = realloc(outline->t, sizeof(uint8_t) * (outline->cnt_point));

	if (outline->cnt_point == 1)
		outline->x[outline->cnt_point - 1] = x, outline->y[outline->cnt_point - 1] = y;
	else
		outline->x[outline->cnt_point - 1] = outline->x[outline->cnt_point - 2] + x,
		                                outline->y[outline->cnt_point - 1] =
		                                    outline->y[outline->cnt_point - 2] + y;

	outline->t[outline->cnt_point - 1] = t;
}

static void outline_put_bezier(CFF_Outline *outline, double x1, double y1, double x2, double y2,
                               double x3, double y3) {
	outline_put_point(outline, x1, y1, 0);
	outline_put_point(outline, x2, y2, 0);
	outline_put_point(outline, x3, y3, 1);
}

CFF_INDEX *cff_index_init(void) {
	CFF_INDEX *out = calloc(1, sizeof(CFF_INDEX));
	return out;
}

void cff_index_fini(CFF_INDEX *out) {
	if (out != NULL) {
		if (out->offset != NULL) free(out->offset);
		if (out->data != NULL) free(out->data);
		free(out);
	}
}

CFF_Outline *cff_outline_init(void) {
	CFF_Outline *out = calloc(1, sizeof(CFF_Outline));
	return out;
}

void cff_outline_fini(CFF_Outline *out) {
	if (out != NULL) {
		if (out->x != NULL) free(out->x);
		if (out->y != NULL) free(out->y);
		if (out->t != NULL) free(out->t);
		if (out->c != NULL) free(out->c);
		free(out);
	}
}

/*
  CharString program:
    w? {hs* vs* cm* hm* mt subpath}? {mt subpath}* endchar
*/
#define CHECK_STACK_TOP(op, n)                                                                     \
	{                                                                                              \
		if (stack->index < n) {                                                                    \
			fprintf(stderr, "[libcff] Stack cannot provide enough parameters for %s (%04x). This " \
			                "operation is ignored.\n",                                             \
			        #op, op);                                                                      \
			break;                                                                                 \
		}                                                                                          \
	}
void parse_outline(uint8_t *data, uint32_t len, CFF_INDEX gsubr, CFF_INDEX lsubr, CFF_Stack *stack,
                   CFF_Outline *outline) {
	uint16_t gsubr_bias = compute_subr_bias(gsubr.count);
	uint16_t lsubr_bias = compute_subr_bias(lsubr.count);
	uint8_t *start = data;
	uint32_t advance, i, cnt_bezier;
	CFF_Value val;

	while (start < data + len) {
		advance = decode_cs2_token(start, &val);

		switch (val.t) {
			case CS2_OPERATOR:
				switch (val.i) {
					case op_hstem:
					case op_vstem:
					case op_hstemhm:
					case op_vstemhm:
						if (stack->index % 2) outline->width = stack->stack[0].d;
						stack->stem += stack->index / 2;
						stack->index = 0;
						break;
					case op_hintmask:
					case op_cntrmask: {
						if (stack->index % 2) outline->width = stack->stack[0].d;
						stack->stem += stack->index >> 1;
						uint32_t hint = (stack->stem + 7) >> 3;
						advance += hint;
						stack->index = 0;
						break;
					}

					case op_vmoveto: {
						CHECK_STACK_TOP(op_vmoveto, 1);
						if (stack->index > 1) outline->width = stack->stack[0].d;
						outline_put_contour(outline, outline->cnt_point);
						outline_put_point(outline, 0.0, stack->stack[stack->index - 1].d, 1);
						stack->index = 0;
						break;
					}
					case op_rmoveto: {
						CHECK_STACK_TOP(op_rmoveto, 2);
						if (stack->index > 2) outline->width = stack->stack[0].d;
						outline_put_contour(outline, outline->cnt_point);
						outline_put_point(outline, stack->stack[stack->index - 2].d,
						                  stack->stack[stack->index - 1].d, 1);
						stack->index = 0;
						break;
					}
					case op_hmoveto: {
						CHECK_STACK_TOP(op_hmoveto, 1);
						if (stack->index > 1) outline->width = stack->stack[0].d;
						outline_put_contour(outline, outline->cnt_point);
						outline_put_point(outline, stack->stack[stack->index - 1].d, 0.0, 1);
						stack->index = 0;
						break;
					}
					case op_endchar: {
						if (stack->index > 0) outline->width = stack->stack[0].d;
						break;
					}
					case op_rlineto: {
						for (i = 0; i < stack->index; i += 2)
							outline_put_point(outline, stack->stack[i].d, stack->stack[i + 1].d, 1);
						stack->index = 0;
						break;
					}
					case op_vlineto: {
						if (stack->index % 2 == 1) {
							outline_put_point(outline, 0.0, stack->stack[0].d, 1);
							for (i = 1; i < stack->index; i += 2) {
								outline_put_point(outline, stack->stack[i].d, 0.0, 1);
								outline_put_point(outline, 0.0, stack->stack[i + 1].d, 1);
							}
						} else {
							for (i = 0; i < stack->index; i += 2) {
								outline_put_point(outline, 0.0, stack->stack[i].d, 1);
								outline_put_point(outline, stack->stack[i + 1].d, 0.0, 1);
							}
						}
						stack->index = 0;
						break;
					}
					case op_hlineto: {
						if (stack->index % 2 == 1) {
							outline_put_point(outline, stack->stack[0].d, 0.0, 1);
							for (i = 1; i < stack->index; i += 2) {
								outline_put_point(outline, 0.0, stack->stack[i].d, 1);
								outline_put_point(outline, stack->stack[i + 1].d, 0.0, 1);
							}
						} else {
							for (i = 0; i < stack->index; i += 2) {
								outline_put_point(outline, stack->stack[i].d, 0.0, 1);
								outline_put_point(outline, 0.0, stack->stack[i + 1].d, 1);
							}
						}
						stack->index = 0;
						break;
					}
					case op_rrcurveto: {
						for (i = 0; i < stack->index; i += 6)
							outline_put_bezier(outline, stack->stack[i].d, stack->stack[i + 1].d,
							                   stack->stack[i + 2].d, stack->stack[i + 3].d,
							                   stack->stack[i + 4].d, stack->stack[i + 5].d);
						stack->index = 0;
						break;
					}
					case op_rcurveline: {
						for (i = 0; i < stack->index - 2; i += 6)
							outline_put_bezier(outline, stack->stack[i].d, stack->stack[i + 1].d,
							                   stack->stack[i + 2].d, stack->stack[i + 3].d,
							                   stack->stack[i + 4].d, stack->stack[i + 5].d);
						outline_put_point(outline, stack->stack[stack->index - 2].d,
						                  stack->stack[stack->index - 1].d, 1);
						stack->index = 0;
						break;
					}
					case op_rlinecurve: {
						for (i = 0; i < stack->index - 6; i += 2)
							outline_put_point(outline, stack->stack[i].d, stack->stack[i + 1].d, 1);
						outline_put_bezier(
						    outline, stack->stack[stack->index - 6].d,
						    stack->stack[stack->index - 5].d, stack->stack[stack->index - 4].d,
						    stack->stack[stack->index - 3].d, stack->stack[stack->index - 2].d,
						    stack->stack[stack->index - 1].d);
						stack->index = 0;
						break;
					}
					case op_vvcurveto: {
						if (stack->index % 4 == 1) {
							outline_put_bezier(outline, stack->stack[0].d, stack->stack[1].d,
							                   stack->stack[2].d, stack->stack[3].d, 0.0,
							                   stack->stack[4].d);
							for (i = 5; i < stack->index; i += 4)
								outline_put_bezier(outline, 0.0, stack->stack[i].d,
								                   stack->stack[i + 1].d, stack->stack[i + 2].d,
								                   0.0, stack->stack[i + 3].d);
						} else {
							for (i = 0; i < stack->index; i += 4)
								outline_put_bezier(outline, 0.0, stack->stack[i].d,
								                   stack->stack[i + 1].d, stack->stack[i + 2].d,
								                   0.0, stack->stack[i + 3].d);
						}
						stack->index = 0;
						break;
					}
					case op_hhcurveto: {
						if (stack->index % 4 == 1) {
							outline_put_bezier(outline, stack->stack[1].d, stack->stack[0].d,
							                   stack->stack[2].d, stack->stack[3].d,
							                   stack->stack[4].d, 0.0);
							for (i = 5; i < stack->index; i += 4)
								outline_put_bezier(outline, stack->stack[i].d, 0.0,
								                   stack->stack[i + 1].d, stack->stack[i + 2].d,
								                   stack->stack[i + 3].d, 0.0);
						} else {
							for (i = 0; i < stack->index; i += 4)
								outline_put_bezier(outline, stack->stack[i].d, 0.0,
								                   stack->stack[i + 1].d, stack->stack[i + 2].d,
								                   stack->stack[i + 3].d, 0.0);
						}
						stack->index = 0;
						break;
					}
					case op_vhcurveto: {
						if (stack->index % 4 == 1)
							cnt_bezier = (stack->index - 5) / 4;
						else
							cnt_bezier = (stack->index / 4);

						for (i = 0; i < 4 * cnt_bezier; i += 4) {
							if ((i / 4) % 2 == 0)
								outline_put_bezier(outline, 0.0, stack->stack[i].d,
								                   stack->stack[i + 1].d, stack->stack[i + 2].d,
								                   stack->stack[i + 3].d, 0.0);
							else
								outline_put_bezier(outline, stack->stack[i].d, 0.0,
								                   stack->stack[i + 1].d, stack->stack[i + 2].d,
								                   0.0, stack->stack[i + 3].d);
						}
						if (stack->index % 8 == 5) {
							outline_put_bezier(
							    outline, 0.0, stack->stack[stack->index - 5].d,
							    stack->stack[stack->index - 4].d, stack->stack[stack->index - 3].d,
							    stack->stack[stack->index - 2].d, stack->stack[stack->index - 1].d);
						}
						if (stack->index % 8 == 1) {
							outline_put_bezier(
							    outline, stack->stack[stack->index - 5].d, 0.0,
							    stack->stack[stack->index - 4].d, stack->stack[stack->index - 3].d,
							    stack->stack[stack->index - 1].d, stack->stack[stack->index - 2].d);
						}
						stack->index = 0;
						break;
					}
					case op_hvcurveto: {
						if (stack->index % 4 == 1)
							cnt_bezier = (stack->index - 5) / 4;
						else
							cnt_bezier = (stack->index / 4);

						for (i = 0; i < 4 * cnt_bezier; i += 4) {
							if ((i / 4) % 2 == 0)
								outline_put_bezier(outline, stack->stack[i].d, 0.0,
								                   stack->stack[i + 1].d, stack->stack[i + 2].d,
								                   0.0, stack->stack[i + 3].d);
							else
								outline_put_bezier(outline, 0.0, stack->stack[i].d,
								                   stack->stack[i + 1].d, stack->stack[i + 2].d,
								                   stack->stack[i + 3].d, 0.0);
						}

						if (stack->index % 8 == 5) {
							outline_put_bezier(
							    outline, stack->stack[stack->index - 5].d, 0.0,
							    stack->stack[stack->index - 4].d, stack->stack[stack->index - 3].d,
							    stack->stack[stack->index - 1].d, stack->stack[stack->index - 2].d);
						}
						if (stack->index % 8 == 1) {
							outline_put_bezier(
							    outline, 0.0, stack->stack[stack->index - 5].d,
							    stack->stack[stack->index - 4].d, stack->stack[stack->index - 3].d,
							    stack->stack[stack->index - 2].d, stack->stack[stack->index - 1].d);
						}
						stack->index = 0;
						break;
					}
					case op_hflex: {
						CHECK_STACK_TOP(op_hflex, 7);
						outline_put_bezier(outline, stack->stack[0].d, 0.0, stack->stack[1].d,
						                   stack->stack[2].d, stack->stack[3].d, 0.0);
						outline_put_bezier(outline, stack->stack[4].d, 0.0, stack->stack[5].d,
						                   -stack->stack[2].d, stack->stack[6].d, 0.0);
						stack->index = 0;
						break;
					}
					case op_flex: {
						CHECK_STACK_TOP(op_flex, 12);
						outline_put_bezier(outline, stack->stack[0].d, stack->stack[1].d,
						                   stack->stack[2].d, stack->stack[3].d, stack->stack[4].d,
						                   stack->stack[5].d);
						outline_put_bezier(outline, stack->stack[6].d, stack->stack[7].d,
						                   stack->stack[8].d, stack->stack[9].d, stack->stack[10].d,
						                   stack->stack[11].d);
						stack->index = 0;
						break;
					}
					case op_hflex1: {
						CHECK_STACK_TOP(op_hflex1, 9);
						outline_put_bezier(outline, stack->stack[0].d, stack->stack[1].d,
						                   stack->stack[2].d, stack->stack[3].d, stack->stack[4].d,
						                   0.0);
						outline_put_bezier(
						    outline, stack->stack[5].d, 0.0, stack->stack[6].d, stack->stack[7].d,
						    stack->stack[8].d,
						    -(stack->stack[1].d + stack->stack[3].d + stack->stack[7].d));
						stack->index = 0;
						break;
					}
					case op_flex1: {
						CHECK_STACK_TOP(op_flex1, 11);
						double dx = stack->stack[0].d + stack->stack[2].d + stack->stack[4].d +
						            stack->stack[6].d + stack->stack[8].d;
						double dy = stack->stack[1].d + stack->stack[3].d + stack->stack[5].d +
						            stack->stack[7].d + stack->stack[9].d;
						if (fabs(dx) > fabs(dy)) {
							dx = stack->stack[10].d;
							dy = -dy;
						} else {
							dx = -dx;
							dy = stack->stack[10].d;
						}
						outline_put_bezier(outline, stack->stack[0].d, stack->stack[1].d,
						                   stack->stack[2].d, stack->stack[3].d, stack->stack[4].d,
						                   stack->stack[5].d);
						outline_put_bezier(outline, stack->stack[6].d, stack->stack[7].d,
						                   stack->stack[8].d, stack->stack[9].d, dx, dy);
						stack->index = 0;
						break;
					}
					case op_and: {
						CHECK_STACK_TOP(op_and, 2);
						double num1 = stack->stack[stack->index - 1].d;
						double num2 = stack->stack[stack->index - 2].d;
						stack->stack[stack->index - 2].d = (num1 && num2) ? 1.0 : 0.0;
						stack->index -= 1;
						break;
					}
					case op_or: {
						CHECK_STACK_TOP(op_or, 2);
						double num1 = stack->stack[stack->index - 1].d;
						double num2 = stack->stack[stack->index - 2].d;
						stack->stack[stack->index - 2].d = (num1 || num2) ? 1.0 : 0.0;
						stack->index -= 1;
						break;
					}
					case op_not: {
						CHECK_STACK_TOP(op_not, 1);
						double num = stack->stack[stack->index - 1].d;
						stack->stack[stack->index - 1].d = num ? 0.0 : 1.0;
						break;
					}
					case op_abs: {
						CHECK_STACK_TOP(op_abs, 1);
						double num = stack->stack[stack->index - 1].d;
						stack->stack[stack->index - 1].d = (num < 0.0) ? -num : num;
						break;
					}
					case op_add: {
						CHECK_STACK_TOP(op_add, 2);
						double num1 = stack->stack[stack->index - 1].d;
						double num2 = stack->stack[stack->index - 2].d;
						stack->stack[stack->index - 2].d = num1 + num2;
						stack->index -= 1;
						break;
					}
					case op_sub: {
						CHECK_STACK_TOP(op_sub, 2);
						double num1 = stack->stack[stack->index - 1].d;
						double num2 = stack->stack[stack->index - 2].d;
						stack->stack[stack->index - 2].d = num1 - num2;
						stack->index -= 1;
						break;
					}
					case op_div: {
						CHECK_STACK_TOP(op_div, 2);
						double num1 = stack->stack[stack->index - 1].d;
						double num2 = stack->stack[stack->index - 2].d;
						stack->stack[stack->index - 2].d = num1 / num2;
						stack->index -= 1;
						break;
					}
					case op_neg: {
						CHECK_STACK_TOP(op_neg, 1);
						double num = stack->stack[stack->index - 1].d;
						stack->stack[stack->index - 1].d = -num;
						break;
					}
					case op_eq: {
						CHECK_STACK_TOP(op_eq, 2);
						double num1 = stack->stack[stack->index - 1].d;
						double num2 = stack->stack[stack->index - 2].d;
						stack->stack[stack->index - 2].d = (num1 == num2) ? 1.0 : 0.0;
						stack->index -= 1;
						break;
					}
					case op_drop: {
						CHECK_STACK_TOP(op_drop, 1);
						stack->index -= 1;
						break;
					}
					case op_put: {
						CHECK_STACK_TOP(op_put, 2);
						double val = stack->stack[stack->index - 2].d;
						int32_t i = (int32_t)stack->stack[stack->index - 1].d;
						stack->transient[i % CFF_LIMIT_TRANSIENT].d = val;
						stack->index -= 2;
						break;
					}
					case op_get: {
						CHECK_STACK_TOP(op_get, 1);
						int32_t i = (int32_t)stack->stack[stack->index - 1].d;
						stack->stack[stack->index - 1].d =
						    stack->transient[i % CFF_LIMIT_TRANSIENT].d;
						break;
					}
					case op_ifelse: {
						CHECK_STACK_TOP(op_ifelse, 4);
						double v2 = stack->stack[stack->index - 1].d;
						double v1 = stack->stack[stack->index - 2].d;
						double s2 = stack->stack[stack->index - 3].d;
						double s1 = stack->stack[stack->index - 4].d;
						stack->stack[stack->index - 4].d = (v1 <= v2) ? s1 : s2;
						stack->index -= 3;
						break;
					}
					case op_random:
						break;
					case op_mul: {
						CHECK_STACK_TOP(op_mul, 2);
						double num1 = stack->stack[stack->index - 1].d;
						double num2 = stack->stack[stack->index - 2].d;
						stack->stack[stack->index - 2].d = num1 * num2;
						stack->index -= 1;
						break;
					}
					case op_sqrt: {
						CHECK_STACK_TOP(op_sqrt, 1);
						double num = stack->stack[stack->index - 1].d;
						stack->stack[stack->index - 1].d = sqrt(num);
						break;
					}
					case op_dup: {
						CHECK_STACK_TOP(op_dup, 1);
						stack->stack[stack->index] = stack->stack[stack->index - 1];
						stack->index += 1;
						break;
					}
					case op_exch: {
						CHECK_STACK_TOP(op_exch, 2);
						double num1 = stack->stack[stack->index - 1].d;
						double num2 = stack->stack[stack->index - 2].d;
						stack->stack[stack->index - 1].d = num2;
						stack->stack[stack->index - 2].d = num1;
						break;
					}
					case op_index:
						break;
					case op_roll:
						break;
					case op_return:
						return;
					case op_callsubr: {
						CHECK_STACK_TOP(op_callsubr, 1);
						uint32_t subr = (uint32_t)stack->stack[--(stack->index)].d;
						parse_outline(lsubr.data + lsubr.offset[lsubr_bias + subr] - 1,
						              lsubr.offset[lsubr_bias + subr + 1] -
						                  lsubr.offset[lsubr_bias + subr],
						              gsubr, lsubr, stack, outline);
						break;
					}
					case op_callgsubr: {
						CHECK_STACK_TOP(op_callgsubr, 1);
						uint32_t subr = (uint32_t)stack->stack[--(stack->index)].d;
						parse_outline(gsubr.data + gsubr.offset[gsubr_bias + subr] - 1,
						              gsubr.offset[gsubr_bias + subr + 1] -
						                  gsubr.offset[gsubr_bias + subr],
						              gsubr, lsubr, stack, outline);
						break;
					}
				}
				break;
			case CS2_OPERAND:
			case CS2_FRACTION:
				stack->stack[(stack->index)++] = val;
				break;
		}

		start += advance;
	}
}
