#ifndef CARYLL_INCLUDE_TABLE_HMTX_H
#define CARYLL_INCLUDE_TABLE_HMTX_H

#include "table-common.h"

#include "hhea.h"
#include "maxp.h"

typedef struct {
	length_t advanceWidth;
	pos_t lsb;
} horizontal_metric;

typedef struct {
	// Horizontal metrics
	horizontal_metric *metrics;
	pos_t *leftSideBearing;
} table_hmtx;

#endif
