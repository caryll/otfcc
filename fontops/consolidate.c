#include "consolidate.h"
// Consolidation
// Replace name entries in json to gid and do some check
void caryll_font_consolidate_glyph(glyf_glyph *g, caryll_font *font) {
	uint16_t nReferencesConsolidated = 0;
	for (uint16_t j = 0; j < g->numberOfReferences; j++) {
		glyph_order_entry *entry = NULL;
		glyf_reference *r = &(g->references[j]);
		if (r->glyph.name) {
			HASH_FIND_STR(*font->glyph_order, r->glyph.name, entry);
			if (entry) {
				r->glyph.gid = entry->gid;
				sdsfree(r->glyph.name);
				r->glyph.name = entry->name;
				nReferencesConsolidated += 1;
			} else {
				fprintf(stderr, "[Consolidate] Ignored absent glyph component reference /%s within /%s.\n",
				        r->glyph.name, g->name);
				r->glyph.gid = 0;
				sdsfree(r->glyph.name);
				r->glyph.name = NULL;
			}
		} else {
			r->glyph.gid = 0;
			r->glyph.name = NULL;
		}
	}
	if (nReferencesConsolidated < g->numberOfReferences) {
		if (nReferencesConsolidated == 0) {
			free(g->references);
			g->references = NULL;
			g->numberOfReferences = 0;
		} else {
			glyf_reference *consolidatedReferences = calloc(nReferencesConsolidated, sizeof(glyf_reference));
			for (uint16_t j = 0, k = 0; j < g->numberOfReferences; j++) {
				if (g->references[j].glyph.name) { consolidatedReferences[k++] = g->references[j]; }
			}
			free(g->references);
			g->references = consolidatedReferences;
			g->numberOfReferences = nReferencesConsolidated;
		}
	}
}
void caryll_font_consolidate_glyf(caryll_font *font) {
	if (font->glyph_order && *font->glyph_order && font->glyf) {
		for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
			if (font->glyf->glyphs[j]) {
				caryll_font_consolidate_glyph(font->glyf->glyphs[j], font);
			} else {
				font->glyf->glyphs[j] = caryll_new_glyf_glhph();
			}
		}
	}
}
void caryll_font_consolidate_cmap(caryll_font *font) {
	if (font->glyph_order && *font->glyph_order && font->cmap) {
		cmap_entry *item;
		foreach_hash(item, *font->cmap) if (item->glyph.name) {
			glyph_order_entry *ordentry;
			HASH_FIND_STR(*font->glyph_order, item->glyph.name, ordentry);
			if (ordentry) {
				item->glyph.gid = ordentry->gid;
				sdsfree(item->glyph.name);
				item->glyph.name = ordentry->name;
			} else {
				fprintf(stderr, "[Consolidate] Ignored mapping U+%04X to non-existent glyph /%s.\n", item->unicode,
				        item->glyph.name);
				item->glyph.gid = 0;
				sdsfree(item->glyph.name);
				item->glyph.name = NULL;
			}
		}
		else {
			item->glyph.gid = 0;
			item->glyph.name = NULL;
		}
	}
}
static void consolidate_coverage(caryll_font *font, otl_coverage *coverage, sds lookupName) {
	for (uint16_t j = 0; j < coverage->numGlyphs; j++) {
		glyph_order_entry *ordentry;
		HASH_FIND_STR(*font->glyph_order, coverage->glyphs[j].name, ordentry);
		if (ordentry) {
			coverage->glyphs[j].gid = ordentry->gid;
			sdsfree(coverage->glyphs[j].name);
			coverage->glyphs[j].name = ordentry->name;
		} else {
			fprintf(stderr, "[Consolidate] Ignored missing glyph /%s in lookup %s.\n", coverage->glyphs[j].name,
			        lookupName);
			coverage->glyphs[j].gid = 0;
			DELETE(sdsfree, coverage->glyphs[j].name);
		}
	}
}
typedef struct {
	int fromid;
	sds fromname;
	int toid;
	sds toname;
	UT_hash_handle hh;
} gsub_single_map_hash;
static INLINE int by_from_id(gsub_single_map_hash *a, gsub_single_map_hash *b) { return a->fromid - b->fromid; }
static void consolidate_gsub_single(caryll_font *font, otl_subtable *_subtable, sds lookupName) {
	subtable_gsub_single *subtable = &(_subtable->gsub_single);
	consolidate_coverage(font, subtable->from, lookupName);
	consolidate_coverage(font, subtable->to, lookupName);
	uint16_t len =
	    (subtable->from->numGlyphs < subtable->to->numGlyphs ? subtable->from->numGlyphs : subtable->from->numGlyphs);
	gsub_single_map_hash *h = NULL;
	for (uint16_t k = 0; k < len; k++) {
		if (subtable->from->glyphs[k].name && subtable->to->glyphs[k].name) {
			gsub_single_map_hash *s;
			HASH_FIND_INT(h, &(subtable->from->glyphs[k].gid), s);
			if (s) {
				fprintf(stderr, "[Consolidate] Double-mapping a glyph in a single substitution /%s.\n",
				        subtable->from->glyphs[k].name);
			} else {
				NEW(s);
				s->fromid = subtable->from->glyphs[k].gid;
				s->toid = subtable->to->glyphs[k].gid;
				s->fromname = subtable->from->glyphs[k].name;
				s->toname = subtable->to->glyphs[k].name;
				HASH_ADD_INT(h, fromid, s);
			}
		}
	}
	HASH_SORT(h, by_from_id);
	if (HASH_COUNT(h) != subtable->from->numGlyphs || HASH_COUNT(h) != subtable->to->numGlyphs) {
		fprintf(stderr, "[Consolidate] In single subsitution lookup %s, some mappings are ignored.\n", lookupName);
	}
	subtable->from->numGlyphs = HASH_COUNT(h);
	subtable->to->numGlyphs = HASH_COUNT(h);
	FREE(subtable->from->glyphs);
	FREE(subtable->to->glyphs);
	NEW_N(subtable->from->glyphs, subtable->from->numGlyphs);
	NEW_N(subtable->to->glyphs, subtable->to->numGlyphs);
	{
		gsub_single_map_hash *s, *tmp;
		uint16_t j = 0;
		HASH_ITER(hh, h, s, tmp) {
			subtable->from->glyphs[j].gid = s->fromid;
			subtable->from->glyphs[j].name = s->fromname;
			subtable->to->glyphs[j].gid = s->toid;
			subtable->to->glyphs[j].name = s->toname;
			j++;
			HASH_DEL(h, s);
			free(s);
		}
	}
}

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
static void consolidate_mark_to_single(caryll_font *font, otl_subtable *_subtable, sds lookupName) {
	subtable_gpos_mark_to_single *subtable = &(_subtable->gpos_mark_to_single);
	consolidate_coverage(font, subtable->marks, lookupName);
	consolidate_coverage(font, subtable->bases, lookupName);
	{ // consolidate marks
		mark_hash *hm = NULL;
		for (uint16_t k = 0; k < subtable->marks->numGlyphs; k++) {
			if (subtable->marks->glyphs[k].name) {
				mark_hash *s = NULL;
				HASH_FIND_INT(hm, &(subtable->marks->glyphs[k].gid), s);
				if (!s && subtable->markArray->records[k].anchor.present &&
				    subtable->markArray->records[k].markClass < subtable->classCount) {
					NEW(s);
					s->gid = subtable->marks->glyphs[k].gid;
					s->name = subtable->marks->glyphs[k].name;
					s->markrec = subtable->markArray->records[k];
					HASH_ADD_INT(hm, gid, s);
				} else {
					fprintf(stderr,
					        "[Consolidate] Ignored invalid or double-mapping mark definition for /%s in lookup %s.\n",
					        subtable->marks->glyphs[k].name, lookupName);
				}
			}
		}
		HASH_SORT(hm, mark_by_gid);
		subtable->marks->numGlyphs = HASH_COUNT(hm);
		subtable->markArray->markCount = HASH_COUNT(hm);
		mark_hash *s, *tmp;
		uint16_t k = 0;
		HASH_ITER(hh, hm, s, tmp) {
			subtable->marks->glyphs[k].gid = s->gid;
			subtable->marks->glyphs[k].name = s->name;
			subtable->markArray->records[k] = s->markrec;
			k++;
			HASH_DEL(hm, s);
			free(s);
		}
	}
	{ // consolidate bases
		base_hash *hm = NULL;
		for (uint16_t k = 0; k < subtable->bases->numGlyphs; k++) {
			if (subtable->bases->glyphs[k].name) {
				base_hash *s = NULL;
				HASH_FIND_INT(hm, &(subtable->bases->glyphs[k].gid), s);
				if (!s) {
					NEW(s);
					s->gid = subtable->bases->glyphs[k].gid;
					s->name = subtable->bases->glyphs[k].name;
					s->anchors = subtable->baseArray[k];
					HASH_ADD_INT(hm, gid, s);
				} else {
					free(subtable->baseArray[k]);
					fprintf(stderr, "[Consolidate] Ignored anchor double-definition for /%s in lookup %s.\n",
					        subtable->bases->glyphs[k].name, lookupName);
				}
			} else {
				free(subtable->baseArray[k]);
			}
		}
		HASH_SORT(hm, base_by_gid);
		subtable->bases->numGlyphs = HASH_COUNT(hm);
		base_hash *s, *tmp;
		uint16_t k = 0;
		HASH_ITER(hh, hm, s, tmp) {
			subtable->bases->glyphs[k].gid = s->gid;
			subtable->bases->glyphs[k].name = s->name;
			subtable->baseArray[k] = s->anchors;
			k++;
			HASH_DEL(hm, s);
			free(s);
		}
	}
}

