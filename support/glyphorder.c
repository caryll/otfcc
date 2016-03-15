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

	// pass 2: Map to custom `post` names
	if (font->post != NULL && font->post->post_name_map != NULL) {
		glyph_order_entry *s, *_tmp;
		HASH_ITER(hh, *font->post->post_name_map, s, _tmp){
			try_name_glyph(glyph_order, s->gid, sdsdup(s->name));
		}
	}
	// pass 3: Map to Unicode
	if (font->cmap != NULL) {
		cmap_hash handle = *(font->cmap);
		cmap_hash s, _tmp;
		HASH_ITER(hh, handle, s, _tmp) if (s->glyph.gid > 0) {
			sds name = sdscatprintf(sdsempty(), "uni%04X", s->unicode);
			int actuallyNamed = try_name_glyph(glyph_order, s->glyph.gid, name);
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
void lookup_name(caryll_font *font, uint16_t _gid, sds *field) {
	glyph_order_entry *so;
	int gid = _gid;
	HASH_FIND_INT(*font->glyph_order, &gid, so);
	if (so != NULL) { *field = so->name; }
}
void caryll_name_cmap_entries(caryll_font *font) {
	if (font->glyph_order != NULL && font->cmap != NULL) {
		cmap_hash handle = *(font->cmap);
		cmap_hash s, _tmp;
		HASH_ITER(hh, handle, s, _tmp) { lookup_name(font, s->glyph.gid, &s->glyph.name); }
	}
}
void caryll_name_glyf(caryll_font *font) {
	if (font->glyph_order != NULL && font->glyf != NULL) {
		for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
			glyf_glyph *g = font->glyf->glyphs[j];
			lookup_name(font, j, &g->name);
			if (g->numberOfReferences > 0 && g->references != NULL) {
				for (uint16_t k = 0; k < g->numberOfReferences; k++) {
					lookup_name(font, g->references[k].glyph.gid, &g->references[k].glyph.name);
				}
			}
		}
	}
}
