#include "gsub-multi.h"

typedef struct {
	int fromid;
	sds fromname;
	otl_Coverage *to;
	UT_hash_handle hh;
} gsub_multi_hash;
static int by_from_id_multi(gsub_multi_hash *a, gsub_multi_hash *b) {
	return a->fromid - b->fromid;
}

bool consolidate_gsub_multi(caryll_Font *font, table_OTL *table, otl_Subtable *_subtable, sds lookupName) {
	subtable_gsub_multi *subtable = &(_subtable->gsub_multi);
	fontop_consolidateCoverage(font, subtable->from, lookupName);
	for (uint16_t j = 0; j < subtable->from->numGlyphs; j++) {
		fontop_consolidateCoverage(font, subtable->to[j], lookupName);
		fontop_shrinkCoverage(subtable->to[j], false);
	}
	gsub_multi_hash *h = NULL;
	for (uint16_t k = 0; k < subtable->from->numGlyphs; k++) {
		if (subtable->from->glyphs[k].name) {
			gsub_multi_hash *s;
			int fromid = subtable->from->glyphs[k].index;
			HASH_FIND_INT(h, &fromid, s);
			if (!s) {
				NEW(s);
				s->fromid = subtable->from->glyphs[k].index;
				s->fromname = subtable->from->glyphs[k].name;
				s->to = subtable->to[k];
				HASH_ADD_INT(h, fromid, s);
			} else {
				otl_delete_Coverage(subtable->to[k]);
			}
		} else {
			otl_delete_Coverage(subtable->to[k]);
		}
	}
	HASH_SORT(h, by_from_id_multi);
	subtable->from->numGlyphs = HASH_COUNT(h);
	{
		gsub_multi_hash *s, *tmp;
		uint16_t j = 0;
		HASH_ITER(hh, h, s, tmp) {
			subtable->from->glyphs[j].state = HANDLE_STATE_CONSOLIDATED;
			subtable->from->glyphs[j].index = s->fromid;
			subtable->from->glyphs[j].name = s->fromname;
			subtable->to[j] = s->to;
			j++;
			HASH_DEL(h, s);
			free(s);
		}
	}
	return (subtable->from->numGlyphs == 0);
}
