#ifndef CARYLL_SUPPORT_GLYPHORDER_H
#define CARYLL_SUPPORT_GLYPHORDER_H

#include "util.h"

typedef struct {
	int gid;
	sds name;
	uint8_t dump_order_type;
	uint32_t dump_order_entry;
	UT_hash_handle hh;
} glyphorder_Entry;
typedef glyphorder_Entry *glyphorder_Map;

int glyphorder_tryAssignName(glyphorder_Map *glyph_order, int _id, sds name);
void glyphorder_nameAnIndex(glyphorder_Map *glyph_order, int _gid, sds *field);
void glyphorder_nameAIndexedHandle(glyphorder_Map *glyph_order, glyph_handle *h);
void glyphorder_deleteMap(glyphorder_Map *map);

glyphorder_Map *parse_glyphorder(json_value *root, caryll_Options *options);

#endif