void declare_consolidate_type(otl_lookup_type type, void (*fn)(caryll_font *, otl_subtable *, sds), caryll_font *font,
                              otl_lookup *lookup) {
	if (lookup && lookup->subtableCount && lookup->type == type) {
		for (uint16_t j = 0; j < lookup->subtableCount; j++) {
			if (lookup->subtables[j]) { fn(font, lookup->subtables[j], lookup->name); }
		}
	}
}
void caryll_consolidate_lookup(caryll_font *font, otl_lookup *lookup) {
	declare_consolidate_type(otl_type_gsub_single, consolidate_gsub_single, font, lookup);
	declare_consolidate_type(otl_type_gpos_mark_to_base, consolidate_mark_to_single, font, lookup);
	declare_consolidate_type(otl_type_gpos_mark_to_mark, consolidate_mark_to_single, font, lookup);
}

void caryll_font_consolidate_otl(caryll_font *font) {
	if (font->glyph_order && font->GSUB)
		for (uint16_t j = 0; j < font->GSUB->lookupCount; j++) {
			caryll_consolidate_lookup(font, font->GSUB->lookups[j]);
		}
	if (font->glyph_order && font->GPOS)
		for (uint16_t j = 0; j < font->GPOS->lookupCount; j++) {
			caryll_consolidate_lookup(font, font->GPOS->lookups[j]);
		}
}

void caryll_font_consolidate(caryll_font *font, caryll_dump_options *dumpopts) {
	caryll_font_consolidate_glyf(font);
	caryll_font_consolidate_cmap(font);
	caryll_font_consolidate_otl(font);
}
