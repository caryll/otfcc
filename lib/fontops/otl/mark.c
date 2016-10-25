#include "mark.h"

typedef struct {
	int gid;
	sds name;
	otl_MarkRecord markrec;
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
	otl_MarkToLigatureBase *ligAttachment;
	UT_hash_handle hh;
} lig_hash;
static int lig_by_gid(lig_hash *a, lig_hash *b) {
	return a->gid - b->gid;
}

static void consolidateMarkArray(caryll_Font *font, table_OTL *table, sds lookupName, otl_Coverage *marks,
                                 otl_MarkArray *markArray, glyphclass_t classCount) {
	mark_hash *hm = NULL;
	for (glyphid_t k = 0; k < marks->numGlyphs; k++) {
		if (marks->glyphs[k].name) {
			mark_hash *s = NULL;
			int gid = marks->glyphs[k].index;
			HASH_FIND_INT(hm, &gid, s);
			if (!s && markArray->records[k].anchor.present && markArray->records[k].markClass < classCount) {
				NEW(s);
				s->gid = marks->glyphs[k].index;
				s->name = marks->glyphs[k].name;
				s->markrec = markArray->records[k];
				HASH_ADD_INT(hm, gid, s);
			} else {
				fprintf(stderr, "[Consolidate] Ignored invalid or "
				                "double-mapping mark definition for /%s in "
				                "lookup %s.\n",
				        marks->glyphs[k].name, lookupName);
			}
		}
	}
	HASH_SORT(hm, mark_by_gid);
	marks->numGlyphs = HASH_COUNT(hm);
	markArray->markCount = HASH_COUNT(hm);
	mark_hash *s, *tmp;
	glyphid_t k = 0;
	HASH_ITER(hh, hm, s, tmp) {
		marks->glyphs[k] = handle_fromConsolidated(s->gid, s->name);
		markArray->records[k] = s->markrec;
		k++;
		HASH_DEL(hm, s);
		free(s);
	}
}

static void consolidateBaseArray(caryll_Font *font, table_OTL *table, sds lookupName, otl_Coverage *bases,
                                 otl_Anchor **baseArray) {
	// consolidate bases
	base_hash *hm = NULL;
	for (glyphid_t k = 0; k < bases->numGlyphs; k++) {
		if (bases->glyphs[k].name) {
			base_hash *s = NULL;
			int gid = bases->glyphs[k].index;
			HASH_FIND_INT(hm, &gid, s);
			if (!s) {
				NEW(s);
				s->gid = bases->glyphs[k].index;
				s->name = bases->glyphs[k].name;
				s->anchors = baseArray[k];
				HASH_ADD_INT(hm, gid, s);
			} else {
				free(baseArray[k]);
				fprintf(stderr, "[Consolidate] Ignored anchor "
				                "double-definition for /%s in lookup %s.\n",
				        bases->glyphs[k].name, lookupName);
			}
		} else {
			free(baseArray[k]);
		}
	}
	HASH_SORT(hm, base_by_gid);
	bases->numGlyphs = HASH_COUNT(hm);
	base_hash *s, *tmp;
	glyphid_t k = 0;
	HASH_ITER(hh, hm, s, tmp) {
		bases->glyphs[k] = handle_fromConsolidated(s->gid, s->name);
		baseArray[k] = s->anchors;
		k++;
		HASH_DEL(hm, s);
		free(s);
	}
}

static void consolidateLigArray(caryll_Font *font, table_OTL *table, sds lookupName, otl_Coverage *bases,
                                otl_MarkToLigatureBase **ligArray) {
	lig_hash *hm = NULL;
	for (glyphid_t k = 0; k < bases->numGlyphs; k++) {
		if (bases->glyphs[k].name) {
			lig_hash *s = NULL;
			int gid = bases->glyphs[k].index;
			HASH_FIND_INT(hm, &gid, s);
			if (!s) {
				NEW(s);
				s->gid = bases->glyphs[k].index;
				s->name = bases->glyphs[k].name;
				s->ligAttachment = ligArray[k];
				HASH_ADD_INT(hm, gid, s);
			} else {
				delete_lig_attachment(ligArray[k]);
				fprintf(stderr, "[Consolidate] Ignored anchor "
				                "double-definition for /%s in lookup %s.\n",
				        bases->glyphs[k].name, lookupName);
			}
		} else {
			delete_lig_attachment(ligArray[k]);
		}
	}
	HASH_SORT(hm, lig_by_gid);
	bases->numGlyphs = HASH_COUNT(hm);
	lig_hash *s, *tmp;
	glyphid_t k = 0;
	HASH_ITER(hh, hm, s, tmp) {
		bases->glyphs[k] = handle_fromConsolidated(s->gid, s->name);
		ligArray[k] = s->ligAttachment;
		k++;
		HASH_DEL(hm, s);
		free(s);
	}
}

bool consolidate_mark_to_single(caryll_Font *font, table_OTL *table, otl_Subtable *_subtable, sds lookupName) {
	subtable_gpos_markToSingle *subtable = &(_subtable->gpos_markToSingle);
	fontop_consolidateCoverage(font, subtable->marks, lookupName);
	fontop_consolidateCoverage(font, subtable->bases, lookupName);
	consolidateMarkArray(font, table, lookupName, subtable->marks, subtable->markArray, subtable->classCount);
	consolidateBaseArray(font, table, lookupName, subtable->bases, subtable->baseArray);
	return false;
}

bool consolidate_mark_to_ligature(caryll_Font *font, table_OTL *table, otl_Subtable *_subtable, sds lookupName) {
	subtable_gpos_markToLigature *subtable = &(_subtable->gpos_markToLigature);
	fontop_consolidateCoverage(font, subtable->marks, lookupName);
	fontop_consolidateCoverage(font, subtable->bases, lookupName);
	consolidateMarkArray(font, table, lookupName, subtable->marks, subtable->markArray, subtable->classCount);
	consolidateLigArray(font, table, lookupName, subtable->bases, subtable->ligArray);
	return false;
}
