#ifndef CARYLL_CFF_DICT_H
#define CARYLL_CFF_DICT_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <support/util.h>
#include "cff-value.h"

typedef struct {
	uint32_t op;
	uint32_t cnt;
	CFF_Value *vals;
} CFF_Dict_Entry;

typedef struct {
	uint32_t count;
	CFF_Dict_Entry *ents;
} CFF_Dict;

void cff_delete_dict(CFF_Dict *dict);
void esrap_dict(CFF_Dict *d);
CFF_Dict *parse_dict(uint8_t *data, uint32_t len);

void parse_dict_callback(uint8_t *data, uint32_t len, void *context,
                         void (*callback)(uint32_t op, uint8_t top, CFF_Value *stack, void *context));
CFF_Value parse_dict_key(uint8_t *data, uint32_t len, uint32_t op, uint32_t idx);

caryll_buffer *compile_dict(CFF_Dict *dict);

#endif
