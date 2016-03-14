#pragma once

#include <stdint.h>
#include "../caryll-font.h"
#include "../extern/uthash.h"

// We will support format 0, 4, 12 of CMAP only
typedef struct {
	int unicode;
	uint16_t glyph;
	UT_hash_handle hh;
} cmap_entry;
typedef cmap_entry *cmap_hash;

void caryll_read_cmap(caryll_font *font, caryll_packet packet);
