#ifndef CARYLL_TABLES_HHEA_H
#define CARYLL_TABLES_HHEA_H

#include "../support/util.h"
#include "../caryll-sfnt.h"

typedef struct {
	// Horizontal header
	uint32_t version;
	int16_t ascender;
	int16_t descender;
	int16_t lineGap;
	uint16_t advanceWithMax;
	int16_t minLeftSideBearing;
	int16_t minRightSideBearing;
	int16_t xMaxExtent;
	int16_t caretSlopeRise;
	int16_t caretSlopeRun;
	int16_t caretOffset;
	int16_t reserved[4];
	int16_t metricDataFormat;
	uint16_t numberOfMetrics;
} table_hhea;

table_hhea *caryll_hhea_new();
table_hhea *caryll_read_hhea(caryll_packet packet);
void caryll_hhea_to_json(table_hhea *table, json_value *root, caryll_dump_options dumpopts);
table_hhea *caryll_hhea_from_json(json_value *root, caryll_dump_options dumpopts);
caryll_buffer *caryll_write_hhea(table_hhea *hhea);

#endif
