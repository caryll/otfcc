#include "common.h"

static int by_gid(const void *a, const void *b) {
	return ((glyph_handle *)a)->index - ((glyph_handle *)b)->index;
}

void fontop_consolidateCoverage(caryll_Font *font, otl_Coverage *coverage, sds lookupName) {
	if (!coverage) return;
	for (glyphid_t j = 0; j < coverage->numGlyphs; j++) {
		caryll_GlyphOrderEntry *ordentry = caryll_lookupName(font->glyph_order, coverage->glyphs[j].name);
		if (ordentry) {
			handle_consolidateTo(&coverage->glyphs[j], ordentry->gid, ordentry->name);
		} else {
			fprintf(stderr, "[Consolidate] Ignored missing glyph /%s in lookup %s.\n", coverage->glyphs[j].name,
			        lookupName);
			handle_delete(&coverage->glyphs[j]);
		}
	}
}
void fontop_shrinkCoverage(otl_Coverage *coverage, bool dosort) {
	if (!coverage) return;
	glyphid_t k = 0;
	for (glyphid_t j = 0; j < coverage->numGlyphs; j++) {
		if (coverage->glyphs[j].name) {
			coverage->glyphs[k++] = coverage->glyphs[j];
		} else {
			handle_delete(&coverage->glyphs[j]);
		}
	}
	if (dosort) {
		qsort(coverage->glyphs, k, sizeof(glyph_handle), by_gid);
		glyphid_t skip = 0;
		for (glyphid_t rear = 1; rear < coverage->numGlyphs; rear++) {
			if (coverage->glyphs[rear].index == coverage->glyphs[rear - skip - 1].index) {
				handle_delete(&coverage->glyphs[rear]);
				skip += 1;
			} else {
				coverage->glyphs[rear - skip] = coverage->glyphs[rear];
			}
		}
		k -= skip;
	}
	coverage->numGlyphs = k;
}

void fontop_consolidateClassDef(caryll_Font *font, otl_ClassDef *cd, sds lookupName) {
	if (!cd) return;
	for (glyphid_t j = 0; j < cd->numGlyphs; j++) {
		caryll_GlyphOrderEntry *ordentry = caryll_lookupName(font->glyph_order, cd->glyphs[j].name);
		if (ordentry) {
			handle_consolidateTo(&cd->glyphs[j], ordentry->gid, ordentry->name);
		} else {
			fprintf(stderr, "[Consolidate] Ignored missing glyph /%s in lookup %s.\n", cd->glyphs[j].name, lookupName);
			cd->glyphs[j].index = 0;
			handle_delete(&cd->glyphs[j]);
		}
	}
}
void fontop_shrinkClassDef(otl_ClassDef *cd) {
	glyphid_t k = 0;
	for (glyphid_t j = 0; j < cd->numGlyphs; j++) {
		if (cd->glyphs[j].name) {
			cd->glyphs[k] = cd->glyphs[j];
			cd->classes[k] = cd->classes[j];
			k++;
		} else {
			handle_delete(&cd->glyphs[j]);
		}
	}
	cd->numGlyphs = k;
}
