/*
  Compiler of CFF, subset and full conversion
    * note that there is no optimization in current implement
*/

#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cff_io.h"

void blob_merge(cff_blob *dst, cff_blob *src) {
	dst->data = realloc(dst->data, dst->size + src->size);
	memcpy(dst->data + dst->size, src->data, src->size);
	dst->size += src->size;
}

void blob_free(cff_blob *b) {
	if (b != NULL) {
		if (b->data != NULL) free(b->data);
		free(b);
	}
}

cff_blob *compile_header(void) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));

	blob->size = 4;
	blob->data = calloc(blob->size, sizeof(uint8_t));
	blob->data[0] = 1;
	blob->data[1] = 0;
	blob->data[2] = 4;
	blob->data[3] = 4;

	return blob;
}

cff_blob *compile_index(CFF_INDEX index) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));
	uint32_t i;

	if (index.count != 0)
		blob->size = 3 + (index.offset[index.count] - 1) + ((index.count + 1) * index.offSize);
	else
		blob->size = 3;

	blob->data = calloc(blob->size, sizeof(uint8_t));
	blob->data[0] = index.count / 256;
	blob->data[1] = index.count % 256;
	blob->data[2] = index.offSize;

	if (index.count > 0) {
		for (i = 0; i <= index.count; i++) {
			switch (index.offSize) {
				case 1:
					blob->data[3 + i] = index.offset[i];
					break;
				case 2:
					blob->data[3 + i * 2] = index.offset[i] / 256;
					blob->data[4 + i * 2] = index.offset[i] % 256;
					break;
				case 3:
					blob->data[3 + i * 3] = index.offset[i] / 65536;
					blob->data[4 + i * 3] = (index.offset[i] % 65536) / 256;
					blob->data[5 + i * 3] = (index.offset[i] % 65536) % 256;
					break;
				case 4:
					blob->data[3 + i * 4] = (index.offset[i] / 65536) / 256;
					blob->data[4 + i * 4] = (index.offset[i] / 65536) % 256;
					blob->data[5 + i * 4] = (index.offset[i] % 65536) / 256;
					blob->data[6 + i * 4] = (index.offset[i] % 65536) % 256;
					break;
			}
		}

		if (index.data != NULL)
			memcpy(blob->data + 3 + ((index.count + 1) * index.offSize), index.data,
			       index.offset[index.count] - 1);
	}

	return blob;
}

cff_blob *compile_encoding(CFF_Encoding enc) {
	switch (enc.t) {
		case CFF_ENC_STANDARD:
		case CFF_ENC_EXPERT:
		case CFF_ENC_UNSPECED: {
			cff_blob *blob = calloc(1, sizeof(cff_blob));
			return blob;
		}
		case CFF_ENC_FORMAT0: {
			cff_blob *blob = calloc(1, sizeof(cff_blob));
			blob->size = 2 + enc.f0.ncodes;
			blob->data = calloc(blob->size, sizeof(uint8_t));
			blob->data[0] = 0;
			blob->data[1] = enc.f0.ncodes;
			memcpy(blob->data + 2, enc.f0.code, enc.f0.ncodes);
			return blob;
		}
		case CFF_ENC_FORMAT1: {
			cff_blob *blob = calloc(1, sizeof(cff_blob));
			blob->size = 2 + 2 * enc.f1.nranges;
			blob->data = calloc(blob->size, sizeof(uint8_t));
			blob->data[0] = 1;
			blob->data[1] = enc.f1.nranges;
			for (int i = 0; i < enc.f1.nranges; i++)
				blob->data[2 + i * 2] = enc.f1.range1[i].first,
				                   blob->data[3 + i * 2] = enc.f1.range1[i].nleft;
			return blob;
		}
		case CFF_ENC_FORMAT_SUPPLEMENT: {
			cff_blob *blob = calloc(1, sizeof(cff_blob));
			blob->size = 1 + 3 * enc.ns.nsup;
			blob->data = calloc(blob->size, sizeof(uint8_t));
			blob->data[0] = enc.ns.nsup;
			for (int i = 0; i < enc.ns.nsup; i++)
				blob->data[1 + 3 * i] = enc.ns.supplement[i].code,
				                   blob->data[2 + 3 * i] = enc.ns.supplement[i].glyph / 256,
				                   blob->data[3 + 3 * 1] = enc.ns.supplement[i].glyph % 256;
			return blob;
		}
	}
	return NULL;
}

