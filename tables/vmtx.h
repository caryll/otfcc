#ifndef CARYLL_TABLES_VMTX_H
#define CARYLL_TABLES_VMTX_H

#include "../support/util.h"
#include "../caryll-sfnt.h"

typedef struct {
	uint16_t advanceHeight;
	int16_t topSideBearing;
} vertical_metric;

typedef struct {
	vertical_metric *vmetrics;
	uint16_t *topSideBearing;
} table_vmtx;

#endif
