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

static void prepareSpaceForBlobMerge(cff_blob *dst, size_t size) {
	if (dst->free >= size) {
		dst->free -= size;
	} else {
		dst->free = (((dst->size + size) >> 1) + 0x10) & 0xFFFFFF;
		dst->data = realloc(dst->data, dst->size + size + dst->free);
	}
}
void blob_merge_raw(cff_blob *dst, cff_blob *src) {
	prepareSpaceForBlobMerge(dst, src->size);
	memcpy(dst->data + dst->size, src->data, src->size);
	dst->size += src->size;
}
void blob_merge(cff_blob *dst, cff_blob *src) {
	blob_merge_raw(dst, src);
	blob_free(src);
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

cff_blob *compile_charset(CFF_Charset cset) {
	switch (cset.t) {
		case CFF_CHARSET_ISOADOBE:
		case CFF_CHARSET_EXPERT:
		case CFF_CHARSET_EXPERTSUBSET: {
			cff_blob *blob = calloc(1, sizeof(cff_blob));
			return blob;
		}
		case CFF_CHARSET_FORMAT0: {
			cff_blob *blob = calloc(1, sizeof(cff_blob));
			blob->size = 1 + cset.s * 2;
			blob->data = calloc(blob->size, sizeof(uint8_t));
			blob->data[0] = 0;
			for (uint32_t i = 0; i < cset.s; i++)
				blob->data[1 + 2 * i] = cset.f0.glyph[i] / 256,
				                   blob->data[2 + 2 * i] = cset.f0.glyph[i] % 256;
			return blob;
		}
		case CFF_CHARSET_FORMAT1: {
			cff_blob *blob = calloc(1, sizeof(cff_blob));
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
			cff_blob *blob = calloc(1, sizeof(cff_blob));
			blob->size = 1 + cset.s * 4;
			blob->data = calloc(blob->size, sizeof(uint8_t));
			blob->data[0] = 2;
			for (uint32_t i = 0; i < cset.s; i++)
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
			for (uint16_t j = 0; j < fd.s; j++) { blob->data[j] = fd.f0.fds[j]; }
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

void merge_cs2_operator(cff_blob *blob, int32_t val) {
	if (val >= 0x100) {
		prepareSpaceForBlobMerge(blob, 2);
		blob->data[blob->size] = val >> 8;
		blob->data[blob->size + 1] = val & 0xFF;
		blob->size += 2;
	} else {
		prepareSpaceForBlobMerge(blob, 1);
		blob->data[blob->size] = val;
		blob->size += 1;
	}
}
static void merge_cs2_int(cff_blob *blob, int32_t val) {
	if (val >= -1131 && val <= -108) {
		prepareSpaceForBlobMerge(blob, 2);
		blob->data[blob->size + 0] = (uint8_t)((-108 - val) / 256 + 251);
		blob->data[blob->size + 1] = (uint8_t)((-108 - val) % 256);
		blob->size += 2;
	} else if (val >= -107 && val <= 107) {
		prepareSpaceForBlobMerge(blob, 1);
		blob->data[blob->size + 0] = (uint8_t)(val + 139);
		blob->size += 1;
	} else if (val >= 108 && val <= 1131) {
		prepareSpaceForBlobMerge(blob, 2);
		blob->data[blob->size + 0] = (uint8_t)((val - 108) / 256 + 247);
		blob->data[blob->size + 1] = (uint8_t)((val - 108) % 256);
		blob->size += 2;
	} else {
		if (val >= -32768 && val <= 32767) {
			prepareSpaceForBlobMerge(blob, 3);
			blob->data[blob->size + 0] = 28;
			blob->data[blob->size + 1] = (uint8_t)(val >> 8);
			blob->data[blob->size + 2] = (uint8_t)((val << 8) >> 8);
			blob->size += 3;
		} else {
			fprintf(stderr, "Error: Illegal Number (%d) in Type2 CharString.\n", val);
			merge_cs2_int(blob, 0);
		}
	}
}
static void merge_cs2_real(cff_blob *blob, double val) {
	prepareSpaceForBlobMerge(blob, 5);
	int16_t integerPart = floor(val);
	uint16_t fractionPart = (val - integerPart) * 65536.0;
	blob->data[blob->size + 0] = 255;
	blob->data[blob->size + 1] = integerPart >> 8;
	blob->data[blob->size + 2] = integerPart & 0xFF;
	blob->data[blob->size + 3] = fractionPart >> 8;
	blob->data[blob->size + 4] = fractionPart & 0xFF;
	blob->size += 5;
}
void merge_cs2_operand(cff_blob *blob, double val) {
	double intpart;
	if (modf(val, &intpart) == 0.0) {
		merge_cs2_int(blob, intpart);
	} else {
		merge_cs2_real(blob, val);
	}
}
void merge_cs2_special(cff_blob *blob, uint8_t val) {
	prepareSpaceForBlobMerge(blob, 1);
	blob->data[blob->size + 0] = val;
	blob->size += 1;
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
