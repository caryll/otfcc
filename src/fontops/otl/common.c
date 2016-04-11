#include "common.h"

static int by_gid(const void *a, const void *b) {
	return ((glyph_handle *)a)->gid - ((glyph_handle *)b)->gid;
}

void consolidate_coverage(caryll_font *font, otl_coverage *coverage, sds lookupName) {
	if (!coverage) return;
	for (uint16_t j = 0; j < coverage->numGlyphs; j++) {
		glyph_order_entry *ordentry;
		HASH_FIND_STR(*font->glyph_order, coverage->glyphs[j].name, ordentry);
		if (ordentry) {
			coverage->glyphs[j].gid = ordentry->gid;
			if (ordentry->name != coverage->glyphs[j].name) sdsfree(coverage->glyphs[j].name);
			coverage->glyphs[j].name = ordentry->name;
		} else {
			fprintf(stderr, "[Consolidate] Ignored missing glyph /%s in lookup %s.\n",
			        coverage->glyphs[j].name, lookupName);
			coverage->glyphs[j].gid = 0;
			DELETE(sdsfree, coverage->glyphs[j].name);
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
			cd->glyphs[j].gid = ordentry->gid;
			if (ordentry->name != cd->glyphs[j].name) sdsfree(cd->glyphs[j].name);
			cd->glyphs[j].name = ordentry->name;
		} else {
			fprintf(stderr, "[Consolidate] Ignored missing glyph /%s in lookup %s.\n",
			        cd->glyphs[j].name, lookupName);
			cd->glyphs[j].gid = 0;
			DELETE(sdsfree, cd->glyphs[j].name);
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
