#include "gsub-ligature.h"

bool consolidate_gsub_ligature(caryll_Font *font, table_OTL *table, otl_Subtable *_subtable, sds lookupName) {
	subtable_gsub_ligature *subtable = &(_subtable->gsub_ligature);
	fontop_consolidateCoverage(font, subtable->to, lookupName);
	for (glyphid_t j = 0; j < subtable->to->numGlyphs; j++) {
		fontop_consolidateCoverage(font, subtable->from[j], lookupName);
		fontop_shrinkCoverage(subtable->from[j], false);
	}
	glyphid_t jj = 0;
	for (glyphid_t k = 0; k < subtable->to->numGlyphs; k++) {
		if (subtable->to->glyphs[k].name && subtable->from[k]->numGlyphs) {
			subtable->to->glyphs[jj] = subtable->to->glyphs[k];
			subtable->from[jj] = subtable->from[k];
			jj++;
		} else {
			otl_delete_Coverage(subtable->from[k]);
		}
	}
	subtable->to->numGlyphs = jj;
	return (subtable->to->numGlyphs==0);
}
