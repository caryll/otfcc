#ifndef CARYLL_TABLES_CMAP_H
#define CARYLL_TABLES_CMAP_H

#include <support/util.h>
#include <font/caryll-sfnt.h>

// We will support format 0, 4, 12 of CMAP only
typedef struct {
	UT_hash_handle hh;
	int unicode;
	glyph_handle glyph;
} cmap_Entry;
typedef cmap_Entry *table_cmap;

table_cmap *table_read_cmap(caryll_Packet packet);
void table_delete_cmap(table_cmap *table);
void table_dump_cmap(table_cmap *table, json_value *root, const caryll_Options *options);
table_cmap *table_parse_cmap(json_value *root, const caryll_Options *options);
caryll_buffer *table_build_cmap(table_cmap *cmap, const caryll_Options *options);
#endif
