#pragma once

#include <stdint.h>

typedef struct {
	uint16_t advanceHeight;
	int16_t topSideBearing;
} vertical_metric;

typedef struct {
	vertical_metric *vmetrics;
	uint16_t *topSideBearing;
} table_vmtx;
