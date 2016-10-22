#ifndef CARYLL_TABLES_FPGM_PREP_H
#define CARYLL_TABLES_FPGM_PREP_H

#include "support/util.h"
#include "font/caryll-sfnt.h"
#include "support/ttinstr.h"

typedef struct {
	sds tag;
	uint32_t length;
	uint8_t *bytes;
} table_fpgm_prep;
void table_delete_fpgm_prep(table_fpgm_prep *table);
table_fpgm_prep *table_read_fpgm_prep(const caryll_Packet packet, uint32_t tag);
void table_fpgm_dump_prep(const table_fpgm_prep *table, json_value *root, const caryll_Options *options,
                          const char *tag);
table_fpgm_prep *table_fpgm_parse_prep(const json_value *root, const caryll_Options *options, const char *tag);
caryll_buffer *table_build_fpgm_prep(const table_fpgm_prep *table, const caryll_Options *options);

#endif
