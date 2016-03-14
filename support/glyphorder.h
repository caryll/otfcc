#ifndef CARYLL_SUPPORT_GLYPHORDER_H
#define CARYLL_SUPPORT_GLYPHORDER_H

#include <stdint.h>
#include "../caryll-font.h"
#include "../extern/uthash.h"
#include "../extern/sds.h"

typedef struct {
	uint16_t gid;
	sds name;
} glyph_reference;

typedef struct {
	int gid;
	sds name;
	UT_hash_handle hh;
} glyph_order_entry;
typedef glyph_order_entry *glyph_order_hash;

void caryll_name_glyphs(caryll_font *font);
void caryll_name_cmap_entries(caryll_font *font);
void caryll_name_glyf(caryll_font *font);

#endif
