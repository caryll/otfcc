#ifndef CARYLL_SUPPORT_GLYPHORDER_H
#define CARYLL_SUPPORT_GLYPHORDER_H

#include "util.h"
#include "../extern/uthash.h"
#include "../extern/sds.h"
#include "../extern/json-builder.h"

typedef struct {
	int gid;
	sds name;
	uint8_t dump_order_type;
	uint32_t dump_order_entry;
	UT_hash_handle hh;
} glyph_order_entry;
typedef glyph_order_entry *glyph_order_hash;

int try_name_glyph(glyph_order_hash *glyph_order, uint16_t _id, sds name);
void lookup_name(glyph_order_hash *glyph_order, uint16_t _gid, sds *field);
void delete_glyph_order_map(glyph_order_hash *map);

glyph_order_hash *caryll_glyphorder_from_json(json_value *root, caryll_dump_options dumpopts);

static int dump_order_type_glyphorder = 1;
static int dump_order_dotnotdef = 2;
static int dump_order_type_cmap = 3;
static int dump_order_type_glyf = 4;

#endif
