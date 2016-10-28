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

void otfcc_deleteTablehmtx(table_hmtx *table);
table_hmtx *otfcc_readTablehmtx(const otfcc_Packet packet, const otfcc_Options *options, table_hhea *hhea,
                            table_maxp *maxp);
caryll_Buffer *otfcc_buildTablehmtx(const table_hmtx *table, glyphid_t count_a, glyphid_t count_k,
                                const otfcc_Options *options);

#endif
