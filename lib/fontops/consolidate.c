#include "consolidate.h"
// Consolidation
// Replace name entries in json to gid and do some check
static int by_stem_pos(const void *_a, const void *_b) {
	const glyf_postscript_hint_stemdef *a = _a;
	const glyf_postscript_hint_stemdef *b = _b;
	if (a->position == b->position) {
		return (int)a->map - (int)b->map;
	} else if (a->position > b->position) {
		return 1;
	} else {
		return -1;
	}
}
static int by_mask_pointindex(const void *a, const void *b) {
	return ((glyf_postscript_hint_mask *)a)->pointsBefore -
	       ((glyf_postscript_hint_mask *)b)->pointsBefore;
}
void caryll_font_consolidate_glyph(glyf_glyph *g, caryll_font *font) {
	uint16_t nReferencesConsolidated = 0;
	for (uint16_t j = 0; j < g->numberOfReferences; j++) {
		glyph_order_entry *entry = NULL;
		glyf_reference *r = &(g->references[j]);
		if (r->glyph.name) {
			HASH_FIND_STR(*font->glyph_order, r->glyph.name, entry);
			if (entry) {
				r->glyph.gid = entry->gid;
				if (r->glyph.name != entry->name) sdsfree(r->glyph.name);
				r->glyph.name = entry->name;
				nReferencesConsolidated += 1;
			} else {
				fprintf(stderr, "[Consolidate] Ignored absent glyph component "
				                "reference /%s within /%s.\n",
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
			glyf_reference *consolidatedReferences =
			    calloc(nReferencesConsolidated, sizeof(glyf_reference));
			for (uint16_t j = 0, k = 0; j < g->numberOfReferences; j++) {
				if (g->references[j].glyph.name) { consolidatedReferences[k++] = g->references[j]; }
			}
			free(g->references);
			g->references = consolidatedReferences;
			g->numberOfReferences = nReferencesConsolidated;
		}
	}

	// Sort stems
	if (g->stemH) {
		for (uint16_t j = 0; j < g->numberOfStemH; j++) { g->stemH[j].map = j; }
		qsort(g->stemH, g->numberOfStemH, sizeof(glyf_postscript_hint_stemdef), by_stem_pos);
	}
	if (g->stemV) {
		for (uint16_t j = 0; j < g->numberOfStemV; j++) { g->stemV[j].map = j; }
		qsort(g->stemV, g->numberOfStemV, sizeof(glyf_postscript_hint_stemdef), by_stem_pos);
	}
	uint16_t *hmap;
	NEW_N(hmap, g->numberOfStemH);
	uint16_t *vmap;
	NEW_N(vmap, g->numberOfStemV);
	for (uint16_t j = 0; j < g->numberOfStemH; j++) { hmap[g->stemH[j].map] = j; }
	for (uint16_t j = 0; j < g->numberOfStemV; j++) { vmap[g->stemV[j].map] = j; }
	// sort masks
	if (g->hintMasks) {
		qsort(g->hintMasks, g->numberOfHintMasks, sizeof(glyf_postscript_hint_mask),
		      by_mask_pointindex);
		for (uint16_t j = 0; j < g->numberOfHintMasks; j++) {
			glyf_postscript_hint_mask oldmask = g->hintMasks[j]; // copy
			for (uint16_t k = 0; k < g->numberOfStemH; k++) {
				g->hintMasks[j].maskH[k] = oldmask.maskH[hmap[k]];
			}
			for (uint16_t k = 0; k < g->numberOfStemV; k++) {
				g->hintMasks[j].maskV[k] = oldmask.maskV[vmap[k]];
			}
		}
	}
	if (g->contourMasks) {
		qsort(g->contourMasks, g->numberOfContourMasks, sizeof(glyf_postscript_hint_mask),
		      by_mask_pointindex);
		for (uint16_t j = 0; j < g->numberOfContourMasks; j++) {
			glyf_postscript_hint_mask oldmask = g->contourMasks[j]; // copy
			for (uint16_t k = 0; k < g->numberOfStemH; k++) {
				g->contourMasks[j].maskH[k] = oldmask.maskH[hmap[k]];
			}
			for (uint16_t k = 0; k < g->numberOfStemV; k++) {
				g->contourMasks[j].maskV[k] = oldmask.maskV[vmap[k]];
			}
		}
	}
	free(hmap);
	free(vmap);
}

void caryll_font_consolidate_glyf(caryll_font *font) {
	if (font->glyph_order && *font->glyph_order && font->glyf) {
		for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
			if (font->glyf->glyphs[j]) {
				caryll_font_consolidate_glyph(font->glyf->glyphs[j], font);
			} else {
				font->glyf->glyphs[j] = caryll_new_glyf_glyph();
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
				if (item->glyph.name != ordentry->name) sdsfree(item->glyph.name);
				item->glyph.name = ordentry->name;
			} else {
				fprintf(stderr, "[Consolidate] Ignored mapping U+%04X to "
				                "non-existent glyph /%s.\n",
				        item->unicode, item->glyph.name);
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

typedef bool (*otl_consolidation_function)(caryll_font *, table_otl *, otl_subtable *, sds);
#define LOOKUP_CONSOLIDATOR(llt, fn) __declare_otl_consolidation(llt, fn, font, table, lookup);

static void __declare_otl_consolidation(otl_lookup_type type, otl_consolidation_function fn,
                                        caryll_font *font, table_otl *table, otl_lookup *lookup) {
	if (lookup && lookup->subtableCount && lookup->type == type) {
		for (uint16_t j = 0; j < lookup->subtableCount; j++) {
			if (lookup->subtables[j]) {
				bool subtableRemoved = fn(font, table, lookup->subtables[j], lookup->name);
				if (subtableRemoved) { lookup->subtables[j] = NULL; }
			}
		}
		uint16_t k = 0;
		for (uint16_t j = 0; j < lookup->subtableCount; j++) {
			if (lookup->subtables[j]) { lookup->subtables[k++] = lookup->subtables[j]; }
		}
		lookup->subtableCount = k;

		if (lookup->subtableCount &&
		    (lookup->type == otl_type_gsub_chaining || lookup->type == otl_type_gpos_chaining)) {
			classify(lookup);
			uint16_t k = 0;
			for (uint16_t j = 0; j < lookup->subtableCount; j++) {
				if (lookup->subtables[j]) { lookup->subtables[k++] = lookup->subtables[j]; }
			}
			lookup->subtableCount = k;
		}
	}
}

void caryll_consolidate_lookup(caryll_font *font, table_otl *table, otl_lookup *lookup) {
	LOOKUP_CONSOLIDATOR(otl_type_gsub_single, consolidate_gsub_single);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_multiple, consolidate_gsub_multi);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_alternate, consolidate_gsub_multi);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_ligature, consolidate_gsub_ligature);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_chaining, consolidate_chaining);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_reverse, consolidate_gsub_reverse);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_single, consolidate_gpos_single);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_pair, consolidate_gpos_pair);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_cursive, consolidate_gpos_cursive);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_chaining, consolidate_chaining);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_mark_to_base, consolidate_mark_to_single);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_mark_to_mark, consolidate_mark_to_single);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_mark_to_ligature, consolidate_mark_to_ligature);
}

void caryll_font_consolidate_otl(caryll_font *font) {
	if (font->glyph_order && font->GSUB)
		for (uint16_t j = 0; j < font->GSUB->lookupCount; j++) {
			caryll_consolidate_lookup(font, font->GSUB, font->GSUB->lookups[j]);
		}
	if (font->glyph_order && font->GPOS)
		for (uint16_t j = 0; j < font->GPOS->lookupCount; j++) {
			caryll_consolidate_lookup(font, font->GPOS, font->GPOS->lookups[j]);
		}
	consolidate_GDEF(font, font->GDEF, "GDEF");
}

void caryll_font_consolidate(caryll_font *font, caryll_options *options) {
	caryll_font_consolidate_glyf(font);
	caryll_font_consolidate_cmap(font);
	if (font->glyf) caryll_font_consolidate_otl(font);
}
