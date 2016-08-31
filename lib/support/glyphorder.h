#ifndef CARYLL_SUPPORT_GLYPHORDER_H
#define CARYLL_SUPPORT_GLYPHORDER_H

#include "util.h"

typedef struct {
	int gid;
	sds name;
	uint8_t dump_order_type;
	uint32_t dump_order_entry;
	UT_hash_handle hh;
} glyph_order_entry;
typedef glyph_order_entry *glyph_order_hash;

int try_name_glyph(glyph_order_hash *glyph_order, int _id, sds name);
void lookup_name(glyph_order_hash *glyph_order, int _gid, sds *field);
void lookup_name_handle(glyph_order_hash *glyph_order, glyph_handle *h);
void delete_glyph_order_map(glyph_order_hash *map);

glyph_order_hash *caryll_glyphorder_from_json(json_value *root, caryll_options *options);

#endif
