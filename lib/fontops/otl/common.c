#include "common.h"

void fontop_consolidateCoverage(caryll_Font *font, otl_Coverage *coverage, sds lookupName) {
	if (!coverage) return;
	for (glyphid_t j = 0; j < coverage->numGlyphs; j++) {
		glyph_handle *h = &(coverage->glyphs[j]);
		if (!gord_consolidateHandle(font->glyph_order, h)) {
			fprintf(stderr, "[Consolidate] Ignored missing glyph /%s in lookup %s.\n", h->name, lookupName);
			handle_delete(h);
		}
	}
}

void fontop_consolidateClassDef(caryll_Font *font, otl_ClassDef *cd, sds lookupName) {
	if (!cd) return;
	for (glyphid_t j = 0; j < cd->numGlyphs; j++) {
		glyph_handle *h = &(cd->glyphs[j]);
		if (!gord_consolidateHandle(font->glyph_order, h)) {
			fprintf(stderr, "[Consolidate] Ignored missing glyph /%s in lookup %s.\n", h->name, lookupName);
			handle_delete(h);
		}
	}
}
