#include "mark.h"

typedef struct {
	int gid;
	sds name;
	glyphclass_t markClass;
	otl_Anchor anchor;
	UT_hash_handle hh;
} mark_hash;
static int mark_by_gid(mark_hash *a, mark_hash *b) {
	return a->gid - b->gid;
}
typedef struct {
	int gid;
	sds name;
	otl_Anchor *anchors;
	UT_hash_handle hh;
} base_hash;
static int base_by_gid(base_hash *a, base_hash *b) {
	return a->gid - b->gid;
}
typedef struct {
	int gid;
	sds name;
	glyphid_t componentCount;
	otl_Anchor **anchors;
	UT_hash_handle hh;
} lig_hash;
static int lig_by_gid(lig_hash *a, lig_hash *b) {
	return a->gid - b->gid;
}

static void consolidateMarkArray(otfcc_Font *font, table_OTL *table, const otfcc_Options *options,
                                 otl_MarkArray *markArray, glyphclass_t classCount) {
	mark_hash *hm = NULL;
	for (glyphid_t k = 0; k < markArray->length; k++) {
		if (!GlyphOrder.consolidateHandle(font->glyph_order, &markArray->data[k].glyph)) {
			logWarning("[Consolidate] Ignored unknown glyph name %s.", markArray->data[k].glyph.name);
			continue;
		}
		mark_hash *s = NULL;
		int gid = markArray->data[k].glyph.index;
		HASH_FIND_INT(hm, &gid, s);
		if (!s && markArray->data[k].anchor.present && markArray->data[k].markClass < classCount) {
			NEW(s);
			s->gid = markArray->data[k].glyph.index;
			s->name = sdsdup(markArray->data[k].glyph.name);
			s->markClass = markArray->data[k].markClass;
			s->anchor = markArray->data[k].anchor;
			HASH_ADD_INT(hm, gid, s);
		} else {
			logWarning("[Consolidate] Ignored invalid or double-mapping mark definition for /%s.",
			           markArray->data[k].glyph.name);
		}
	}
	HASH_SORT(hm, mark_by_gid);
	caryll_vecReset(markArray);
	mark_hash *s, *tmp;
	HASH_ITER(hh, hm, s, tmp) {
		caryll_vecPush(
		    markArray,
		    ((otl_MarkRecord){
		        .glyph = Handle.fromConsolidated(s->gid, s->name), .markClass = s->markClass, .anchor = s->anchor,
		    }));
		HASH_DEL(hm, s);
		FREE(s);
	}
}

static void consolidateBaseArray(otfcc_Font *font, table_OTL *table, const otfcc_Options *options,
                                 otl_BaseArray *baseArray) {
	// consolidate bases
	base_hash *hm = NULL;
	for (glyphid_t k = 0; k < baseArray->length; k++) {
		if (!GlyphOrder.consolidateHandle(font->glyph_order, &baseArray->data[k].glyph)) {
			logWarning("[Consolidate] Ignored unknown glyph name %s.", baseArray->data[k].glyph.name);
			continue;
		}
		base_hash *s = NULL;
		int gid = baseArray->data[k].glyph.index;
		HASH_FIND_INT(hm, &gid, s);
		if (!s) {
			NEW(s);
			s->gid = baseArray->data[k].glyph.index;
			s->name = sdsdup(baseArray->data[k].glyph.name);
			s->anchors = baseArray->data[k].anchors;
			baseArray->data[k].anchors = NULL; // Transfer ownership
			HASH_ADD_INT(hm, gid, s);
		} else {
			logWarning("[Consolidate] Ignored anchor double-definition for /%s.", baseArray->data[k].glyph.name);
		}
	}
	HASH_SORT(hm, base_by_gid);
	caryll_vecReset(baseArray);
	base_hash *s, *tmp;
	HASH_ITER(hh, hm, s, tmp) {
		caryll_vecPush(baseArray, ((otl_BaseRecord){
		                              .glyph = Handle.fromConsolidated(s->gid, s->name), .anchors = s->anchors,
		                          }));
		HASH_DEL(hm, s);
		FREE(s);
	}
}

static void consolidateLigArray(otfcc_Font *font, table_OTL *table, const otfcc_Options *options,
                                otl_LigatureArray *ligArray) {
	lig_hash *hm = NULL;
	for (glyphid_t k = 0; k < ligArray->length; k++) {
		if (!GlyphOrder.consolidateHandle(font->glyph_order, &ligArray->data[k].glyph)) {
			logWarning("[Consolidate] Ignored unknown glyph name %s.", ligArray->data[k].glyph.name);
			continue;
		}
		lig_hash *s = NULL;
		int gid = ligArray->data[k].glyph.index;
		HASH_FIND_INT(hm, &gid, s);
		if (!s) {
			NEW(s);
			s->gid = ligArray->data[k].glyph.index;
			s->name = sdsdup(ligArray->data[k].glyph.name);
			s->componentCount = ligArray->data[k].componentCount;
			s->anchors = ligArray->data[k].anchors;
			ligArray->data[k].anchors = NULL;
			HASH_ADD_INT(hm, gid, s);
		} else {
			logWarning("[Consolidate] Ignored anchor double-definition for /%s.", ligArray->data[k].glyph.name);
		}
	}
	HASH_SORT(hm, lig_by_gid);
	caryll_vecReset(ligArray);
	lig_hash *s, *tmp;
	HASH_ITER(hh, hm, s, tmp) {
		caryll_vecPush(ligArray, ((otl_LigatureBaseRecord){
		                             .glyph = Handle.fromConsolidated(s->gid, s->name),
		                             .componentCount = s->componentCount,
		                             .anchors = s->anchors,
		                         }));
		HASH_DEL(hm, s);
		FREE(s);
	}
}

bool consolidate_mark_to_single(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable,
                                const otfcc_Options *options) {
	subtable_gpos_markToSingle *subtable = &(_subtable->gpos_markToSingle);
	consolidateMarkArray(font, table, options, &subtable->markArray, subtable->classCount);
	consolidateBaseArray(font, table, options, &subtable->baseArray);
	return false;
}

bool consolidate_mark_to_ligature(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable,
                                  const otfcc_Options *options) {
	subtable_gpos_markToLigature *subtable = &(_subtable->gpos_markToLigature);
	consolidateMarkArray(font, table, options, &subtable->markArray, subtable->classCount);
	consolidateLigArray(font, table, options, &subtable->ligArray);
	return false;
}
