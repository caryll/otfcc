#include "gpos-pair.h"

static otl_coverage *covFromCD(otl_classdef *cd) {
	otl_coverage *cov;
	NEW(cov);
	cov->numGlyphs = cd->numGlyphs;
	NEW_N(cov->glyphs, cd->numGlyphs);
	for (uint16_t j = 0; j < cd->numGlyphs; j++) {
		cov->glyphs[j].gid = cd->glyphs[j].gid;
		cov->glyphs[j].name = cd->glyphs[j].name;
	}
	return cov;
}
bool consolidate_gpos_pair(caryll_font *font, table_otl *table, otl_subtable *_subtable,
                           sds lookupName) {
	subtable_gpos_pair *subtable = &(_subtable->gpos_pair);
	consolidate_classdef(font, subtable->first, lookupName);
	consolidate_classdef(font, subtable->second, lookupName);
	subtable->coverage = covFromCD(subtable->first);
	shrink_classdef(subtable->first);
	shrink_classdef(subtable->second);
	shrink_coverage(subtable->coverage, true);
	return false;
}
