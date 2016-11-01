#include "gsub-single.h"

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
bool consolidate_gsub_single(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable,
                             const otfcc_Options *options) {
	subtable_gsub_single *subtable = &(_subtable->gsub_single);
	fontop_consolidateCoverage(font, subtable->from, options);
	fontop_consolidateCoverage(font, subtable->to, options);
	glyphid_t len = subtable->to->numGlyphs;
	if (subtable->from->numGlyphs < subtable->to->numGlyphs) { len = subtable->from->numGlyphs; }
	for (glyphid_t j = len; j < subtable->from->numGlyphs; j++) {
		Handle.dispose(&subtable->from->glyphs[j]);
	}
	for (glyphid_t j = len; j < subtable->to->numGlyphs; j++) {
		Handle.dispose(&subtable->to->glyphs[j]);
	}
	gsub_single_map_hash *h = NULL;
	for (glyphid_t k = 0; k < len; k++) {
		if (subtable->from->glyphs[k].name && subtable->to->glyphs[k].name) {
			gsub_single_map_hash *s;
			int fromid = subtable->from->glyphs[k].index;
			HASH_FIND_INT(h, &fromid, s);
			if (s) {
				logWarning("[Consolidate] Double-mapping a glyph in a single substitution /%s.\n",
				           subtable->from->glyphs[k].name);
			} else {
				NEW(s);
				s->fromid = subtable->from->glyphs[k].index;
				s->toid = subtable->to->glyphs[k].index;
				s->fromname = sdsdup(subtable->from->glyphs[k].name);
				s->toname = sdsdup(subtable->to->glyphs[k].name);
				HASH_ADD_INT(h, fromid, s);
			}
		}
	}
	HASH_SORT(h, by_from_id);
	if (HASH_COUNT(h) != subtable->from->numGlyphs || HASH_COUNT(h) != subtable->to->numGlyphs) {
		logWarning("[Consolidate] In this lookup, some mappings are ignored.\n");
	}
	Coverage.clear(subtable->from, 0), Coverage.clear(subtable->to, 0);
	{
		gsub_single_map_hash *s, *tmp;
		HASH_ITER(hh, h, s, tmp) {
			Coverage.push(subtable->from, Handle.fromConsolidated(s->fromid, s->fromname));
			Coverage.push(subtable->to, Handle.fromConsolidated(s->toid, s->toname));
			HASH_DEL(h, s);
			FREE(s);
		}
	}
	return (subtable->from->numGlyphs == 0);
}
