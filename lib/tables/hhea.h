#ifndef CARYLL_TABLES_HHEA_H
#define CARYLL_TABLES_HHEA_H

#include <support/util.h>
#include <font/caryll-sfnt.h>

typedef struct {
	// Horizontal header
	f16dot16 version;
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

table_hhea *caryll_new_hhea();
table_hhea *caryll_read_hhea(caryll_packet packet);
void caryll_hhea_to_json(table_hhea *table, json_value *root, caryll_options *options);
table_hhea *caryll_hhea_from_json(json_value *root, caryll_options *options);
caryll_buffer *caryll_write_hhea(table_hhea *hhea, caryll_options *options);

#endif
