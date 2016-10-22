#ifndef CARYLL_TABLES_HEAD_H
#define CARYLL_TABLES_HEAD_H

#include "support/util.h"
#include "font/caryll-sfnt.h"

typedef struct {
	// Font header
	f16dot16 version;
	uint32_t fontRevison;
	uint32_t checkSumAdjustment;
	uint32_t magicNumber;
	uint16_t flags;
	uint16_t unitsPerEm;
	int64_t created;
	int64_t modified;
	int16_t xMin;
	int16_t yMin;
	int16_t xMax;
	int16_t yMax;
	uint16_t macStyle;
	uint16_t lowestRecPPEM;
	int16_t fontDirectoryHint;
	int16_t indexToLocFormat;
	int16_t glyphDataFormat;
} table_head;

table_head *table_new_head();
table_head *table_read_head(caryll_Packet packet);
void table_dump_head(table_head *table, json_value *root, const caryll_Options *options);
table_head *table_parse_head(json_value *root, const caryll_Options *options);
caryll_buffer *table_build_head(table_head *head, const caryll_Options *options);

#endif
