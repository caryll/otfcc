#ifndef CARYLL_TABLES_CVT_H
#define CARYLL_TABLES_CVT_H

#include <font/caryll-sfnt.h>
#include <support/glyphorder.h>
#include <support/util.h>

typedef struct {
	uint32_t length;
	uint16_t *words;
} table_cvt;
table_cvt *caryll_read_cvt(caryll_packet packet, uint32_t tag);
void caryll_delete_cvt(table_cvt *table);
void caryll_cvt_to_json(table_cvt *table, json_value *root, caryll_dump_options *dumpopts, const char *tag);
table_cvt *caryll_cvt_from_json(json_value *root, const char *tag);
caryll_buffer *caryll_write_cvt(table_cvt *table, caryll_dump_options *dumpopts);

#endif
