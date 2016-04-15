#ifndef CARYLL_TABLES_POST_H
#define CARYLL_TABLES_POST_H

#include <support/util.h>
#include <font/caryll-sfnt.h>
#include <support/glyphorder.h>


typedef struct {
	// PostScript information
	f16dot16 version;
	f16dot16 italicAngle;
	int16_t underlinePosition;
	int16_t underlineThickness;
	uint32_t isFixedPitch;
	uint32_t minMemType42;
	uint32_t maxMemType42;
	uint32_t minMemType1;
	uint32_t maxMemType1;
	glyph_order_hash *post_name_map;
} table_post;

table_post *caryll_new_post();
table_post *caryll_read_post(caryll_packet packet);
void caryll_delete_post(table_post *table);
void caryll_post_to_json(table_post *table, json_value *root, caryll_dump_options *dumpopts);
table_post *caryll_post_from_json(json_value *root, caryll_dump_options *dumpopts);
caryll_buffer *caryll_write_post(table_post *post, glyph_order_hash *glyphorder);

#endif
