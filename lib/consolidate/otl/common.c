#include "common.h"

void fontop_consolidateCoverage(otfcc_Font *font, otl_Coverage *coverage, const otfcc_Options *options) {
	if (!coverage) return;
	for (glyphid_t j = 0; j < coverage->numGlyphs; j++) {
		glyph_handle *h = &(coverage->glyphs[j]);
		if (!otfcc_gordConsolidateHandle(font->glyph_order, h)) {
			logWarning("[Consolidate] Ignored missing glyph /%s.\n", h->name);
			handle_dispose(h);
		}
	}
}

void fontop_consolidateClassDef(otfcc_Font *font, otl_ClassDef *cd, const otfcc_Options *options) {
	if (!cd) return;
	for (glyphid_t j = 0; j < cd->numGlyphs; j++) {
		glyph_handle *h = &(cd->glyphs[j]);
		if (!otfcc_gordConsolidateHandle(font->glyph_order, h)) {
			logWarning("[Consolidate] Ignored missing glyph /%s.\n", h->name);
			handle_dispose(h);
		}
	}
}
