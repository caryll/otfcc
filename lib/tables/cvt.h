#ifndef CARYLL_TABLES_CVT_H
#define CARYLL_TABLES_CVT_H

#include "font/caryll-sfnt.h"
#include "support/util.h"

typedef struct {
	uint32_t length;
	uint16_t *words;
} table_cvt;
void table_delete_cvt(table_cvt *table);

table_cvt *table_read_cvt(const caryll_Packet packet, uint32_t tag);
void table_dump_cvt(const table_cvt *table, json_value *root, const caryll_Options *options, const char *tag);
table_cvt *table_parse_cvt(const json_value *root, const caryll_Options *options, const char *tag);
caryll_Buffer *table_build_cvt(const table_cvt *table, const caryll_Options *options);

#endif
