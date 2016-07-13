#ifndef CARYLL_TABLES_VMTX_H
#define CARYLL_TABLES_VMTX_H

#include <support/util.h>
#include <font/caryll-sfnt.h>

#include "vhea.h"
#include "maxp.h"

typedef struct {
	uint16_t advanceHeight;
	int16_t tsb;
} vertical_metric;

typedef struct {
	vertical_metric *metrics;
	int16_t *topSideBearing;
} table_vmtx;

table_vmtx *caryll_read_vmtx(caryll_packet packet, table_vhea *vhea, table_maxp *maxp);
void caryll_delete_vmtx(table_vmtx *table);
caryll_buffer *caryll_write_vmtx(table_vmtx *table, uint16_t count_a, uint16_t count_k);

#endif
