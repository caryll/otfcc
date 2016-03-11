#pragma once

#include <stdint.h>

typedef struct {
	// Vertical Metrics header
	uint32_t version;
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
	int16_t dummy[4];
	int16_t metricDataFormat;
	uint16_t numOf;
} table_vhea;
