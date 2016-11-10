#include "support/util.h"
#include "otfcc/font.h"
#include "table/all.h"

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
static int by_stem_pos(const glyf_PostscriptStemDef *a, const glyf_PostscriptStemDef *b) {
	if (a->position == b->position) {
		return (int)a->map - (int)b->map;
	} else if (a->position > b->position) {
		return 1;
	} else {
		return -1;
	}
}
static int by_mask_pointindex(const glyf_PostscriptHintMask *a, const glyf_PostscriptHintMask *b) {
	return a->contoursBefore == b->contoursBefore ? a->pointsBefore - b->pointsBefore
	                                              : a->contoursBefore - b->contoursBefore;
}
void consolidateGlyph(glyf_Glyph *g, otfcc_Font *font, const otfcc_Options *options) {
	// Remove empty contours
	{
		shapeid_t nContoursConsolidated = 0;
		shapeid_t skip = 0;
		for (shapeid_t j = 0; j < g->contours.length; j++) {
			if (g->contours.items[j].length) {
				g->contours.items[j - skip] = g->contours.items[j];
				nContoursConsolidated += 1;
			} else {
				glyf_iContourList.disposeItem(&g->contours, j);
				logWarning("[Consolidate] Removed empty contour #%d in glyph %s.\n", j, g->name);
				skip += 1;
			}
		}
		g->contours.length = nContoursConsolidated;
	}

	// Consolidate references
	{
		shapeid_t nReferencesConsolidated = 0;
		shapeid_t skip = 0;
		for (shapeid_t j = 0; j < g->references.length; j++) {
			if (!GlyphOrder.consolidateHandle(font->glyph_order, &g->references.items[j].glyph)) {
				logWarning("[Consolidate] Ignored absent glyph component reference /%s within /%s.\n",
				           g->references.items[j].glyph.name, g->name);
				glyf_iReferenceList.disposeItem(&(g->references), j);
				skip += 1;
			} else {
				g->references.items[j - skip] = g->references.items[j];
				nReferencesConsolidated += 1;
			}
		}
		g->references.length = nReferencesConsolidated;
	}

	// Sort stems
	if (g->stemH.length) {
		for (shapeid_t j = 0; j < g->stemH.length; j++) {
			g->stemH.items[j].map = j;
		}
		glyf_iStemDefList.sort(&g->stemH, by_stem_pos);
	}
	if (g->stemV.length) {
		for (shapeid_t j = 0; j < g->stemV.length; j++) {
			g->stemV.items[j].map = j;
		}
		glyf_iStemDefList.sort(&g->stemV, by_stem_pos);
	}
	shapeid_t *hmap;
	NEW(hmap, g->stemH.length);
	shapeid_t *vmap;
	NEW(vmap, g->stemV.length);
	for (shapeid_t j = 0; j < g->stemH.length; j++) {
		hmap[g->stemH.items[j].map] = j;
	}
	for (shapeid_t j = 0; j < g->stemV.length; j++) {
		vmap[g->stemV.items[j].map] = j;
	}
	// sort masks
	if (g->hintMasks.length) {
		glyf_iMaskList.sort(&g->hintMasks, by_mask_pointindex);
		for (shapeid_t j = 0; j < g->hintMasks.length; j++) {
			glyf_PostscriptHintMask oldmask = g->hintMasks.items[j]; // copy
			for (shapeid_t k = 0; k < g->stemH.length; k++) {
				g->hintMasks.items[j].maskH[k] = oldmask.maskH[hmap[k]];
			}
			for (shapeid_t k = 0; k < g->stemV.length; k++) {
				g->hintMasks.items[j].maskV[k] = oldmask.maskV[vmap[k]];
			}
		}
	}
	if (g->contourMasks.length) {
		glyf_iMaskList.sort(&g->contourMasks, by_mask_pointindex);
		for (shapeid_t j = 0; j < g->contourMasks.length; j++) {
			glyf_PostscriptHintMask oldmask = g->contourMasks.items[j]; // copy
			for (shapeid_t k = 0; k < g->stemH.length; k++) {
				g->contourMasks.items[j].maskH[k] = oldmask.maskH[hmap[k]];
			}
			for (shapeid_t k = 0; k < g->stemV.length; k++) {
				g->contourMasks.items[j].maskV[k] = oldmask.maskV[vmap[k]];
			}
		}
	}
	FREE(hmap);
	FREE(vmap);
	// Consolidate fdSelect
	if (g->fdSelect.state == HANDLE_STATE_INDEX && font->CFF_ && font->CFF_->fdArray) {
		if (g->fdSelect.index >= font->CFF_->fdArrayCount) { g->fdSelect.index = 0; }
		g->fdSelect = Handle.fromConsolidated(g->fdSelect.index, font->CFF_->fdArray[g->fdSelect.index]->fontName);
	} else if (g->fdSelect.name && font->CFF_ && font->CFF_->fdArray) {
		bool found = false;
		for (tableid_t j = 0; j < font->CFF_->fdArrayCount; j++) {
			if (strcmp(g->fdSelect.name, font->CFF_->fdArray[j]->fontName) == 0) {
				found = true;
				Handle.consolidateTo(&(g->fdSelect), j, font->CFF_->fdArray[j]->fontName);
				break;
			}
		}
		if (!found) {
			logWarning("[Consolidate] CID Subfont %s is not defined. (in glyph /%s).\n", g->fdSelect.name, g->name);
			Handle.dispose(&(g->fdSelect));
		}
	} else if (g->fdSelect.name) {
		Handle.dispose(&(g->fdSelect));
	}
}

