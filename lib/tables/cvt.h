#ifndef CARYLL_TABLES_CVT_H
#define CARYLL_TABLES_CVT_H

#include "font/caryll-sfnt.h"
#include "support/util.h"

typedef struct {
	uint32_t length;
	uint16_t *words;
} table_cvt;
table_cvt *table_read_cvt(caryll_Packet packet, uint32_t tag);
void table_delete_cvt(table_cvt *table);
void table_dump_cvt(table_cvt *table, json_value *root, const caryll_Options *options, const char *tag);
table_cvt *table_parse_cvt(json_value *root, const caryll_Options *options, const char *tag);
caryll_buffer *table_build_cvt(table_cvt *table, const caryll_Options *options);

#endif
