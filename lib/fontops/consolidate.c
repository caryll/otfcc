#include "support/util.h"
#include "otfcc/font.h"

#include "otl/gsub-single.h"
#include "otl/gsub-multi.h"
#include "otl/gsub-ligature.h"
#include "otl/gsub-reverse.h"
#include "otl/gpos-single.h"
#include "otl/gpos-pair.h"
#include "otl/gpos-cursive.h"
#include "otl/chaining.h"
#include "otl/mark.h"
#include "otl/GDEF.h"

// Consolidation
// Replace name entries in json to ids and do some check
static int by_stem_pos(const void *_a, const void *_b) {
	const glyf_PostscriptStemDef *a = _a;
	const glyf_PostscriptStemDef *b = _b;
	if (a->position == b->position) {
		return (int)a->map - (int)b->map;
	} else if (a->position > b->position) {
		return 1;
	} else {
		return -1;
	}
}
static int by_mask_pointindex(const void *a, const void *b) {
	return ((glyf_PostscriptHintMask *)a)->pointsBefore - ((glyf_PostscriptHintMask *)b)->pointsBefore;
}
void caryll_font_consolidate_glyph(glyf_Glyph *g, caryll_Font *font) {
	// The name field of glyf_Glyph will not be consolidated with glyphOrder
	// Consolidate references
	shapeid_t nReferencesConsolidated = 0;
	for (shapeid_t j = 0; j < g->numberOfReferences; j++) {
		caryll_GlyphOrderEntry *entry = NULL;
		glyf_ComponentReference *r = &(g->references[j]);
		if (r->glyph.name) {
			entry = caryll_lookupName(font->glyph_order, r->glyph.name);
			if (entry) {
				handle_consolidateTo(&r->glyph, entry->gid, entry->name);
				nReferencesConsolidated += 1;
			} else {
				fprintf(stderr, "[Consolidate] Ignored absent glyph component "
				                "reference /%s within /%s.\n",
				        r->glyph.name, g->name);
				handle_delete(&r->glyph);
			}
		} else {
			handle_delete(&r->glyph);
		}
	}
	if (nReferencesConsolidated < g->numberOfReferences) {
		if (nReferencesConsolidated == 0) {
			free(g->references);
			g->references = NULL;
			g->numberOfReferences = 0;
		} else {
			glyf_ComponentReference *consolidatedReferences =
			    calloc(nReferencesConsolidated, sizeof(glyf_ComponentReference));
			for (shapeid_t j = 0, k = 0; j < g->numberOfReferences; j++) {
				if (g->references[j].glyph.name) { consolidatedReferences[k++] = g->references[j]; }
			}
			free(g->references);
			g->references = consolidatedReferences;
			g->numberOfReferences = nReferencesConsolidated;
		}
	}

	// Sort stems
	if (g->stemH) {
		for (shapeid_t j = 0; j < g->numberOfStemH; j++) {
			g->stemH[j].map = j;
		}
		qsort(g->stemH, g->numberOfStemH, sizeof(glyf_PostscriptStemDef), by_stem_pos);
	}
	if (g->stemV) {
		for (shapeid_t j = 0; j < g->numberOfStemV; j++) {
			g->stemV[j].map = j;
		}
		qsort(g->stemV, g->numberOfStemV, sizeof(glyf_PostscriptStemDef), by_stem_pos);
	}
	shapeid_t *hmap;
	NEW_N(hmap, g->numberOfStemH);
	shapeid_t *vmap;
	NEW_N(vmap, g->numberOfStemV);
	for (shapeid_t j = 0; j < g->numberOfStemH; j++) {
		hmap[g->stemH[j].map] = j;
	}
	for (shapeid_t j = 0; j < g->numberOfStemV; j++) {
		vmap[g->stemV[j].map] = j;
	}
	// sort masks
	if (g->hintMasks) {
		qsort(g->hintMasks, g->numberOfHintMasks, sizeof(glyf_PostscriptHintMask), by_mask_pointindex);
		for (shapeid_t j = 0; j < g->numberOfHintMasks; j++) {
			glyf_PostscriptHintMask oldmask = g->hintMasks[j]; // copy
			for (shapeid_t k = 0; k < g->numberOfStemH; k++) {
				g->hintMasks[j].maskH[k] = oldmask.maskH[hmap[k]];
			}
			for (shapeid_t k = 0; k < g->numberOfStemV; k++) {
				g->hintMasks[j].maskV[k] = oldmask.maskV[vmap[k]];
			}
		}
	}
	if (g->contourMasks) {
		qsort(g->contourMasks, g->numberOfContourMasks, sizeof(glyf_PostscriptHintMask), by_mask_pointindex);
		for (shapeid_t j = 0; j < g->numberOfContourMasks; j++) {
			glyf_PostscriptHintMask oldmask = g->contourMasks[j]; // copy
			for (shapeid_t k = 0; k < g->numberOfStemH; k++) {
				g->contourMasks[j].maskH[k] = oldmask.maskH[hmap[k]];
			}
			for (shapeid_t k = 0; k < g->numberOfStemV; k++) {
				g->contourMasks[j].maskV[k] = oldmask.maskV[vmap[k]];
			}
		}
	}
	free(hmap);
	free(vmap);
	// Consolidate fdSelect
	if (g->fdSelect.name && font->CFF_ && font->CFF_->fdArray) {
		bool found = false;
		for (tableid_t j = 0; j < font->CFF_->fdArrayCount; j++) {
			if (strcmp(g->fdSelect.name, font->CFF_->fdArray[j]->fontName) == 0) {
				found = true;
				handle_consolidateTo(&(g->fdSelect), j, font->CFF_->fdArray[j]->fontName);
				break;
			}
		}
		if (!found) {
			fprintf(stderr, "[Consolidate] CID Subfont %s is not defined. (in glyph /%s).\n", g->fdSelect.name,
			        g->name);
			handle_delete(&(g->fdSelect));
		}
	} else if (g->fdSelect.name) {
		handle_delete(&(g->fdSelect));
	}
}

