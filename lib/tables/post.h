#ifndef CARYLL_TABLES_POST_H
#define CARYLL_TABLES_POST_H

#include "support/util.h"
#include "font/caryll-sfnt.h"
#include "otfcc/glyph-order.h"

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
	caryll_GlyphOrder *post_name_map;
} table_post;

table_post *table_new_post();
void table_delete_post(MOVE table_post *table);
table_post *table_read_post(const caryll_Packet packet);
void table_dump_post(const table_post *table, json_value *root, const otfcc_Options *options);
table_post *table_parse_post(const json_value *root, const otfcc_Options *options);
caryll_Buffer *table_build_post(const table_post *post, caryll_GlyphOrder *glyphorder, const otfcc_Options *options);

#endif
