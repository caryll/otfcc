#ifndef CARYLL_CFF_INDEX_H
#define CARYLL_CFF_INDEX_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <support/util.h>

#include "cff-util.h"
#include "cff-value.h"

typedef struct {
	uint16_t count;
	uint8_t offSize;
	uint32_t *offset;
	uint8_t *data;
} CFF_Index;

extern CFF_Index *cff_index_init(void);
extern void cff_index_fini(CFF_Index *out);
extern void esrap_index(CFF_Index in);
extern void empty_index(CFF_Index *in);
uint32_t count_index(CFF_Index i);

void parse_index(uint8_t *data, uint32_t pos, CFF_Index *in);

CFF_Index *cff_buildindex_callback(void *context, uint32_t length, caryll_buffer *(*fn)(void *, uint32_t));

caryll_buffer *compile_index(CFF_Index index);

#endif
