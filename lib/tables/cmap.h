#ifndef CARYLL_TABLES_CMAP_H
#define CARYLL_TABLES_CMAP_H

#include <support/util.h>
#include <font/caryll-sfnt.h>

// We will support format 0, 4, 12 of CMAP only
typedef struct {
	UT_hash_handle hh;
	int unicode;
	glyph_handle glyph;
} cmap_entry;
typedef cmap_entry *cmap_hash;

cmap_hash *caryll_read_cmap(caryll_packet packet);
void caryll_delete_cmap(cmap_hash *table);
void caryll_cmap_to_json(cmap_hash *table, json_value *root, const caryll_options *options);
cmap_hash *caryll_cmap_from_json(json_value *root, const caryll_options *options);
caryll_buffer *caryll_write_cmap(cmap_hash *cmap, const caryll_options *options);
#endif
