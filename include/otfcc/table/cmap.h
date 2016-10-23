#ifndef CARYLL_INCLUDE_TABLE_CMAP_H
#define CARYLL_INCLUDE_TABLE_CMAP_H

#include "table-common.h"

// We will support format 0, 4, 12 of CMAP only
typedef struct {
	UT_hash_handle hh;
	int unicode;
	otfcc_GlyphHandle glyph;
} cmap_Entry;
typedef cmap_Entry *table_cmap;

void table_delete_cmap(MOVE table_cmap *table);
table_cmap *table_read_cmap(const caryll_Packet packet);
void table_dump_cmap(const table_cmap *table, json_value *root, const otfcc_Options *options);
table_cmap *table_parse_cmap(const json_value *root, const otfcc_Options *options);
caryll_Buffer *table_build_cmap(const table_cmap *cmap, const otfcc_Options *options);

void table_encodeCmapByIndex(table_cmap *map, int c, uint16_t gid);
void table_encodeCmapByName(table_cmap *map, int c, sds name);
#endif
