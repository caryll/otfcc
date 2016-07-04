#include "CFF.h"

caryll_cff_parse_result caryll_read_CFF_and_glyf(caryll_packet packet, uint32_t tag) {
	caryll_cff_parse_result ret;
	ret.meta = NULL;
	ret.glyphs = NULL;
	return ret;
}

void caryll_delete_CFF(table_CFF *table) {
	if (!table) return;
}

void caryll_CFF_to_json(table_CFF *table, json_value *root, caryll_dump_options *dumpopts,
                        const char *tag) {
	if (!table) return;
}

table_CFF *caryll_CFF_from_json(json_value *root, caryll_dump_options *dumpopts) { return NULL; }

caryll_buffer *caryll_write_CFF(caryll_cff_parse_result cffAndGlyf) { return NULL; }