cff_blob *compile_charset(CFF_Charset cset) {
	switch (cset.t) {
		case CFF_CHARSET_ISOADOBE:
		case CFF_CHARSET_EXPERT:
		case CFF_CHARSET_EXPERTSUBSET:
		case CFF_CHARSET_UNSPECED: {
			cff_blob *blob = calloc(1, sizeof(cff_blob));
			return blob;
		}
		case CFF_CHARSET_FORMAT0: {
			cff_blob *blob = calloc(1, sizeof(cff_blob));
			blob->size = 1 + cset.s * 2;
			blob->data = calloc(blob->size, sizeof(uint8_t));
			blob->data[0] = 0;
			for (int i = 0; i < cset.s; i++)
				blob->data[1 + 2 * i] = cset.f0.glyph[i] / 256,
				                   blob->data[2 + 2 * i] = cset.f0.glyph[i] % 256;
			return blob;
		}
		case CFF_CHARSET_FORMAT1: {
			cff_blob *blob = calloc(1, sizeof(cff_blob));
			blob->size = 1 + cset.s * 3;
			blob->data = calloc(blob->size, sizeof(uint8_t));
			blob->data[0] = 1;
			for (int i = 0; i < cset.s; i++)
				blob->data[1 + 3 * i] = cset.f1.range1[i].first / 256,
				                   blob->data[2 + 3 * i] = cset.f1.range1[i].first % 256,
				                   blob->data[3 + 3 * i] = cset.f1.range1[i].nleft;
			return blob;
		}
		case CFF_CHARSET_FORMAT2: {
			cff_blob *blob = calloc(1, sizeof(cff_blob));
			blob->size = 1 + cset.s * 4;
			blob->data = calloc(blob->size, sizeof(uint8_t));
			blob->data[0] = 2;
			for (int i = 0; i < cset.s; i++)
				blob->data[1 + 4 * i] = cset.f2.range2[i].first / 256,
				                   blob->data[2 + 4 * i] = cset.f2.range2[i].first % 256,
				                   blob->data[3 + 4 * i] = cset.f2.range2[i].nleft / 256,
				                   blob->data[4 + 4 * i] = cset.f2.range2[i].nleft % 256;
			return blob;
		}
	}
	return NULL;
}

cff_blob *compile_fdselect(CFF_FDSelect fd) {
	switch (fd.t) {
		case CFF_FDSELECT_UNSPECED: {
			cff_blob *blob = calloc(1, sizeof(cff_blob));
			return blob;
		}
		case CFF_FDSELECT_FORMAT0: {
			cff_blob *blob = calloc(1, sizeof(cff_blob));
			blob->size = 1 + fd.s;
			blob->data = calloc(blob->size, sizeof(uint8_t));
			blob->data[0] = 0;
			memcpy(blob->data + 1, fd.f0.fds, fd.s);
			return blob;
		}
		case CFF_FDSELECT_FORMAT3: {
			cff_blob *blob = calloc(1, sizeof(cff_blob));
			blob->size = 5 + fd.f3.nranges * 3;
			blob->data = calloc(blob->size, sizeof(uint8_t));
			blob->data[0] = 3;
			blob->data[1] = fd.f3.nranges / 256;
			blob->data[2] = fd.f3.nranges % 256;
			for (int i = 0; i < fd.f3.nranges; i++)
				blob->data[3 + 3 * i] = fd.f3.range3[i].first / 256,
				                   blob->data[4 + 3 * i] = fd.f3.range3[i].first % 256,
				                   blob->data[5 + 3 * i] = fd.f3.range3[i].fd;
			blob->data[blob->size - 2] = fd.f3.sentinel / 256;
			blob->data[blob->size - 1] = fd.f3.sentinel % 256;
			return blob;
		}
	}
	return NULL;
}

