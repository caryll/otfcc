#ifndef CARYLL_TABLES_VHEA_H
#define CARYLL_TABLES_VHEA_H

#include "../support/util.h"
#include "../caryll-sfnt.h"

typedef struct {
	f16dot16 version;
	int16_t ascent;
	int16_t descent;
	int16_t lineGap;
	int16_t advanceHeightMax;
	int16_t minTop;
	int16_t minBottom;
	int16_t yMaxExtent;
	int16_t caretSlopeRise;
	int16_t caretSlopeRun;
	int16_t caretOffset;
	int16_t dummy0;
	int16_t dummy1;
	int16_t dummy2;
	int16_t dummy3;
	int16_t metricDataFormat;
	uint16_t numOfLongVerMetrics;
} table_vhea;

table_vhea *caryll_new_vhea();
table_vhea *caryll_read_vhea(caryll_packet packet);
void caryll_vhea_to_json(table_vhea *table, json_value *root, caryll_dump_options *dumpopts);
table_vhea *caryll_vhea_from_json(json_value *root, caryll_dump_options *dumpopts);
caryll_buffer *caryll_write_vhea(table_vhea *vhea);
#endif
