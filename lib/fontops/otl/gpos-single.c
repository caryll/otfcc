#include "gpos-single.h"

typedef struct {
	int fromid;
	sds fromname;
	otl_position_value v;
	UT_hash_handle hh;
} gpos_single_hash;
static int gpos_by_from_id(gpos_single_hash *a, gpos_single_hash *b) {
	return a->fromid - b->fromid;
}

bool consolidate_gpos_single(caryll_font *font, table_otl *table, otl_subtable *_subtable, sds lookupName) {
	subtable_gpos_single *subtable = &(_subtable->gpos_single);
	consolidate_coverage(font, subtable->coverage, lookupName);
	gpos_single_hash *h = NULL;
	for (uint16_t k = 0; k < subtable->coverage->numGlyphs; k++) {
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
		uint16_t j = 0;
		HASH_ITER(hh, h, s, tmp) {
			subtable->coverage->glyphs[j].state = HANDLE_STATE_CONSOLIDATED;
			subtable->coverage->glyphs[j].index = s->fromid;
			subtable->coverage->glyphs[j].name = s->fromname;
			subtable->values[j] = s->v;
			j++;
			HASH_DEL(h, s);
			free(s);
		}
	}
	return (subtable->coverage->numGlyphs == 0);
}
