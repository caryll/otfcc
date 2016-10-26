#ifndef CARYLL_INCLUDE_TABLE_GASP_H
#define CARYLL_INCLUDE_TABLE_GASP_H

#include "table-common.h"

typedef struct {
	uint16_t rangeMaxPPEM;
	bool dogray;
	bool gridfit;
	bool symmetric_smoothing;
	bool symmetric_gridfit;
} gasp_Record;
typedef struct {
	uint16_t version;
	uint16_t numRanges;
	gasp_Record *records;
} table_gasp;

table_gasp *table_new_gasp();
void table_delete_gasp(table_gasp *table);
table_gasp *table_read_gasp(const caryll_Packet packet);
void table_dump_gasp(const table_gasp *table, json_value *root, const otfcc_Options *options);
table_gasp *table_parse_gasp(const json_value *root, const otfcc_Options *options);
caryll_Buffer *table_build_gasp(const table_gasp *table, const otfcc_Options *options);
#endif
