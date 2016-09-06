#include "gpos-cursive.h"

typedef struct {
	int fromid;
	sds fromname;
	otl_Anchor enter;
	otl_Anchor exit;
	UT_hash_handle hh;
} gpos_cursive_hash;
static int gpos_cursive_by_from_id(gpos_cursive_hash *a, gpos_cursive_hash *b) {
	return a->fromid - b->fromid;
}
bool consolidate_gpos_cursive(caryll_Font *font, table_OTL *table, otl_Subtable *_subtable, sds lookupName) {
	subtable_gpos_cursive *subtable = &(_subtable->gpos_cursive);
	fontop_consolidateCoverage(font, subtable->coverage, lookupName);
	gpos_cursive_hash *h = NULL;
	for (uint16_t k = 0; k < subtable->coverage->numGlyphs; k++) {
		if (subtable->coverage->glyphs[k].name) {
			gpos_cursive_hash *s;
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
				s->enter = subtable->enter[k];
				s->exit = subtable->exit[k];
				HASH_ADD_INT(h, fromid, s);
			}
		}
	}
	HASH_SORT(h, gpos_cursive_by_from_id);

	subtable->coverage->numGlyphs = HASH_COUNT(h);
	{
		gpos_cursive_hash *s, *tmp;
		uint16_t j = 0;
		HASH_ITER(hh, h, s, tmp) {
			subtable->coverage->glyphs[j].state = HANDLE_STATE_CONSOLIDATED;
			subtable->coverage->glyphs[j].index = s->fromid;
			subtable->coverage->glyphs[j].name = s->fromname;
			subtable->enter[j] = s->enter;
			subtable->exit[j] = s->exit;
			j++;
			HASH_DEL(h, s);
			free(s);
		}
	}
	return (subtable->coverage->numGlyphs == 0);
}
