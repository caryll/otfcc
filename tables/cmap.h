#ifndef CARYLL_TABLES_CMAP_H
#define CARYLL_TABLES_CMAP_H

#include <stdint.h>
#include "../caryll-font.h"
#include "../extern/uthash.h"
#include "../support/glyphorder.h"

#include "../extern/parson.h"

// We will support format 0, 4, 12 of CMAP only
typedef struct {
	int unicode;
	glyph_handle glyph;
	UT_hash_handle hh;
} cmap_entry;
typedef cmap_entry *cmap_hash;

void caryll_read_cmap(caryll_font *font, caryll_packet packet);
void caryll_delete_table_cmap(caryll_font *font);
void caryll_cmap_to_json(caryll_font *font, JSON_Object *root);

#endif
