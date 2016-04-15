#ifndef CARYLL_TABLES_FPGM_PREP_H
#define CARYLL_TABLES_FPGM_PREP_H

#include <support/util.h>
#include <font/caryll-sfnt.h>
#include <support/glyphorder.h>

typedef struct {
	uint32_t length;
	uint8_t *bytes;
} table_fpgm_prep;
table_fpgm_prep *caryll_read_fpgm_prep(caryll_packet packet, uint32_t tag);
void caryll_delete_fpgm_prep(table_fpgm_prep *table);
void caryll_fpgm_prep_to_json(table_fpgm_prep *table, json_value *root,
                              caryll_dump_options *dumpopts, const char *tag);
table_fpgm_prep *caryll_fpgm_prep_from_json(json_value *root, const char *tag);
caryll_buffer *caryll_write_fpgm_prep(table_fpgm_prep *table);

#endif
