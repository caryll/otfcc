#ifndef CARYLL_cff_INDEX_H
#define CARYLL_cff_INDEX_H

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
} cff_Index;

extern cff_Index *cff_new_Index(void);
extern void cff_delete_Index(cff_Index *out);
extern void cff_close_Index(cff_Index in);
extern void cff_empty_Index(cff_Index *in);
uint32_t cff_lengthOfIndex(cff_Index i);

void cff_extract_Index(uint8_t *data, uint32_t pos, cff_Index *in);

cff_Index *cff_newIndexByCallback(void *context, uint32_t length, caryll_buffer *(*fn)(void *, uint32_t));

caryll_buffer *cff_build_Index(cff_Index index);

#endif
