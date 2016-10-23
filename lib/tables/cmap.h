#ifndef CARYLL_TABLES_CMAP_H
#define CARYLL_TABLES_CMAP_H

#include "support/util.h"
#include "font/caryll-sfnt.h"

// We will support format 0, 4, 12 of CMAP only
typedef struct {
	UT_hash_handle hh;
	int unicode;
	glyph_handle glyph;
} cmap_Entry;
typedef cmap_Entry *table_cmap;

void table_delete_cmap(MOVE table_cmap *table);
table_cmap *table_read_cmap(const caryll_Packet packet);
void table_dump_cmap(const table_cmap *table, json_value *root, const caryll_Options *options);
table_cmap *table_parse_cmap(const json_value *root, const caryll_Options *options);
caryll_buffer *table_build_cmap(const table_cmap *cmap, const caryll_Options *options);

void table_encodeCmapByIndex(table_cmap *map, int c, uint16_t gid);
void table_encodeCmapByName(table_cmap *map, int c, sds name);
#endif
