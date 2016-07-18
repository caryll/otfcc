#ifndef CARYLL_CFF_CHARSET_H
#define CARYLL_CFF_CHARSET_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <support/util.h>
#include "cff-util.h"
#include "cff-value.h"

enum {
	CFF_CHARSET_ISOADOBE = 0,
	CFF_CHARSET_UNSPECED = 0,
	CFF_CHARSET_EXPERT = 1,
	CFF_CHARSET_EXPERTSUBSET = 2,
	CFF_CHARSET_FORMAT0 = 3,
	CFF_CHARSET_FORMAT1 = 4,
	CFF_CHARSET_FORMAT2 = 5,
};

// CFF Charset Structures
typedef struct {
	uint8_t format;
	uint16_t *glyph;
} charset_f0;

typedef struct {
	uint16_t first;
	uint8_t nleft;
} charset_range1;

typedef struct {
	uint8_t format;
	charset_range1 *range1;
} charset_f1;

typedef struct {
	uint16_t first;
	uint16_t nleft;
} charset_range2;

typedef struct {
	uint8_t format;
	charset_range2 *range2;
} charset_f2;

typedef struct {
	uint32_t t;
	uint32_t s; // size
	union {
		charset_f0 f0;
		charset_f1 f1;
		charset_f2 f2;
	};
} CFF_Charset;

void close_charset(CFF_Charset cset);
void parse_charset(uint8_t *data, int32_t offset, uint16_t nchars, CFF_Charset *charsets);
caryll_buffer *compile_charset(CFF_Charset cset);

#endif