void caryll_font_consolidate_glyf(caryll_Font *font) {
	if (!font->glyph_order || !font->glyf) return;
	for (glyphid_t j = 0; j < font->glyf->numberGlyphs; j++) {
		if (font->glyf->glyphs[j]) {
			caryll_font_consolidate_glyph(font->glyf->glyphs[j], font);
		} else {
			font->glyf->glyphs[j] = table_new_glyf_glyph();
		}
	}
}

void caryll_font_consolidate_cmap(caryll_Font *font) {
	if (font->glyph_order && font->cmap) {
		cmap_Entry *item;
		foreach_hash(item, *font->cmap) {
			if (item->glyph.name) {
				caryll_GlyphOrderEntry *ordentry = caryll_lookupName(font->glyph_order, item->glyph.name);
				if (ordentry) {
					handle_consolidateTo(&item->glyph, ordentry->gid, ordentry->name);
				} else {
					fprintf(stderr, "[Consolidate] Ignored mapping U+%04X to "
					                "non-existent glyph /%s.\n",
					        item->unicode, item->glyph.name);
					handle_delete(&item->glyph);
				}
			} else {
				handle_delete(&item->glyph);
			}
		}
	}
}

typedef bool (*otl_consolidation_function)(caryll_Font *, table_OTL *, otl_Subtable *, sds);
typedef void (*subtable_remover)(otl_Subtable *);
#define LOOKUP_CONSOLIDATOR(llt, fn, fndel) __declare_otl_consolidation(llt, fn, fndel, font, table, lookup);

