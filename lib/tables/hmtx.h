#ifndef CARYLL_TABLES_HMTX_H
#define CARYLL_TABLES_HMTX_H

#include <support/util.h>
#include <font/caryll-sfnt.h>

#include "hhea.h"
#include "maxp.h"

typedef struct {
	uint16_t advanceWidth;
	int16_t lsb;
} horizontal_metric;

typedef struct {
	// Horizontal metrics
	horizontal_metric *metrics;
	int16_t *leftSideBearing;
} table_hmtx;

table_hmtx *caryll_read_hmtx(caryll_packet packet, table_hhea *hhea, table_maxp *maxp);
void caryll_delete_hmtx(table_hmtx *table);
caryll_buffer *caryll_write_hmtx(table_hmtx *table, uint16_t count_a, uint16_t count_k, caryll_dump_options *dumpopts);

#endif
