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
void otfcc_consolidateTableglyph(glyf_Glyph *g, otfcc_Font *font, const otfcc_Options *options) {
	// The name field of glyf_Glyph will not be consolidated with glyphOrder
	// Consolidate references
	shapeid_t nContoursConsolidated = 0;
	shapeid_t skip = 0;
	for (shapeid_t j = 0; j < g->numberOfContours; j++) {
		if (g->contours[j].pointsCount) {
			g->contours[j - skip] = g->contours[j];
			nContoursConsolidated += 1;
		} else {
			logWarning("[Consolidate] Removed empty contour #%d in glyph %s.\n", j, g->name);
			skip += 1;
		}
	}
	g->numberOfContours = nContoursConsolidated;
	shapeid_t nReferencesConsolidated = 0;
	for (shapeid_t j = 0; j < g->numberOfReferences; j++) {
		if (!otfcc_gordConsolidateHandle(font->glyph_order, &g->references[j].glyph)) {
			logWarning("[Consolidate] Ignored absent glyph component reference /%s within /%s.\n",
			           g->references[j].glyph.name, g->name);
			handle_dispose(&g->references[j].glyph);
		} else {
			nReferencesConsolidated += 1;
		}
	}
	if (nReferencesConsolidated < g->numberOfReferences) {
		if (nReferencesConsolidated == 0) {
			FREE(g->references);
			g->references = NULL;
			g->numberOfReferences = 0;
		} else {
			glyf_ComponentReference *consolidatedReferences;
			NEW(consolidatedReferences, nReferencesConsolidated);
			for (shapeid_t j = 0, k = 0; j < g->numberOfReferences; j++) {
				if (g->references[j].glyph.name) { consolidatedReferences[k++] = g->references[j]; }
			}
			FREE(g->references);
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
	NEW(hmap, g->numberOfStemH);
	shapeid_t *vmap;
	NEW(vmap, g->numberOfStemV);
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
	FREE(hmap);
	FREE(vmap);
	// Consolidate fdSelect
	if (g->fdSelect.state == HANDLE_STATE_INDEX && font->CFF_ && font->CFF_->fdArray) {
		if (g->fdSelect.index >= font->CFF_->fdArrayCount) { g->fdSelect.index = 0; }
		g->fdSelect = handle_fromConsolidated(g->fdSelect.index, font->CFF_->fdArray[g->fdSelect.index]->fontName);
	} else if (g->fdSelect.name && font->CFF_ && font->CFF_->fdArray) {
		bool found = false;
		for (tableid_t j = 0; j < font->CFF_->fdArrayCount; j++) {
			if (strcmp(g->fdSelect.name, font->CFF_->fdArray[j]->fontName) == 0) {
				found = true;
				handle_consolidateTo(&(g->fdSelect), j, font->CFF_->fdArray[j]->fontName);
				break;
			}
		}
		if (!found) {
			logWarning("[Consolidate] CID Subfont %s is not defined. (in glyph /%s).\n", g->fdSelect.name, g->name);
			handle_dispose(&(g->fdSelect));
		}
	} else if (g->fdSelect.name) {
		handle_dispose(&(g->fdSelect));
	}
}

void consolidateGlyf(otfcc_Font *font, const otfcc_Options *options) {
	if (!font->glyph_order || !font->glyf) return;
	for (glyphid_t j = 0; j < font->glyf->numberGlyphs; j++) {
		if (font->glyf->glyphs[j]) {
			otfcc_consolidateTableglyph(font->glyf->glyphs[j], font, options);
		} else {
			font->glyf->glyphs[j] = otfcc_newGlyf_glyph();
		}
	}
}

void consolidateCmap(otfcc_Font *font, const otfcc_Options *options) {
	if (font->glyph_order && font->cmap) {
		cmap_Entry *item;
		foreach_hash(item, *font->cmap) {
			if (!otfcc_gordConsolidateHandle(font->glyph_order, &item->glyph)) {
				logWarning("[Consolidate] Ignored mapping U+%04X to non-existent glyph /%s.\n", item->unicode,
				           item->glyph.name);
				handle_dispose(&item->glyph);
			}
		}
	}
}

typedef bool (*otl_consolidation_function)(otfcc_Font *, table_OTL *, otl_Subtable *, const otfcc_Options *);
typedef void (*subtable_remover)(otl_Subtable *);
#define LOOKUP_CONSOLIDATOR(llt, fn, fndel) __declare_otl_consolidation(llt, fn, fndel, font, table, lookup, options);

static void __declare_otl_consolidation(otl_LookupType type, otl_consolidation_function fn, subtable_remover fndel,
                                        otfcc_Font *font, table_OTL *table, otl_Lookup *lookup,
                                        const otfcc_Options *options) {
	if (lookup && lookup->subtableCount && lookup->type == type) {
		loggedStep(lookup->name) {
			for (tableid_t j = 0; j < lookup->subtableCount; j++) {
				if (lookup->subtables[j]) {
					bool subtableRemoved;
					loggedStep("Subtable %d", j) {
						subtableRemoved = fn(font, table, lookup->subtables[j], options);
					}
					if (subtableRemoved) {
						fndel(lookup->subtables[j]);
						lookup->subtables[j] = NULL;
						logWarning("[Consolidate] Ignored empty subtable %d of lookup %s.\n", j, lookup->name);
					}
				}
			}
		}
		tableid_t k = 0;
		for (tableid_t j = 0; j < lookup->subtableCount; j++) {
			if (lookup->subtables[j]) { lookup->subtables[k++] = lookup->subtables[j]; }
		}
		lookup->subtableCount = k;
	}
}

void otfcc_consolidate_lookup(otfcc_Font *font, table_OTL *table, otl_Lookup *lookup, const otfcc_Options *options) {
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

void consolidateOTL(otfcc_Font *font, const otfcc_Options *options) {
	loggedStep("GSUB") {
		if (font->glyph_order && font->GSUB) {
			for (tableid_t j = 0; j < font->GSUB->lookupCount; j++) {
				otfcc_consolidate_lookup(font, font->GSUB, font->GSUB->lookups[j], options);
			}
		}
	}
	loggedStep("GPOS") {
		if (font->glyph_order && font->GPOS) {
			for (tableid_t j = 0; j < font->GPOS->lookupCount; j++) {
				otfcc_consolidate_lookup(font, font->GPOS, font->GPOS->lookups[j], options);
			}
		}
	}
	loggedStep("GDEF") {
		consolidate_GDEF(font, font->GDEF, options);
	}
}

void otfcc_consolidateFont(otfcc_Font *font, const otfcc_Options *options) {
	// In case we don’t have a glyph order, make one.
	if (font->glyf && !font->glyph_order) {
		otfcc_GlyphOrder *go = otfcc_newGlyphOrder();
		for (glyphid_t j = 0; j < font->glyf->numberGlyphs; j++) {
			sds name;
			sds glyfName = font->glyf->glyphs[j]->name;
			if (glyfName) {
				name = sdsdup(glyfName);
			} else {
				name = sdscatprintf(sdsempty(), "__gid%d", j);
				font->glyf->glyphs[j]->name = sdsdup(name);
			}
			otfcc_setGlyphOrderByName(go, name, j);
		}
	}
	loggedStep("glyf") {
		consolidateGlyf(font, options);
	}
	loggedStep("cmap") {
		consolidateCmap(font, options);
	}
	if (font->glyf) consolidateOTL(font, options);
}