static void __declare_otl_consolidation(otl_LookupType type, otl_consolidation_function fn, subtable_remover fndel,
                                        caryll_Font *font, table_OTL *table, otl_Lookup *lookup) {
	if (lookup && lookup->subtableCount && lookup->type == type) {
		for (tableid_t j = 0; j < lookup->subtableCount; j++) {
			if (lookup->subtables[j]) {
				bool subtableRemoved = fn(font, table, lookup->subtables[j], lookup->name);
				if (subtableRemoved) {
					fndel(lookup->subtables[j]);
					lookup->subtables[j] = NULL;
					fprintf(stderr, "[Consolidate] Ignored empty subtable %d of lookup %s.\n", j, lookup->name);
				}
			}
		}
		tableid_t k = 0;
		for (tableid_t j = 0; j < lookup->subtableCount; j++) {
			if (lookup->subtables[j]) { lookup->subtables[k++] = lookup->subtables[j]; }
		}
		lookup->subtableCount = k;

		if (lookup->subtableCount &&
		    (lookup->type == otl_type_gsub_chaining || lookup->type == otl_type_gpos_chaining)) {
			fontop_classifyChainings(lookup);
			tableid_t k = 0;
			for (tableid_t j = 0; j < lookup->subtableCount; j++) {
				if (lookup->subtables[j]) { lookup->subtables[k++] = lookup->subtables[j]; }
			}
			lookup->subtableCount = k;
		}
	}
}

void caryll_consolidate_lookup(caryll_Font *font, table_OTL *table, otl_Lookup *lookup) {
	LOOKUP_CONSOLIDATOR(otl_type_gsub_single, consolidate_gsub_single, otl_delete_gsub_single);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_multiple, consolidate_gsub_multi, otl_delete_gsub_multi);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_alternate, consolidate_gsub_multi, otl_delete_gsub_multi);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_ligature, consolidate_gsub_ligature, otl_delete_gsub_ligature);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_chaining, consolidate_chaining, otl_delete_chaining);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_reverse, consolidate_gsub_reverse, otl_delete_gsub_reverse);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_single, consolidate_gpos_single, otl_delete_gpos_single);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_pair, consolidate_gpos_pair, otl_delete_gpos_pair);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_cursive, consolidate_gpos_cursive, otl_delete_gpos_cursive);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_chaining, consolidate_chaining, otl_delete_chaining);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_markToBase, consolidate_mark_to_single, otl_delete_gpos_markToSingle);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_markToMark, consolidate_mark_to_single, otl_delete_gpos_markToSingle);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_markToLigature, consolidate_mark_to_ligature, otl_delete_gpos_markToLigature);
}

void caryll_font_consolidate_otl(caryll_Font *font) {
	if (font->glyph_order && font->GSUB)
		for (tableid_t j = 0; j < font->GSUB->lookupCount; j++) {
			caryll_consolidate_lookup(font, font->GSUB, font->GSUB->lookups[j]);
		}
	if (font->glyph_order && font->GPOS)
		for (tableid_t j = 0; j < font->GPOS->lookupCount; j++) {
			caryll_consolidate_lookup(font, font->GPOS, font->GPOS->lookups[j]);
		}
	consolidate_GDEF(font, font->GDEF, "GDEF");
}

void caryll_font_consolidate(caryll_Font *font, const otfcc_Options *options) {
	// In case we don’t have a glyph order, make one.
	if (font->glyf && !font->glyph_order) {
		caryll_GlyphOrder *go = caryll_new_GlyphOrder();
		for (glyphid_t j = 0; j < font->glyf->numberGlyphs; j++) {
			sds name;
			sds glyfName = font->glyf->glyphs[j]->name;
			if (glyfName) {
				name = sdsdup(glyfName);
			} else {
				name = sdscatprintf(sdsempty(), "__gid%d", j);
				font->glyf->glyphs[j]->name = sdsdup(name);
			}
			caryll_setGlyphOrderByName(go, name, j);
		}
	}
	caryll_font_consolidate_glyf(font);
	caryll_font_consolidate_cmap(font);
	if (font->glyf) caryll_font_consolidate_otl(font);
}
