#ifndef CARYLL_TABLES_GASP_H
#define CARYLL_TABLES_GASP_H

#include "../support/util.h"
#include "../caryll-sfnt.h"

typedef struct {
	uint16_t rangeMaxPPEM;
	bool dogray;
	bool gridfit;
	bool symmetric_smoothing;
	bool symmetric_gridfit;
} gasp_record;
typedef struct {
	uint16_t version;
	uint16_t numRanges;
	gasp_record *records;
} table_gasp;

table_gasp *caryll_new_gasp();
void caryll_delete_gasp(table_gasp *table);
table_gasp *caryll_read_gasp(caryll_packet packet);
void caryll_gasp_to_json(table_gasp *table, json_value *root, caryll_dump_options dumpopts);
table_gasp *caryll_gasp_from_json(json_value *root);
caryll_buffer *caryll_write_gasp(table_gasp *table);
#endif
