#include "gpos-pair.h"

bool consolidate_gpos_pair(caryll_Font *font, table_OTL *table, otl_Subtable *_subtable, const otfcc_Options *options) {
	subtable_gpos_pair *subtable = &(_subtable->gpos_pair);
	fontop_consolidateClassDef(font, subtable->first, options);
	fontop_consolidateClassDef(font, subtable->second, options);
	fontop_shrinkClassDef(subtable->first);
	fontop_shrinkClassDef(subtable->second);
	return (subtable->first->numGlyphs == 0);
}
