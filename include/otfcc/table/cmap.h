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

void otfcc_encodeCmapByIndex(table_cmap *map, int c, uint16_t gid);
void otfcc_encodeCmapByName(table_cmap *map, int c, sds name);
void otfcc_unmapCmap(table_cmap *map, int c);

#endif