cff_blob *compile_name(const char *name) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));
	blob->size = 2 + 1 + 2 + strlen(name);
	blob->data = calloc(blob->size, sizeof(uint8_t));
	blob->data[0] = 0;
	blob->data[1] = 1;
	blob->data[2] = 1;
	blob->data[3] = 1;
	blob->data[4] = strlen(name) + 1;
	memcpy(blob->data + 5, name, strlen(name));

	return blob;
}

cff_blob *compile_pubdict(uint8_t *data, int32_t len) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));
	CFF_Dict *dict = parse_dict(data, len);
	blob->size = 0;

	for (int i = 0; i < dict->count; i++) {
		switch (dict->ents[i].op) {
			case op_version:
			case op_Notice:
			case op_Copyright:
			case op_FullName:
			case op_FamilyName:
			case op_Weight:
			case op_isFixedPitch:
			case op_ItalicAngle:
			case op_UnderlinePosition:
			case op_UnderlineThickness:
			case op_PaintType:
			case op_CharstringType:
			case op_FontMatrix:
			case op_UniqueID:
			case op_FontBBox:
			case op_StrokeWidth:
			case op_XUID:
			case op_SyntheicBase:
			case op_PostScript:
			case op_BaseFontName:
			case op_BaseFontBlend:
			case op_ROS:
			case op_CIDFontVersion:
			case op_CIDFontRevision:
			case op_CIDFontType:
			case op_CIDCount:
			case op_UIDBase:
			case op_FontName:
				for (int j = 0; j < dict->ents[i].cnt; j++) {
					cff_blob *blob_val;

					if (dict->ents[i].vals[j].t == CFF_INTEGER)
						blob_val = encode_cff_number(dict->ents[i].vals[j].i);

					if (dict->ents[i].vals[j].t == CFF_DOUBLE)
						blob_val = encode_cff_real(dict->ents[i].vals[j].d);

					blob_merge(blob, blob_val);
					blob_free(blob_val);
				}

				{
					cff_blob *blob_op = encode_cff_operator(dict->ents[i].op);
					blob_merge(blob, blob_op);
					free(blob_op->data);
				}
				break;
			case op_charset: {
				int32_t check = dict->ents[i].vals[0].i;
				if (check >= 0 && check <= 2) {
					cff_blob *blob_val = encode_cff_number(check);
					cff_blob *blob_op = encode_cff_operator(dict->ents[i].op);
					blob_merge(blob, blob_val);
					blob_merge(blob, blob_op);
					blob_free(blob_val);
					blob_free(blob_op);
				}
			} break;
			case op_Encoding: {
				int32_t check = dict->ents[i].vals[0].i;
				if (check >= 0 && check <= 1) {
					cff_blob *blob_val = encode_cff_number(check);
					cff_blob *blob_op = encode_cff_operator(dict->ents[i].op);
					blob_merge(blob, blob_val);
					blob_merge(blob, blob_op);
					blob_free(blob_val);
					blob_free(blob_op);
				}
			} break;
			case op_CharStrings:
			case op_Private:
			case op_FDArray:
			case op_FDSelect:
				break;
		}
	}

	return blob;
}

cff_blob *compile_topdict(CFF_INDEX top) {
	return compile_pubdict(top.data, top.offset[1] - top.offset[0]);
}

