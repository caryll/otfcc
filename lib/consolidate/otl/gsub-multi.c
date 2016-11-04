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

bool consolidate_gsub_multi(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable, const otfcc_Options *options) {
	subtable_gsub_multi *subtable = &(_subtable->gsub_multi);
	gsub_multi_hash *h = NULL;

	for (glyphid_t k = 0; k < subtable->length; k++) {
		if (!GlyphOrder.consolidateHandle(font->glyph_order, &subtable->data[k].from)) {
			logWarning("[Consolidate] Ignored missing glyph /%s.\n", subtable->data[k].from.name);
			continue;
		}
		fontop_consolidateCoverage(font, subtable->data[k].to, options);
		Coverage.shrink(subtable->data[k].to, false);

		gsub_multi_hash *s;
		int fromid = subtable->data[k].from.index;
		HASH_FIND_INT(h, &fromid, s);
		if (!s) {
			NEW(s);
			s->fromid = subtable->data[k].from.index;
			s->fromname = sdsdup(subtable->data[k].from.name);
			s->to = subtable->data[k].to;
			subtable->data[k].to = NULL; // Transfer ownership
			HASH_ADD_INT(h, fromid, s);
		}
	}
	HASH_SORT(h, by_from_id_multi);
	caryll_vecReset(subtable);
	{
		gsub_multi_hash *s, *tmp;
		HASH_ITER(hh, h, s, tmp) {
			caryll_vecPush(subtable, ((otl_GsubMultiEntry){
			                             .from = Handle.fromConsolidated(s->fromid, s->fromname), .to = s->to,
			                         }));
			HASH_DEL(h, s);
			FREE(s);
		}
	}
	return (subtable->length == 0);
}
