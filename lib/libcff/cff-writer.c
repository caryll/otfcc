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

#include "libcff.h"

caryll_buffer *compile_header(void) { return bufninit(4, 1, 0, 4, 4); }

caryll_buffer *compile_dict(CFF_Dict *dict) {
	caryll_buffer *blob = bufnew();
	for (uint32_t i = 0; i < dict->count; i++) {
		for (uint32_t j = 0; j < dict->ents[i].cnt; j++) {
			caryll_buffer *blob_val;
			if (dict->ents[i].vals[j].t == CFF_INTEGER) {
				blob_val = encode_cff_number(dict->ents[i].vals[j].i);
			} else if (dict->ents[i].vals[j].t == CFF_DOUBLE) {
				blob_val = encode_cff_real(dict->ents[i].vals[j].d);
			} else {
				blob_val = encode_cff_number(0);
			}
			bufwrite_bufdel(blob, blob_val);
		}
		bufwrite_bufdel(blob, encode_cff_operator(dict->ents[i].op));
	}
	return blob;
}

CFF_INDEX *cff_buildindex_callback(void *context, uint32_t length,
                                   caryll_buffer *(*fn)(void *, uint32_t)) {
	CFF_INDEX *idx = cff_index_init();
	idx->count = length;
	NEW_N(idx->offset, idx->count + 1);
	idx->offset[0] = 1;
	idx->data = NULL;

	size_t used = 0;
	size_t blank = 0;
	for (uint32_t i = 0; i < length; i++) {
		caryll_buffer *blob = fn(context, i);
		if (blank < blob->size) {
			used += blob->size;
			blank = (used >> 1) & 0xFFFFFF;
			idx->data = realloc(idx->data, sizeof(uint8_t) * (used + blank));
		} else {
			used += blob->size;
			blank -= blob->size;
		}
		idx->offset[i + 1] = (uint32_t)(blob->size + idx->offset[i]);
		memcpy(idx->data + idx->offset[i] - 1, blob->data, blob->size);
		buffree(blob);
	}
	idx->offSize = 4;
	return idx;
}

caryll_buffer *compile_index(CFF_INDEX index) {
	caryll_buffer *blob = bufnew();
	uint32_t i;

	uint32_t lastOffset = index.offset[index.count];
	if (lastOffset < 0x100) {
		index.offSize = 1;
	} else if (lastOffset < 0x10000) {
		index.offSize = 2;
	} else if (lastOffset < 0x1000000) {
		index.offSize = 3;
	} else {
		index.offSize = 4;
	}

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
	blob->cursor = blob->size;
	return blob;
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
				blob->data[1 + 2 * i] = cset.f0.glyph[i] / 256,
				                   blob->data[2 + 2 * i] = cset.f0.glyph[i] % 256;
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

caryll_buffer *compile_fdselect(CFF_FDSelect fd) {
	switch (fd.t) {
		case CFF_FDSELECT_UNSPECED: {
			return bufnew();
		}
		case CFF_FDSELECT_FORMAT0: {
			caryll_buffer *blob = bufnew();
			blob->size = 1 + fd.s;
			blob->data = calloc(blob->size, sizeof(uint8_t));
			for (uint16_t j = 0; j < fd.s; j++) { blob->data[j] = fd.f0.fds[j]; }
			return blob;
		}
		case CFF_FDSELECT_FORMAT3: {
			caryll_buffer *blob = bufnew();
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
		default: { return NULL; }
	}
}

void merge_cs2_operator(caryll_buffer *blob, int32_t val) {
	if (val >= 0x100) {
		bufnwrite8(blob, 2, val >> 8, val & 0xff);
	} else {
		bufnwrite8(blob, 1, val & 0xff);
	}
}
static void merge_cs2_int(caryll_buffer *blob, int32_t val) {
	if (val >= -1131 && val <= -108) {
		bufnwrite8(blob, 2, (uint8_t)((-108 - val) / 256 + 251), (uint8_t)((-108 - val) % 256));
	} else if (val >= -107 && val <= 107) {
		bufnwrite8(blob, 1, (uint8_t)(val + 139));
	} else if (val >= 108 && val <= 1131) {
		bufnwrite8(blob, 2, (uint8_t)((val - 108) / 256 + 247), (uint8_t)((val - 108) % 256));
	} else {
		if (val >= -32768 && val <= 32767) {
			bufnwrite8(blob, 3, 28, (uint8_t)(val >> 8), (uint8_t)((val << 8) >> 8));
		} else {
			fprintf(stderr, "Error: Illegal Number (%d) in Type2 CharString.\n", val);
			merge_cs2_int(blob, 0);
		}
	}
}
static void merge_cs2_real(caryll_buffer *blob, double val) {
	int16_t integerPart = floor(val);
	uint16_t fractionPart = (val - integerPart) * 65536.0;
	bufnwrite8(blob, 5, 0xFF, integerPart >> 8, integerPart & 0xFF, fractionPart >> 8,
	           fractionPart & 0xFF);
}
void merge_cs2_operand(caryll_buffer *blob, double val) {
	double intpart;
	if (modf(val, &intpart) == 0.0) {
		merge_cs2_int(blob, intpart);
	} else {
		merge_cs2_real(blob, val);
	}
}
void merge_cs2_special(caryll_buffer *blob, uint8_t val) { bufwrite8(blob, val); }

caryll_buffer *compile_offset(int32_t val) {
	return bufninit(5, 29, (val >> 24) & 0xff, (val >> 16) & 0xff, (val >> 8) & 0xff, val & 0xff);
}