cff_blob *compile_private(CFF_File *f) {
	int32_t private_len =
	    parse_dict_key(f->top_dict.data, f->top_dict.offset[1] - f->top_dict.offset[0], op_Private,
	                   0)
	        .i;
	int32_t private_off =
	    parse_dict_key(f->top_dict.data, f->top_dict.offset[1] - f->top_dict.offset[0], op_Private,
	                   1)
	        .i;

	cff_blob *blob = calloc(1, sizeof(cff_blob));

	if (private_len != -1 && private_off != -1) {
		CFF_Dict *dict = parse_dict(f->raw_data + private_off, private_len);
		blob->size = 0;

		for (int i = 0; i < dict->count; i++) {
			switch (dict->ents[i].op) {
				case op_BlueValues:
				case op_OtherBlues:
				case op_FamilyBlues:
				case op_FamilyOtherBlues:
				case op_BlueScale:
				case op_BlueShift:
				case op_BlueFuzz:
				case op_StdHW:
				case op_StdVW:
				case op_StemSnapH:
				case op_StemSnapV:
				case op_ForceBold:
				case op_LanguageGroup:
				case op_ExpansionFactor:
				case op_initialRandomSeed:
				case op_defaultWidthX:
				case op_nominalWidthX:
					for (int j = 0; j < dict->ents[i].cnt; j++) {
						cff_blob *blob_val;

						if (dict->ents[i].vals[j].t == CFF_INTEGER)
							blob_val = encode_cff_number(dict->ents[i].vals[j].i);
						if (dict->ents[i].vals[j].t == CFF_DOUBLE)
							blob_val = encode_cff_real(dict->ents[i].vals[j].d);

						blob_merge(blob, blob_val);
						blob_free(blob_val);
					}

					{
						cff_blob *blob_op = encode_cff_operator(dict->ents[i].op);
						blob_merge(blob, blob_op);
						blob_free(blob_op);
					}
					break;
				case op_Subrs:
					break;
			}
		}
	}

	return blob;
}

cff_blob *compile_fdarray(CFF_INDEX fdarray) {
	CFF_INDEX *newfd = cff_index_init();
	cff_blob *final_blob;

	for (uint32_t i = 0; i < fdarray.count; i++) {
		cff_blob *blob = compile_pubdict(fdarray.data + fdarray.offset[i] - 1,
		                                 fdarray.offset[i + 1] - fdarray.offset[i]);
		newfd->count += 1;
		newfd->offset = realloc(newfd->offset, (newfd->count + 1) * sizeof(uint32_t));

		if (newfd->count == 1) newfd->offset[0] = 1;

		newfd->offset[newfd->count] = blob->size + newfd->offset[newfd->count - 1];
		newfd->data = realloc(newfd->data, newfd->offset[newfd->count] - 1);
		memcpy(newfd->data + newfd->offset[newfd->count - 1] - 1, blob->data, blob->size);
		blob_free(blob);
	}

	newfd->offSize = 4;
	final_blob = compile_index(*newfd);
	cff_index_fini(newfd);

	return final_blob;
}

static cff_blob *compile_type2_value(double val) {
	double intpart;

	if (modf(val, &intpart) == 0.0)
		return encode_cs2_number(intpart);
	else
		return encode_cs2_real(val);
}

static cff_blob *compile_rmoveto(double x, double y) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));
	cff_blob *blob_x = compile_type2_value(x);
	cff_blob *blob_y = compile_type2_value(y);
	cff_blob *blob_o = encode_cs2_operator(op_rmoveto);

	blob_merge(blob, blob_x);
	blob_merge(blob, blob_y);
	blob_merge(blob, blob_o);

	blob_free(blob_x);
	blob_free(blob_y);
	blob_free(blob_o);

	return blob;
}

static cff_blob *_rmoveto(CFF_Outline *outline, uint16_t i) {
	if (i == 0)
		return compile_rmoveto(outline->x[i], outline->y[i]);
	else
		return compile_rmoveto(outline->x[i] - outline->x[i - 1],
		                       outline->y[i] - outline->y[i - 1]);
}

static cff_blob *compile_rlineto(double x, double y) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));
	cff_blob *blob_x = compile_type2_value(x);
	cff_blob *blob_y = compile_type2_value(y);
	cff_blob *blob_o = encode_cs2_operator(op_rlineto);

	blob_merge(blob, blob_x);
	blob_merge(blob, blob_y);
	blob_merge(blob, blob_o);

	blob_free(blob_x);
	blob_free(blob_y);
	blob_free(blob_o);

	return blob;
}

