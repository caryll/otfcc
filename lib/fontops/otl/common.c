#include "common.h"

static int by_gid(const void *a, const void *b) {
	return ((glyph_handle *)a)->index - ((glyph_handle *)b)->index;
}

void consolidate_coverage(caryll_font *font, otl_coverage *coverage, sds lookupName) {
	if (!coverage) return;
	for (uint16_t j = 0; j < coverage->numGlyphs; j++) {
		glyph_order_entry *ordentry;
		HASH_FIND_STR(*font->glyph_order, coverage->glyphs[j].name, ordentry);
		if (ordentry) {
			handle_consolidate_to(&coverage->glyphs[j], ordentry->gid, ordentry->name);
		} else {
			fprintf(stderr, "[Consolidate] Ignored missing glyph /%s in lookup %s.\n", coverage->glyphs[j].name,
			        lookupName);
			handle_delete(&coverage->glyphs[j]);
		}
	}
}
void shrink_coverage(otl_coverage *coverage, bool dosort) {
	if (!coverage) return;
	uint16_t k = 0;
	for (uint16_t j = 0; j < coverage->numGlyphs; j++) {
		if (coverage->glyphs[j].name) coverage->glyphs[k++] = coverage->glyphs[j];
	}
	if (dosort) { qsort(coverage->glyphs, k, sizeof(glyph_handle), by_gid); }
	coverage->numGlyphs = k;
}

void consolidate_classdef(caryll_font *font, otl_classdef *cd, sds lookupName) {
	if (!cd) return;
	for (uint16_t j = 0; j < cd->numGlyphs; j++) {
		glyph_order_entry *ordentry;
		HASH_FIND_STR(*font->glyph_order, cd->glyphs[j].name, ordentry);
		if (ordentry) {
			handle_consolidate_to(&cd->glyphs[j], ordentry->gid, ordentry->name);

		} else {
			fprintf(stderr, "[Consolidate] Ignored missing glyph /%s in lookup %s.\n", cd->glyphs[j].name, lookupName);
			cd->glyphs[j].index = 0;
			handle_delete(&cd->glyphs[j]);
		}
	}
}
void shrink_classdef(otl_classdef *cd) {
	uint16_t k = 0;
	for (uint16_t j = 0; j < cd->numGlyphs; j++) {
		if (cd->glyphs[j].name) {
			cd->glyphs[k] = cd->glyphs[j];
			cd->classes[k] = cd->classes[j];
			k++;
		}
	}
	cd->numGlyphs = k;
}
