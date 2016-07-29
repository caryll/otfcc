#include "cff-charset.h"

void parse_charset(uint8_t *data, int32_t offset, uint16_t nchars, CFF_Charset *charsets) {
	uint32_t i;
	if (offset == CFF_CHARSET_ISOADOBE)
		charsets->t = CFF_CHARSET_ISOADOBE;
	else if (offset == CFF_CHARSET_EXPERT)
		charsets->t = CFF_CHARSET_EXPERT;
	else if (offset == CFF_CHARSET_EXPERTSUBSET)
		charsets->t = CFF_CHARSET_EXPERTSUBSET;
	else {
		// NOTE: gid 1 will always be named as .notdef
		switch (data[offset]) {
			case 0:
				charsets->t = CFF_CHARSET_FORMAT0;
				{
					charsets->s = nchars - 1;
					charsets->f0.glyph = calloc(nchars - 1, sizeof(uint16_t));

					for (i = 0; i < charsets->s; i++)
						charsets->f0.glyph[i] = gu2(data, offset + 1 + i * 2);
				}
				break;
			case 1:
				charsets->t = CFF_CHARSET_FORMAT1;
				{
					uint32_t size;
					uint32_t glyphsEncodedSofar = 1;
					for (i = 0; glyphsEncodedSofar < nchars; i++) {
						glyphsEncodedSofar += 1 + gu1(data, offset + 3 + i * 3);
					}

					size = i;
					charsets->s = size;
					charsets->f1.range1 = calloc(i + 1, sizeof(charset_range1));
					for (i = 0; i < size; i++) {
						charsets->f1.range1[i].first = gu2(data, offset + 1 + i * 3);
						charsets->f1.range1[i].nleft = gu1(data, offset + 3 + i * 3);
					}
				}
				break;
			case 2:
				charsets->t = CFF_CHARSET_FORMAT2;
				{
					uint32_t size;
					uint32_t glyphsEncodedSofar = 1;
					for (i = 0; glyphsEncodedSofar < nchars; i++) {
						glyphsEncodedSofar += 1 + gu2(data, offset + 3 + i * 4);
					}

					size = i;
					charsets->s = size;
					charsets->f2.range2 = calloc(i + 1, sizeof(charset_range2));

					for (i = 0; i < size; i++) {
						charsets->f2.range2[i].first = gu2(data, offset + 1 + i * 4);
						charsets->f2.range2[i].nleft = gu2(data, offset + 3 + i * 4);
					}
				}
				break;
		}
	}
}

caryll_buffer *compile_charset(CFF_Charset cset) {
	switch (cset.t) {
		case CFF_CHARSET_ISOADOBE:
		case CFF_CHARSET_EXPERT:
		case CFF_CHARSET_EXPERTSUBSET: {
			return bufnew();
		}
		case CFF_CHARSET_FORMAT0: {
			caryll_buffer *blob = bufnew();
			blob->size = 1 + cset.s * 2;
			blob->data = calloc(blob->size, sizeof(uint8_t));
			blob->data[0] = 0;
			for (uint32_t i = 0; i < cset.s; i++)
				blob->data[1 + 2 * i] = cset.f0.glyph[i] / 256, blob->data[2 + 2 * i] = cset.f0.glyph[i] % 256;
			blob->cursor = blob->size;
			return blob;
		}
		case CFF_CHARSET_FORMAT1: {
			caryll_buffer *blob = bufnew();
			blob->size = 1 + cset.s * 3;
			blob->data = calloc(blob->size, sizeof(uint8_t));
			blob->data[0] = 1;
			for (uint32_t i = 0; i < cset.s; i++)
				blob->data[1 + 3 * i] = cset.f1.range1[i].first / 256,
				                   blob->data[2 + 3 * i] = cset.f1.range1[i].first % 256,
				                   blob->data[3 + 3 * i] = cset.f1.range1[i].nleft;
			return blob;
		}
		case CFF_CHARSET_FORMAT2: {
			caryll_buffer *blob = bufnew();
			blob->size = 1 + cset.s * 4;
			blob->data = calloc(blob->size, sizeof(uint8_t));
			blob->data[0] = 2;
			for (uint32_t i = 0; i < cset.s; i++)
				blob->data[1 + 4 * i] = cset.f2.range2[i].first / 256,
				                   blob->data[2 + 4 * i] = cset.f2.range2[i].first % 256,
				                   blob->data[3 + 4 * i] = cset.f2.range2[i].nleft / 256,
				                   blob->data[4 + 4 * i] = cset.f2.range2[i].nleft % 256;
			blob->cursor = blob->size;
			return blob;
		}
	}
	return NULL;
}

void close_charset(CFF_Charset cset) {
	switch (cset.t) {
		case CFF_CHARSET_EXPERT:
		case CFF_CHARSET_EXPERTSUBSET:
		case CFF_CHARSET_ISOADOBE:
			break;
		case CFF_CHARSET_FORMAT0:
			if (cset.f0.glyph != NULL) free(cset.f0.glyph);
			break;
		case CFF_CHARSET_FORMAT1:
			if (cset.f1.range1 != NULL) free(cset.f1.range1);
			break;
		case CFF_CHARSET_FORMAT2:
			if (cset.f2.range2 != NULL) free(cset.f2.range2);
			break;
	}
}
