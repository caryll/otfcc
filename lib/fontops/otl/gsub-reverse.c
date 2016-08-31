#include "gsub-reverse.h"

typedef struct {
	int fromid;
	sds fromname;
	int toid;
	sds toname;
	UT_hash_handle hh;
} gsub_single_map_hash;
static int by_from_id(gsub_single_map_hash *a, gsub_single_map_hash *b) {
	return a->fromid - b->fromid;
}

bool consolidate_gsub_reverse(caryll_font *font, table_otl *table, otl_subtable *_subtable, sds lookupName) {
	subtable_gsub_reverse *subtable = &(_subtable->gsub_reverse);
	for (uint16_t j = 0; j < subtable->matchCount; j++) {
		consolidate_coverage(font, subtable->match[j], lookupName);
	}
	consolidate_coverage(font, subtable->to, lookupName);
	if (subtable->inputIndex >= subtable->matchCount) { subtable->inputIndex = subtable->matchCount - 1; }
	gsub_single_map_hash *h = NULL;
	otl_coverage *from = subtable->match[subtable->inputIndex];
	for (uint16_t k = 0; k < from->numGlyphs && k < subtable->to->numGlyphs; k++) {
		gsub_single_map_hash *s;
		int fromid = from->glyphs[k].index;
		HASH_FIND_INT(h, &fromid, s);
		if (s) {
			fprintf(stderr, "[Consolidate] Double-mapping a glyph in a "
			                "single substitution /%s.\n",
			        from->glyphs[k].name);
		} else {
			NEW(s);
			s->fromid = from->glyphs[k].index;
			s->toid = subtable->to->glyphs[k].index;
			s->fromname = from->glyphs[k].name;
			s->toname = subtable->to->glyphs[k].name;
			HASH_ADD_INT(h, fromid, s);
		}
	}
	HASH_SORT(h, by_from_id);
	if (HASH_COUNT(h) != from->numGlyphs || HASH_COUNT(h) != subtable->to->numGlyphs) {
		fprintf(stderr, "[Consolidate] In single subsitution lookup %s, some "
		                "mappings are ignored.\n",
		        lookupName);
	}
	from->numGlyphs = HASH_COUNT(h);
	subtable->to->numGlyphs = HASH_COUNT(h);
	{
		gsub_single_map_hash *s, *tmp;
		uint16_t j = 0;
		HASH_ITER(hh, h, s, tmp) {
			from->glyphs[j].state = HANDLE_STATE_CONSOLIDATED;
			from->glyphs[j].index = s->fromid;
			from->glyphs[j].name = s->fromname;
			subtable->to->glyphs[j].state = HANDLE_STATE_CONSOLIDATED;
			subtable->to->glyphs[j].index = s->toid;
			subtable->to->glyphs[j].name = s->toname;
			j++;
			HASH_DEL(h, s);
			free(s);
		}
	}
	return false;
}
