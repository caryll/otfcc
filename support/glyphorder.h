#ifndef CARYLL_SUPPORT_GLYPHORDER_H
#define CARYLL_SUPPORT_GLYPHORDER_H

#include <stdint.h>
#include "../caryll-font.h"
#include "../extern/uthash.h"
#include "../extern/sds.h"

#include "../extern/parson.h"

typedef struct {
	uint16_t gid;
	sds name;
} glyph_handle;

typedef struct {
	int gid;
	sds name;
	UT_hash_handle hh;
} glyph_order_entry;
typedef glyph_order_entry *glyph_order_hash;

void caryll_name_glyphs(caryll_font *font);
void caryll_name_cmap_entries(caryll_font *font);
void caryll_name_glyf(caryll_font *font);

int try_name_glyph(glyph_order_hash *glyph_order, uint16_t _id, sds name);
void delete_glyph_order_map(glyph_order_hash *map);

void caryll_glyphorder_to_json(caryll_font *font, JSON_Object *root);

#endif
