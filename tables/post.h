#ifndef CARYLL_TABLES_POST_H
#define CARYLL_TABLES_POST_H

#include <stdint.h>
#include "../caryll-font.h"
#include "../support/glyphorder.h"

#include "../extern/json-builder.h"

typedef struct {
	// PostScript information
	uint32_t version;
	uint32_t italicAngle;
	int16_t underlinePosition;
	int16_t underlineThickness;
	uint32_t isFixedPitch;
	uint32_t minMemType42;
	uint32_t maxMemType42;
	uint32_t minMemType1;
	uint32_t maxMemType1;
	glyph_order_hash *post_name_map;
} table_post;

void caryll_read_post(caryll_font *font, caryll_packet packet);
void caryll_delete_table_post(caryll_font *font);
void caryll_post_to_json(caryll_font *font, json_value *root);

#endif
