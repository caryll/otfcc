#include "gsub-ligature.h"

bool consolidate_gsub_ligature(caryll_font *font, table_otl *table, otl_subtable *_subtable, sds lookupName) {
	subtable_gsub_ligature *subtable = &(_subtable->gsub_ligature);
	consolidate_coverage(font, subtable->to, lookupName);
	for (uint16_t j = 0; j < subtable->to->numGlyphs; j++) {
		consolidate_coverage(font, subtable->from[j], lookupName);
		shrink_coverage(subtable->from[j], false);
	}
	uint16_t jj = 0;
	for (uint16_t k = 0; k < subtable->to->numGlyphs; k++) {
		if (subtable->to->glyphs[k].name && subtable->from[k]->numGlyphs) {
			subtable->to->glyphs[jj] = subtable->to->glyphs[k];
			subtable->from[jj] = subtable->from[k];
			jj++;
		} else {
			caryll_delete_coverage(subtable->from[k]);
		}
	}
	subtable->to->numGlyphs = jj;
	return (subtable->to->numGlyphs==0);
}
