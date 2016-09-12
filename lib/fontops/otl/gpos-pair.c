#include "gpos-pair.h"

static otl_Coverage *covFromCD(otl_ClassDef *cd) {
	otl_Coverage *cov;
	NEW(cov);
	cov->numGlyphs = cd->numGlyphs;
	NEW_N(cov->glyphs, cd->numGlyphs);
	for (uint16_t j = 0; j < cd->numGlyphs; j++) {
		cov->glyphs[j].state = HANDLE_STATE_CONSOLIDATED;
		cov->glyphs[j].index = cd->glyphs[j].index;
		cov->glyphs[j].name = cd->glyphs[j].name;
	}
	return cov;
}
bool consolidate_gpos_pair(caryll_Font *font, table_OTL *table, otl_Subtable *_subtable, sds lookupName) {
	subtable_gpos_pair *subtable = &(_subtable->gpos_pair);
	fontop_consolidateClassDef(font, subtable->first, lookupName);
	fontop_consolidateClassDef(font, subtable->second, lookupName);
	subtable->coverage = covFromCD(subtable->first);
	fontop_shrinkClassDef(subtable->first);
	fontop_shrinkClassDef(subtable->second);
	fontop_shrinkCoverage(subtable->coverage, true);
	return (subtable->coverage->numGlyphs == 0);
}
