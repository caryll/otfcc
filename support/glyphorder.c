#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

int try_name_glyph(glyph_order_hash *glyph_order, uint16_t _id, sds name) {
	glyph_order_entry *s;
	int id = _id;
	HASH_FIND_INT(*glyph_order, &id, s);
	if (s == NULL) {
		s = malloc(sizeof(glyph_order_entry));
		s->gid = id;
		s->name = name;
		HASH_ADD_INT(*glyph_order, gid, s);
		return 1;
	} else {
		return 0;
	}
}

void caryll_name_glyphs(caryll_font *font) {
	glyph_order_hash *glyph_order = malloc(sizeof(glyph_order_hash));
	*glyph_order = NULL;

	uint16_t numGlyphs = font->glyf->numberGlyphs;
	
	try_name_glyph(glyph_order, 0, sdsnew(".notdef"));
	
	// pass 3: Map to Unicode
	if (font->cmap != NULL) {
		cmap_hash handle = *(font->cmap);
		cmap_hash s, _tmp;
		HASH_ITER(hh, handle, s, _tmp) if (s->gid > 0) {
			sds name = sdscatprintf(sdsempty(), "uni%04X", s->unicode);
			int actuallyNamed = try_name_glyph(glyph_order, s->gid, name);
			if (!actuallyNamed) sdsfree(name);
		}
	}
	// pass 4: Map to GID
	for (uint16_t j = 1; j < numGlyphs; j++) {
		sds name = sdscatfmt(sdsempty(), "glyph%u", j);
		int actuallyNamed = try_name_glyph(glyph_order, j, name);
		if (!actuallyNamed) sdsfree(name);
	}

	font->glyph_order = glyph_order;
}