void consolidateGlyf(otfcc_Font *font, const otfcc_Options *options) {
	if (!font->glyph_order || !font->glyf) return;
	for (glyphid_t j = 0; j < font->glyf->length; j++) {
		if (font->glyf->items[j]) {
			consolidateGlyph(font->glyf->items[j], font, options);
		} else {
			font->glyf->items[j] = otfcc_newGlyf_glyph();
		}
	}
}

void consolidateCmap(otfcc_Font *font, const otfcc_Options *options) {
	if (font->glyph_order && font->cmap) {
		cmap_Entry *item;
		foreach_hash(item, font->cmap->unicodes) {
			if (!GlyphOrder.consolidateHandle(font->glyph_order, &item->glyph)) {
				logWarning("[Consolidate] Ignored mapping U+%04X to non-existent glyph /%s.\n", item->unicode,
				           item->glyph.name);
				Handle.dispose(&item->glyph);
			}
		}
	}
}

typedef bool (*otl_consolidation_function)(otfcc_Font *, table_OTL *, otl_Subtable *, const otfcc_Options *);
typedef void (*subtable_remover)(otl_Subtable *);
#define LOOKUP_CONSOLIDATOR(llt, fn, fndel)                                                                            \
	__declare_otl_consolidation(llt, fn, (subtable_remover)fndel, font, table, lookup, options);

static void __declare_otl_consolidation(otl_LookupType type, otl_consolidation_function fn, subtable_remover fndel,
                                        otfcc_Font *font, table_OTL *table, otl_Lookup *lookup,
                                        const otfcc_Options *options) {
	if (lookup && lookup->subtables.length && lookup->type == type) {
		loggedStep("%s", lookup->name) {
			for (tableid_t j = 0; j < lookup->subtables.length; j++) {
				if (lookup->subtables.items[j]) {
					bool subtableRemoved;
					// loggedStep("Subtable %d", j) {
					subtableRemoved = fn(font, table, lookup->subtables.items[j], options);
					//}
					if (subtableRemoved) {
						fndel(lookup->subtables.items[j]);
						lookup->subtables.items[j] = NULL;
						logWarning("[Consolidate] Ignored empty subtable %d of lookup %s.\n", j, lookup->name);
					}
				}
			}
		}
		tableid_t k = 0;
		for (tableid_t j = 0; j < lookup->subtables.length; j++) {
			if (lookup->subtables.items[j]) { lookup->subtables.items[k++] = lookup->subtables.items[j]; }
		}
		lookup->subtables.length = k;
	}
}

void otfcc_consolidate_lookup(otfcc_Font *font, table_OTL *table, otl_Lookup *lookup, const otfcc_Options *options) {
	LOOKUP_CONSOLIDATOR(otl_type_gsub_single, consolidate_gsub_single, iSubtable_gsub_single.destroy);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_multiple, consolidate_gsub_multi, iSubtable_gsub_multi.destroy);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_alternate, consolidate_gsub_multi, iSubtable_gsub_multi.destroy);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_ligature, consolidate_gsub_ligature, iSubtable_gsub_ligature.destroy);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_chaining, consolidate_chaining, iSubtable_chaining.destroy);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_reverse, consolidate_gsub_reverse, iSubtable_gsub_reverse.destroy);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_single, consolidate_gpos_single, iSubtable_gpos_single.destroy);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_pair, consolidate_gpos_pair, iSubtable_gpos_pair.destroy);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_cursive, consolidate_gpos_cursive, iSubtable_gpos_cursive.destroy);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_chaining, consolidate_chaining, iSubtable_chaining.destroy);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_markToBase, consolidate_mark_to_single, iSubtable_gpos_markToSingle.destroy);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_markToMark, consolidate_mark_to_single, iSubtable_gpos_markToSingle.destroy);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_markToLigature, consolidate_mark_to_ligature,
	                    iSubtable_gpos_markToLigature.destroy);
}

void consolidateOTL(otfcc_Font *font, const otfcc_Options *options) {
	loggedStep("GSUB") {
		if (font->glyph_order && font->GSUB) {
			for (tableid_t j = 0; j < font->GSUB->lookups.length; j++) {
				otfcc_consolidate_lookup(font, font->GSUB, font->GSUB->lookups.items[j], options);
			}
		}
	}
	loggedStep("GPOS") {
		if (font->glyph_order && font->GPOS) {
			for (tableid_t j = 0; j < font->GPOS->lookups.length; j++) {
				otfcc_consolidate_lookup(font, font->GPOS, font->GPOS->lookups.items[j], options);
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
		otfcc_GlyphOrder *go = GlyphOrder.create();
		for (glyphid_t j = 0; j < font->glyf->length; j++) {
			sds name;
			sds glyfName = font->glyf->items[j]->name;
			if (glyfName) {
				name = sdsdup(glyfName);
			} else {
				name = sdscatprintf(sdsempty(), "$$gid%d", j);
				font->glyf->items[j]->name = sdsdup(name);
			}
			if (!GlyphOrder.setByName(go, name, j)) {
				logWarning("[Consolidate] Glyph name %s is already in use.", name);
				uint32_t suffix = 2;
				bool success = false;
				do {
					sds newname = sdscatfmt(sdsempty(), "%s_%u", name, suffix);
					success = GlyphOrder.setByName(go, newname, j);
					if (!success) {
						sdsfree(newname);
						suffix += 1;
					} else {
						logWarning("[Consolidate] Glyph %s is renamed into %s.", name, newname);
						sdsfree(font->glyf->items[j]->name);
						font->glyf->items[j]->name = sdsdup(newname);
					}
				} while (!success);
				sdsfree(name);
			}
		}
		font->glyph_order = go;
	}
	loggedStep("glyf") {
		consolidateGlyf(font, options);
	}
	loggedStep("cmap") {
		consolidateCmap(font, options);
	}
	if (font->glyf) consolidateOTL(font, options);
}
