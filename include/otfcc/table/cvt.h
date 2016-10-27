#ifndef CARYLL_INCLUDE_TABLE_CVT_H
#define CARYLL_INCLUDE_TABLE_CVT_H

#include "table-common.h"

typedef struct {
	uint32_t length;
	uint16_t *words;
} table_cvt;
void table_delete_cvt(table_cvt *table);

table_cvt *table_read_cvt(const caryll_Packet packet, const otfcc_Options *options, uint32_t tag);
void table_dump_cvt(const table_cvt *table, json_value *root, const otfcc_Options *options, const char *tag);
table_cvt *table_parse_cvt(const json_value *root, const otfcc_Options *options, const char *tag);
caryll_Buffer *table_build_cvt(const table_cvt *table, const otfcc_Options *options);

#endif
