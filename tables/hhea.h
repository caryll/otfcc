#ifndef CARYLL_TABLES_HHEA_H
#define CARYLL_TABLES_HHEA_H

#include <stdint.h>
#include "../caryll-font.h"

#include "../extern/json-builder.h"

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

void caryll_read_hhea(caryll_font *font, caryll_packet packet);
void caryll_hhea_to_json(caryll_font *font, json_value *root);

#endif