static cff_blob *_rlineto(CFF_Outline *outline, uint16_t i) {
	if (i == 0)
		return compile_rlineto(outline->x[i], outline->y[i]);
	else
		return compile_rlineto(outline->x[i] - outline->x[i - 1],
		                       outline->y[i] - outline->y[i - 1]);
}

static cff_blob *compile_rrcurveto(double x1, double y1, double x2, double y2, double x3,
                                   double y3) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));
	cff_blob *blob_x1 = compile_type2_value(x1);
	cff_blob *blob_y1 = compile_type2_value(y1);
	cff_blob *blob_x2 = compile_type2_value(x2);
	cff_blob *blob_y2 = compile_type2_value(y2);
	cff_blob *blob_x3 = compile_type2_value(x3);
	cff_blob *blob_y3 = compile_type2_value(y3);
	cff_blob *blob_o = encode_cs2_operator(op_rrcurveto);

	blob_merge(blob, blob_x1);
	blob_merge(blob, blob_y1);
	blob_merge(blob, blob_x2);
	blob_merge(blob, blob_y2);
	blob_merge(blob, blob_x3);
	blob_merge(blob, blob_y3);
	blob_merge(blob, blob_o);

	blob_free(blob_x1);
	blob_free(blob_y1);
	blob_free(blob_x2);
	blob_free(blob_y2);
	blob_free(blob_x3);
	blob_free(blob_y3);
	blob_free(blob_o);

	return blob;
}

static cff_blob *_rrcurveto(CFF_Outline *outline, uint16_t i) {
	return compile_rrcurveto(outline->x[i] - outline->x[i - 1], outline->y[i] - outline->y[i - 1],
	                         outline->x[i + 1] - outline->x[i], outline->y[i + 1] - outline->y[i],
	                         outline->x[i + 2] - outline->x[i + 1],
	                         outline->y[i + 2] - outline->y[i + 1]);
}

static cff_blob *compile_endchar(void) { return encode_cs2_operator(op_endchar); }

static uint32_t check_contour(CFF_Outline *outline, uint16_t pos) {
	for (int i = 0; i < outline->cnt_contour; i++) {
		if (outline->c[i] == pos) return 1;
	}

	return 0;
}

static cff_blob *compile_single_glyph(CFF_File *f, uint16_t i) {
	CFF_INDEX *temp_subr = cff_index_init();
	CFF_Stack temp_stack;
	cff_blob *blob_outline;
	CFF_Outline *temp_outline = cff_outline_init();

	temp_stack.index = 0;
	temp_stack.stem = 0;

	if (f->fdselect.t != CFF_FDSELECT_UNSPECED)
		parse_subr(i, f->raw_data, f->font_dict, f->fdselect, temp_subr);
	else
		parse_subr(i, f->raw_data, f->top_dict, f->fdselect, temp_subr);

	parse_outline(f->char_strings.data + f->char_strings.offset[i] - 1,
	              f->char_strings.offset[i + 1] - f->char_strings.offset[i], f->global_subr,
	              *temp_subr, &temp_stack, temp_outline);

	blob_outline = compile_outline(temp_outline);

	cff_index_fini(temp_subr);
	cff_outline_fini(temp_outline);

	return blob_outline;
}

