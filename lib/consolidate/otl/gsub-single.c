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
	gsub_single_map_hash *h = NULL;
	for (size_t k = 0; k < subtable->length; k++) {
		if (!GlyphOrder.consolidateHandle(font->glyph_order, &subtable->data[k].from)) { continue; }
		if (!GlyphOrder.consolidateHandle(font->glyph_order, &subtable->data[k].to)) { continue; }
		gsub_single_map_hash *s;
		int fromid = subtable->data[k].from.index;
		HASH_FIND_INT(h, &fromid, s);
		if (s) {
			logWarning("[Consolidate] Double-mapping a glyph in a single substitution /%s.\n",
			           subtable->data[k].from.name);
		} else {
			NEW(s);
			s->fromid = subtable->data[k].from.index;
			s->toid = subtable->data[k].to.index;
			s->fromname = sdsdup(subtable->data[k].from.name);
			s->toname = sdsdup(subtable->data[k].to.name);
			HASH_ADD_INT(h, fromid, s);
		}
	}
	HASH_SORT(h, by_from_id);
	if (HASH_COUNT(h) != subtable->length) { logWarning("[Consolidate] In this lookup, some mappings are ignored.\n"); }
	caryll_resetVector(subtable);
	{
		gsub_single_map_hash *s, *tmp;
		HASH_ITER(hh, h, s, tmp) {
			caryll_pushVector(subtable,
			                  ((subtable_gsub_single_entry){.from = Handle.fromConsolidated(s->fromid, s->fromname),
			                                                .to = Handle.fromConsolidated(s->toid, s->toname)}));
			HASH_DEL(h, s);
			FREE(s);
		}
	}
	return (subtable->length == 0);
}
