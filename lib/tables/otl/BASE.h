#ifndef CARYLL_TABLES_OTL_BASE_H
#define CARYLL_TABLES_OTL_BASE_H

#include "otl.h"

typedef struct {
	uint32_t tag;
	int16_t coordinate;
} base_value;

typedef struct {
	uint32_t tag;
	uint32_t defaultBaselineTag;
	uint16_t baseValuesCount;
	base_value *baseValues;
} base_script_entry;

typedef struct {
	uint16_t scriptCount;
	base_script_entry *entries;
} base_axis;

typedef struct {
	base_axis *horizontal;
	base_axis *vertical;
} table_BASE;

void caryll_delete_BASE(table_BASE *base);
table_BASE *caryll_read_BASE(caryll_packet packet);
void caryll_BASE_to_json(table_BASE *base, json_value *root, const caryll_options *options);
table_BASE *caryll_BASE_from_json(json_value *root, const caryll_options *options);
caryll_buffer *caryll_write_BASE(table_BASE *base, const caryll_options *options);

#endif
