#include "common.h"

static int by_gid(const void *a, const void *b) {
	return ((glyph_handle *)a)->index - ((glyph_handle *)b)->index;
}

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
		glyph_handle *h = &(cd->glyphs[j]);
		if (!gord_consolidateHandle(font->glyph_order, h)) {
			fprintf(stderr, "[Consolidate] Ignored missing glyph /%s in lookup %s.\n", h->name, lookupName);
			handle_delete(h);
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
