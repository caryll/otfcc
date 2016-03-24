#ifndef CARYLL_TABLES_CMAP_H
#define CARYLL_TABLES_CMAP_H

#include "../support/util.h"
#include "../caryll-sfnt.h"

// We will support format 0, 4, 12 of CMAP only
typedef struct {
	int unicode;
	glyph_handle glyph;
	UT_hash_handle hh;
} cmap_entry;
typedef cmap_entry *cmap_hash;

cmap_hash *caryll_read_cmap(caryll_packet packet);
void caryll_delete_cmap(cmap_hash *table);
void caryll_cmap_to_json(cmap_hash *table, json_value *root);
cmap_hash *caryll_cmap_from_json(json_value *root);

#endif
