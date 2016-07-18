#ifndef CARYLL_CFF_FDSELECT_H
#define CARYLL_CFF_FDSELECT_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <support/util.h>
#include "cff-util.h"
#include "cff-value.h"

enum {
	CFF_FDSELECT_FORMAT0,
	CFF_FDSELECT_FORMAT3,
	CFF_FDSELECT_UNSPECED,
};

typedef struct {
	uint8_t format;
	uint8_t *fds;
} fdselect_f0;

typedef struct {
	uint16_t first;
	uint8_t fd;
} fdselect_range3;

typedef struct {
	uint8_t format;
	uint16_t nranges;
	fdselect_range3 *range3;
	uint16_t sentinel;
} fdselect_f3;

typedef struct {
	uint32_t t;
	uint32_t s;
	union {
		fdselect_f0 f0;
		fdselect_f3 f3;
	};
} CFF_FDSelect;

void close_fdselect(CFF_FDSelect fds);
caryll_buffer *compile_fdselect(CFF_FDSelect fd);
void parse_fdselect(uint8_t *data, int32_t offset, uint16_t nchars, CFF_FDSelect *fdselect);

#endif
