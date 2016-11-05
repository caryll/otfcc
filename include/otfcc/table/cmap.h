#ifndef CARYLL_INCLUDE_TABLE_CMAP_H
#define CARYLL_INCLUDE_TABLE_CMAP_H

#include "table-common.h"

// We will support format 0, 4, 12 of CMAP only
typedef struct {
	UT_hash_handle hh;
	int unicode;
	otfcc_GlyphHandle glyph;
} cmap_Entry;

typedef struct { OWNING cmap_Entry *unicodes; } table_cmap;
extern caryll_ElementInterfaceOf(table_cmap) {
	caryll_RT(table_cmap);
	bool (*encodeByIndex)(table_cmap * cmap, int c, uint16_t gid);
	bool (*encodeByName)(table_cmap * cmap, int c, MOVE sds name);
	bool (*unmap)(table_cmap * cmap, int c);
	otfcc_GlyphHandle *(*lookup)(table_cmap * cmap, int c);
}
iTable_cmap;

#endif