cff_blob *compile_charstring(CFF_File *f) {
	if (f->char_strings.count > 0) {
		cff_blob *final_blob;
		CFF_INDEX *charstring = malloc(sizeof(CFF_INDEX));
		charstring->count = f->char_strings.count;
		charstring->offSize = 4;
		charstring->offset = calloc(f->char_strings.count + 1, sizeof(uint32_t));
		charstring->offset[0] = 1;
		charstring->data = NULL;

		for (uint32_t i = 0; i < f->char_strings.count; i++) {
			cff_blob *blob = compile_single_glyph(f, i);
			charstring->offset[i + 1] = blob->size + charstring->offset[i];
			charstring->data = realloc(charstring->data, charstring->offset[i + 1] - 1);
			memcpy(charstring->data + charstring->offset[i] - 1, blob->data, blob->size);
			blob_free(blob);
		}

		final_blob = compile_index(*charstring);
		cff_index_fini(charstring);

		return final_blob;
	} else {
		return calloc(1, sizeof(cff_blob));
	}
}

cff_blob *compile_outline(CFF_Outline *outline) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));
	uint16_t *len_countour = calloc(outline->cnt_contour, sizeof(uint16_t));

	// glyph width (delta)
	if (outline->width != 0.0) {
		cff_blob *width = compile_type2_value(outline->width);
		blob_merge(blob, width);
		blob_free(width);
	}

	// rmoveto/rlineto/rrcurveto
	for (int i = 0; i < outline->cnt_point; /**/) {
		switch (outline->t[i]) {
			case 1:
				if (check_contour(outline, i)) {
					cff_blob *rmoveto = _rmoveto(outline, i);
					blob_merge(blob, rmoveto);
					blob_free(rmoveto);
				} else {
					cff_blob *rlineto = _rlineto(outline, i);
					blob_merge(blob, rlineto);
					blob_free(rlineto);
				}

				i += 1;
				break;
			case 0: {
				cff_blob *rrcurveto = _rrcurveto(outline, i);
				blob_merge(blob, rrcurveto);
				blob_free(rrcurveto);
			}

				i += 3;
				break;
		}
	}

	// final endchar
	{
		cff_blob *endchar = compile_endchar();
		blob_merge(blob, endchar);
		blob_free(endchar);
	}

	return blob;
}

/*
  SVG related path operations.
    https://www.w3.org/TR/SVG11/paths.html
*/

static cff_blob *__rmoveto(CFF_Outline *outline, uint16_t i) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));
	char buf[256] = {0};

	/*
	  PostScript : '%g %g moveto' / 'stroke %g %g moveto'
	*/

	if (i == 0)
		sprintf(buf, "M %g %g\n", outline->x[i], outline->y[i]);
	else
		sprintf(buf, "Z\nM %g %g\n", outline->x[i], outline->y[i]);

	blob->size = strlen(buf);
	blob->data = strdup(buf);
	return blob;
}

static cff_blob *__rlineto(CFF_Outline *outline, uint16_t i) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));
	char buf[256] = {0};

	/*
	  PostScript : '%g %g lineto'
	*/

	sprintf(buf, "L %g %g\n", outline->x[i], outline->y[i]);

	blob->size = strlen(buf);
	blob->data = strdup(buf);
	return blob;
}

static cff_blob *__rrcurveto(CFF_Outline *outline, uint16_t i) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));
	char buf[256] = {0};

	/*
	  PostScript : '%g %g %g %g %g %g curveto'
	*/

	sprintf(buf, "C %g %g %g %g %g %g\n", outline->x[i], outline->y[i], outline->x[i + 1],
	        outline->y[i + 1], outline->x[i + 2], outline->y[i + 2]);

	blob->size = strlen(buf);
	blob->data = strdup(buf);
	return blob;
}

cff_blob *compile_outline_to_svg(CFF_Outline *outline) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));

	for (int i = 0; i < outline->cnt_point;) {
		if (outline->t[i] == 1) {
			if (check_contour(outline, i)) {
				cff_blob *rmoveto = __rmoveto(outline, i);
				blob_merge(blob, rmoveto);
				blob_free(rmoveto);
			} else {
				cff_blob *rlineto = __rlineto(outline, i);
				blob_merge(blob, rlineto);
				blob_free(rlineto);
			}

			i += 1;
		} else if (outline->t[i] == 0) {
			cff_blob *rrcurveto = __rrcurveto(outline, i);
			blob_merge(blob, rrcurveto);
			blob_free(rrcurveto);

			i += 3;
		}
	}

	{
		blob->data = realloc(blob->data, blob->size + 1);
		blob->data[blob->size] = 0;
	}

	return blob;
}

