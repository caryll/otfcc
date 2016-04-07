#include "mark.h"

typedef struct {
	int gid;
	sds name;
	otl_mark_record markrec;
	UT_hash_handle hh;
} mark_hash;
static INLINE int mark_by_gid(mark_hash *a, mark_hash *b) { return a->gid - b->gid; }
typedef struct {
	int gid;
	sds name;
	otl_anchor *anchors;
	UT_hash_handle hh;
} base_hash;
static INLINE int base_by_gid(base_hash *a, base_hash *b) { return a->gid - b->gid; }
typedef struct {
	int gid;
	sds name;
	mark_to_ligature_base *ligAttachment;
	UT_hash_handle hh;
} lig_hash;
static INLINE int lig_by_gid(lig_hash *a, lig_hash *b) { return a->gid - b->gid; }

static void consolidateMarkArray(caryll_font *font, table_otl *table, sds lookupName, otl_coverage *marks,
                                 otl_mark_array *markArray, uint16_t classCount) {
	mark_hash *hm = NULL;
	for (uint16_t k = 0; k < marks->numGlyphs; k++) {
		if (marks->glyphs[k].name) {
			mark_hash *s = NULL;
			HASH_FIND_INT(hm, &(marks->glyphs[k].gid), s);
			if (!s && markArray->records[k].anchor.present && markArray->records[k].markClass < classCount) {
				NEW(s);
				s->gid = marks->glyphs[k].gid;
				s->name = marks->glyphs[k].name;
				s->markrec = markArray->records[k];
				HASH_ADD_INT(hm, gid, s);
			} else {
				fprintf(stderr,
				        "[Consolidate] Ignored invalid or double-mapping mark definition for /%s in lookup %s.\n",
				        marks->glyphs[k].name, lookupName);
			}
		}
	}
	HASH_SORT(hm, mark_by_gid);
	marks->numGlyphs = HASH_COUNT(hm);
	markArray->markCount = HASH_COUNT(hm);
	mark_hash *s, *tmp;
	uint16_t k = 0;
	HASH_ITER(hh, hm, s, tmp) {
		marks->glyphs[k].gid = s->gid;
		marks->glyphs[k].name = s->name;
		markArray->records[k] = s->markrec;
		k++;
		HASH_DEL(hm, s);
		free(s);
	}
}

static void consolidateBaseArray(caryll_font *font, table_otl *table, sds lookupName, otl_coverage *bases,
                                 otl_anchor **baseArray) {
	// consolidate bases
	base_hash *hm = NULL;
	for (uint16_t k = 0; k < bases->numGlyphs; k++) {
		if (bases->glyphs[k].name) {
			base_hash *s = NULL;
			HASH_FIND_INT(hm, &(bases->glyphs[k].gid), s);
			if (!s) {
				NEW(s);
				s->gid = bases->glyphs[k].gid;
				s->name = bases->glyphs[k].name;
				s->anchors = baseArray[k];
				HASH_ADD_INT(hm, gid, s);
			} else {
				free(baseArray[k]);
				fprintf(stderr, "[Consolidate] Ignored anchor double-definition for /%s in lookup %s.\n",
				        bases->glyphs[k].name, lookupName);
			}
		} else {
			free(baseArray[k]);
		}
	}
	HASH_SORT(hm, base_by_gid);
	bases->numGlyphs = HASH_COUNT(hm);
	base_hash *s, *tmp;
	uint16_t k = 0;
	HASH_ITER(hh, hm, s, tmp) {
		bases->glyphs[k].gid = s->gid;
		bases->glyphs[k].name = s->name;
		baseArray[k] = s->anchors;
		k++;
		HASH_DEL(hm, s);
		free(s);
	}
}

static void consolidateLigArray(caryll_font *font, table_otl *table, sds lookupName, otl_coverage *bases,
                                mark_to_ligature_base **ligArray) {
	lig_hash *hm = NULL;
	for (uint16_t k = 0; k < bases->numGlyphs; k++) {
		if (bases->glyphs[k].name) {
			lig_hash *s = NULL;
			HASH_FIND_INT(hm, &(bases->glyphs[k].gid), s);
			if (!s) {
				NEW(s);
				s->gid = bases->glyphs[k].gid;
				s->name = bases->glyphs[k].name;
				s->ligAttachment = ligArray[k];
				HASH_ADD_INT(hm, gid, s);
			} else {
				delete_lig_attachment(ligArray[k]);
				fprintf(stderr, "[Consolidate] Ignored anchor double-definition for /%s in lookup %s.\n",
				        bases->glyphs[k].name, lookupName);
			}
		} else {
			delete_lig_attachment(ligArray[k]);
		}
	}
	HASH_SORT(hm, lig_by_gid);
	bases->numGlyphs = HASH_COUNT(hm);
	lig_hash *s, *tmp;
	uint16_t k = 0;
	HASH_ITER(hh, hm, s, tmp) {
		bases->glyphs[k].gid = s->gid;
		bases->glyphs[k].name = s->name;
		ligArray[k] = s->ligAttachment;
		k++;
		HASH_DEL(hm, s);
		free(s);
	}
}

bool consolidate_mark_to_single(caryll_font *font, table_otl *table, otl_subtable *_subtable, sds lookupName) {
	subtable_gpos_mark_to_single *subtable = &(_subtable->gpos_mark_to_single);
	consolidate_coverage(font, subtable->marks, lookupName);
	consolidate_coverage(font, subtable->bases, lookupName);
	consolidateMarkArray(font, table, lookupName, subtable->marks, subtable->markArray, subtable->classCount);
	consolidateBaseArray(font, table, lookupName, subtable->bases, subtable->baseArray);
	return false;
}

bool consolidate_mark_to_ligature(caryll_font *font, table_otl *table, otl_subtable *_subtable, sds lookupName) {
	subtable_gpos_mark_to_ligature *subtable = &(_subtable->gpos_mark_to_ligature);
	consolidate_coverage(font, subtable->marks, lookupName);
	consolidate_coverage(font, subtable->bases, lookupName);
	consolidateMarkArray(font, table, lookupName, subtable->marks, subtable->markArray, subtable->classCount);
	consolidateLigArray(font, table, lookupName, subtable->bases, subtable->ligArray);
	return false;
}
