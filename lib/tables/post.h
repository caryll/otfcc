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
	glyphorder_Map *post_name_map;
} table_post;

table_post *table_new_post();
table_post *table_read_post(caryll_Packet packet);
void table_delete_post(table_post *table);
void table_dump_post(table_post *table, json_value *root, const caryll_Options *options);
table_post *table_parse_post(json_value *root, const caryll_Options *options);
caryll_buffer *table_build_post(table_post *post, glyphorder_Map *glyphorder, const caryll_Options *options);

#endif