cff_blob *compile_offset(int32_t val) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));
	blob->size = 5;
	blob->data = calloc(blob->size, sizeof(uint8_t));
	blob->data[0] = 29;
	blob->data[1] = (val >> 24) & 0xff;
	blob->data[2] = (val >> 16) & 0xff;
	blob->data[3] = (val >> 8) & 0xff;
	blob->data[4] = val & 0xff;
	return blob;
}

cff_blob *compile_cff(CFF_File *f) {
	cff_blob *blob = calloc(1, sizeof(cff_blob));

	if (f->font_dict.count != 0 && f->fdselect.t != CFF_FDSELECT_UNSPECED) {
		int32_t delta = 0;
		cff_blob *h = compile_header();
		cff_blob *n = compile_index(f->name);
		cff_blob *t = compile_topdict(f->top_dict);
		cff_blob *i = compile_index(f->string);
		cff_blob *e = compile_encoding(f->encodings);
		cff_blob *c = compile_charset(f->charsets);
		cff_blob *s = compile_charstring(f);
		cff_blob *p = compile_private(f);
		if (e->size != 0) delta += 6;
		if (c->size != 0) delta += 6;
		if (s->size != 0) delta += 6;
		if (p->size != 0) delta += 11;
		blob_merge(blob, h);
		blob_merge(blob, n);
		{
			cff_blob delta_blob;
			int32_t delta_size = t->size + delta + 1;
			delta_blob.size = 11;
			delta_blob.data = calloc(delta_blob.size, sizeof(uint8_t));
			delta_blob.data[0] = 0;
			delta_blob.data[1] = 1;
			delta_blob.data[2] = 4;
			delta_blob.data[6] = 1;
			delta_blob.data[7] = (delta_size >> 24) & 0xff;
			delta_blob.data[8] = (delta_size >> 16) & 0xff;
			delta_blob.data[9] = (delta_size >> 8) & 0xff;
			delta_blob.data[10] = (delta_size)&0xff;
			blob_merge(blob, &delta_blob);
			free(delta_blob.data);
		}
		blob_merge(blob, t);
		if (e->size != 0) {
			int32_t off = h->size + n->size + 11 + t->size + i->size + delta + 3;
			cff_blob *val = compile_offset(off);
			cff_blob *op = encode_cff_operator(op_Encoding);
			blob_merge(blob, val);
			blob_merge(blob, op);
			blob_free(val);
			blob_free(op);
		}
		if (c->size != 0) {
			int32_t off = h->size + n->size + 11 + t->size + i->size + delta + 3 + e->size;
			cff_blob *val = compile_offset(off);
			cff_blob *op = encode_cff_operator(op_charset);
			blob_merge(blob, val);
			blob_merge(blob, op);
			blob_free(val);
			blob_free(op);
		}
		if (s->size != 0) {
			int32_t off =
			    h->size + n->size + 11 + t->size + i->size + delta + 3 + e->size + c->size;
			cff_blob *val = compile_offset(off);
			cff_blob *op = encode_cff_operator(op_CharStrings);
			blob_merge(blob, val);
			blob_merge(blob, op);
			blob_free(val);
			blob_free(op);
		}
		if (p->size != 0) {
			int32_t off = h->size + n->size + 11 + t->size + i->size + delta + 3 + e->size +
			              c->size + s->size;
			cff_blob *len = compile_offset(p->size);
			cff_blob *val = compile_offset(off);
			cff_blob *op = encode_cff_operator(op_Private);
			blob_merge(blob, len);
			blob_merge(blob, val);
			blob_merge(blob, op);
			blob_free(len);
			blob_free(val);
			blob_free(op);
		}
		blob_merge(blob, i);
		{
			cff_blob gsubr;
			gsubr.size = 3;
			gsubr.data = calloc(gsubr.size, sizeof(uint8_t));
			blob_merge(blob, &gsubr);
		}
		blob_merge(blob, e);
		blob_merge(blob, c);
		blob_merge(blob, s);
		blob_merge(blob, p);
		blob_free(h);
		blob_free(n);
		blob_free(t);
		blob_free(i);
		blob_free(e);
		blob_free(c);
		blob_free(s);
		blob_free(p);
	} else {
		int32_t delta = 0;
		cff_blob *h = compile_header();
		cff_blob *n = compile_index(f->name);
		cff_blob *t = compile_topdict(f->top_dict);
		cff_blob *i = compile_index(f->string);
		cff_blob *c = compile_charset(f->charsets);
		cff_blob *e = compile_fdselect(f->fdselect);
		cff_blob *s = compile_charstring(f);
		cff_blob *r = compile_fdarray(f->font_dict);
		if (c->size != 0) delta += 6;
		if (e->size != 0) delta += 7;
		if (s->size != 0) delta += 6;
		if (r->size != 0) delta += 7;
		blob_merge(blob, h);
		blob_merge(blob, n);
		{
			cff_blob delta_blob;
			int32_t delta_size = t->size + delta + 1;
			delta_blob.size = 11;
			delta_blob.data = calloc(delta_blob.size, sizeof(uint8_t));
			delta_blob.data[0] = 0;
			delta_blob.data[1] = 1;
			delta_blob.data[2] = 4;
			delta_blob.data[6] = 1;
			delta_blob.data[7] = (delta_size >> 24) & 0xff;
			delta_blob.data[8] = (delta_size >> 16) & 0xff;
			delta_blob.data[9] = (delta_size >> 8) & 0xff;
			delta_blob.data[10] = (delta_size)&0xff;
			blob_merge(blob, &delta_blob);
			free(delta_blob.data);
		}
		blob_merge(blob, t);
		if (c->size != 0) {
			int32_t off = h->size + n->size + 11 + t->size + i->size + delta + 3;
			cff_blob *val = compile_offset(off);
			cff_blob *op = encode_cff_operator(op_charset);
			blob_merge(blob, val);
			blob_merge(blob, op);
			blob_free(val);
			blob_free(op);
		}
		if (e->size != 0) {
			int32_t off = h->size + n->size + 11 + t->size + i->size + delta + 3 + c->size;
			cff_blob *val = compile_offset(off);
			cff_blob *op = encode_cff_operator(op_FDSelect);
			blob_merge(blob, val);
			blob_merge(blob, op);
			blob_free(val);
			blob_free(op);
		}
		if (s->size != 0) {
			int32_t off =
			    h->size + n->size + 11 + t->size + i->size + delta + 3 + c->size + e->size;
			cff_blob *val = compile_offset(off);
			cff_blob *op = encode_cff_operator(op_CharStrings);
			blob_merge(blob, val);
			blob_merge(blob, op);
			blob_free(val);
			blob_free(op);
		}
		if (r->size != 0) {
			int32_t off = h->size + n->size + 11 + t->size + i->size + delta + 3 + c->size +
			              e->size + s->size;
			cff_blob *val = compile_offset(off);
			cff_blob *op = encode_cff_operator(op_FDArray);
			blob_merge(blob, val);
			blob_merge(blob, op);
			blob_free(val);
			blob_free(op);
		}
		blob_merge(blob, i);
		{
			cff_blob gsubr;
			gsubr.size = 3;
			gsubr.data = calloc(gsubr.size, sizeof(uint8_t));
			blob_merge(blob, &gsubr);
		}
		blob_merge(blob, c);
		blob_merge(blob, e);
		blob_merge(blob, s);
		blob_merge(blob, r);
		blob_free(h);
		blob_free(n);
		blob_free(t);
		blob_free(i);
		blob_free(c);
		blob_free(e);
		blob_free(s);
		blob_free(r);
	}

	return blob;
}
