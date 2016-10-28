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

void otfcc_deleteCmap(MOVE table_cmap *table);
table_cmap *otfcc_readCmap(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpCmap(const table_cmap *table, json_value *root, const otfcc_Options *options);
table_cmap *otfcc_parseCmap(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildCmap(const table_cmap *cmap, const otfcc_Options *options);

void table_encodeCmapByIndex(table_cmap *map, int c, uint16_t gid);
void table_encodeCmapByName(table_cmap *map, int c, sds name);
#endif
