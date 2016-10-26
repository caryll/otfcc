#ifndef CARYLL_INCLUDE_TABLE_BASE_H
#define CARYLL_INCLUDE_TABLE_BASE_H

#include "otl.h"

typedef struct {
	uint32_t tag;
	pos_t coordinate;
} otl_BaseValue;

typedef struct {
	uint32_t tag;
	uint32_t defaultBaselineTag;
	tableid_t baseValuesCount;
	otl_BaseValue *baseValues;
} otl_BaseScriptEntry;

typedef struct {
	tableid_t scriptCount;
	otl_BaseScriptEntry *entries;
} otl_BaseAxis;

typedef struct {
	otl_BaseAxis *horizontal;
	otl_BaseAxis *vertical;
} table_BASE;

void table_delete_BASE(MOVE table_BASE *base);
table_BASE *table_read_BASE(const caryll_Packet packet);
void table_dump_BASE(const table_BASE *base, json_value *root, const otfcc_Options *options);
table_BASE *table_parse_BASE(const json_value *root, const otfcc_Options *options);
caryll_Buffer *table_build_BASE(const table_BASE *base, const otfcc_Options *options);

#endif
