#ifndef CARYLL_TABLES_HMTX_H
#define CARYLL_TABLES_HMTX_H

#include <support/util.h>
#include <font/caryll-sfnt.h>

#include "hhea.h"
#include "maxp.h"

typedef struct {
	metric_t advanceWidth;
	pos_t lsb;
} horizontal_metric;

typedef struct {
	// Horizontal metrics
	horizontal_metric *metrics;
	pos_t *leftSideBearing;
} table_hmtx;

table_hmtx *table_read_hmtx(caryll_Packet packet, table_hhea *hhea, table_maxp *maxp);
void table_delete_hmtx(table_hmtx *table);
caryll_buffer *table_build_hmtx(table_hmtx *table, glyphid_t count_a, glyphid_t count_k, const caryll_Options *options);

#endif
