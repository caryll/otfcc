#ifndef CARYLL_cff_DICT_H
#define CARYLL_cff_DICT_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "support/util.h"
#include "cff-value.h"

typedef struct {
	uint32_t op;
	uint32_t cnt;
	cff_Value *vals;
} cff_DictEntry;

typedef struct {
	uint32_t count;
	cff_DictEntry *ents;
} cff_Dict;

void cff_delete_Dict(cff_Dict *dict);
void cff_close_Dict(cff_Dict *d);
cff_Dict *cff_extract_Dict(uint8_t *data, uint32_t len);

void cff_extract_DictByCallback(uint8_t *data, uint32_t len, void *context,
                         void (*callback)(uint32_t op, uint8_t top, cff_Value *stack, void *context));
cff_Value cff_parseDictKey(uint8_t *data, uint32_t len, uint32_t op, uint32_t idx);

caryll_buffer *cff_build_Dict(cff_Dict *dict);

#endif
