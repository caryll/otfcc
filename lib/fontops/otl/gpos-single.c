#include "gpos-single.h"

typedef struct {
	int fromid;
	sds fromname;
	otl_PositionValue v;
	UT_hash_handle hh;
} gpos_single_hash;
static int gpos_by_from_id(gpos_single_hash *a, gpos_single_hash *b) {
	return a->fromid - b->fromid;
}

bool consolidate_gpos_single(caryll_Font *font, table_OTL *table, otl_Subtable *_subtable, sds lookupName) {
	subtable_gpos_single *subtable = &(_subtable->gpos_single);
	fontop_consolidateCoverage(font, subtable->coverage, lookupName);
	gpos_single_hash *h = NULL;
	for (glyphid_t k = 0; k < subtable->coverage->numGlyphs; k++) {
		if (subtable->coverage->glyphs[k].name) {
			gpos_single_hash *s;
			int fromid = subtable->coverage->glyphs[k].index;
			HASH_FIND_INT(h, &fromid, s);
			if (s) {
				fprintf(stderr, "[Consolidate] Double-mapping a glyph in a "
				                "single substitution /%s.\n",
				        subtable->coverage->glyphs[k].name);
			} else {
				NEW(s);
				s->fromid = subtable->coverage->glyphs[k].index;
				s->fromname = subtable->coverage->glyphs[k].name;
				s->v = subtable->values[k];
				HASH_ADD_INT(h, fromid, s);
			}
		}
	}
	HASH_SORT(h, gpos_by_from_id);

	subtable->coverage->numGlyphs = HASH_COUNT(h);
	{
		gpos_single_hash *s, *tmp;
		glyphid_t j = 0;
		HASH_ITER(hh, h, s, tmp) {
			subtable->coverage->glyphs[j] = handle_fromConsolidated(s->fromid, s->fromname);
			subtable->values[j] = s->v;
			j++;
			HASH_DEL(h, s);
			free(s);
		}
	}
	return (subtable->coverage->numGlyphs == 0);
}
