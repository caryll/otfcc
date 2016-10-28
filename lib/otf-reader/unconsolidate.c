#include "unconsolidate.h"
#include "support/util.h"
#include "support/aglfn/aglfn.h"

// Unconsolidation: Remove redundent data and de-couple internal data
// It does these things:
//   1. Merge hmtx data into glyf
//   2. Replace all glyph IDs into glyph names. Note all glyph references with
//      same name whare one unique string entity stored in font->glyph_order.
//      (Separate?)
static void caryll_name_glyphs(caryll_Font *font, const otfcc_Options *options) {
	if (!font->glyf) return;
	caryll_GlyphOrder *glyph_order = caryll_new_GlyphOrder();
	caryll_GlyphOrder *aglfn = caryll_new_GlyphOrder();
	aglfn_setupNames(aglfn);
	glyphid_t numGlyphs = font->glyf->numberGlyphs;
	sds prefix;
	if (options->glyph_name_prefix) {
		prefix = sdsnew(options->glyph_name_prefix);
	} else {
		prefix = sdsempty();
	}
	// pass 1: Map to existing glyph names
	for (glyphid_t j = 0; j < numGlyphs; j++) {
		if (font->glyf->glyphs[j]->name) {
			sds gname = sdscatprintf(sdsempty(), "%s%s", prefix, font->glyf->glyphs[j]->name);
			sds sharedName = caryll_setGlyphOrderByGID(glyph_order, j, gname);
			sdsfree(font->glyf->glyphs[j]->name);
			font->glyf->glyphs[j]->name = sharedName;
		}
	}

	// pass 2: Map to `post` names
	if (font->post != NULL && font->post->post_name_map != NULL) {
		caryll_GlyphOrderEntry *s, *tmp;
		HASH_ITER(hhID, font->post->post_name_map->byGID, s, tmp) {
			sds gname = sdscatprintf(sdsempty(), "%s%s", prefix, s->name);
			caryll_setGlyphOrderByGID(glyph_order, s->gid, gname);
		}
	}

	// pass 3: Map to AGLFN & Unicode
	if (font->cmap != NULL) {
		cmap_Entry *s;
		foreach_hash(s, *font->cmap) if (s->glyph.index > 0) {
			sds name = NULL;
			gord_nameAFieldShared(aglfn, s->unicode, &name);
			if (name == NULL) {
				name = sdscatprintf(sdsempty(), "%suni%04X", prefix, s->unicode);
			} else {
				name = sdscatprintf(sdsempty(), "%s%s", prefix, name);
			}
			caryll_setGlyphOrderByGID(glyph_order, s->glyph.index, name);
		}
	}

	// pass 4 : Map to GID
	for (glyphid_t j = 0; j < numGlyphs; j++) {
		sds name;
		if (j) {
			name = sdscatfmt(sdsempty(), "%sglyph%u", prefix, j);
		} else {
			name = sdscatfmt(sdsempty(), "%s.notdef", prefix);
		}
		caryll_setGlyphOrderByGID(glyph_order, j, name);
	}

	caryll_delete_GlyphOrder(aglfn);
	sdsfree(prefix);
	font->glyph_order = glyph_order;
}

static void caryll_name_glyf(caryll_Font *font) {
	if (font->glyph_order != NULL && font->glyf != NULL) {
		for (glyphid_t j = 0; j < font->glyf->numberGlyphs; j++) {
			glyf_Glyph *g = font->glyf->glyphs[j];
			sds glyphName = NULL;
			gord_nameAFieldShared(font->glyph_order, j, &glyphName);
			g->name = sdsdup(glyphName);
		}
	}
}

static void unconsolidate_chaining(caryll_Font *font, otl_Lookup *lookup, table_OTL *table) {
	tableid_t totalRules = 0;
	for (tableid_t j = 0; j < lookup->subtableCount; j++) {
		if (!lookup->subtables[j]) continue;
		if (lookup->subtables[j]->chaining.type == otl_chaining_poly) {
			totalRules += lookup->subtables[j]->chaining.rulesCount;
		} else if (lookup->subtables[j]->chaining.type == otl_chaining_canonical) {
			totalRules += 1;
		}
	}
	otl_Subtable **newsts;
	NEW(newsts, totalRules);
	tableid_t jj = 0;
	for (tableid_t j = 0; j < lookup->subtableCount; j++) {
		if (!lookup->subtables[j]) continue;
		if (lookup->subtables[j]->chaining.type == otl_chaining_poly) {
			for (tableid_t k = 0; k < lookup->subtables[j]->chaining.rulesCount; k++) {
				NEW(newsts[jj]);
				newsts[jj]->chaining.type = otl_chaining_canonical;
				newsts[jj]->chaining.rule = *(lookup->subtables[j]->chaining.rules[k]);
				jj += 1;
			}
			FREE(lookup->subtables[j]->chaining.rules);
			FREE(lookup->subtables[j]);
		} else if (lookup->subtables[j]->chaining.type == otl_chaining_canonical) {
			NEW(newsts[jj]);
			newsts[jj]->chaining.type = otl_chaining_canonical;
			newsts[jj]->chaining.rule = lookup->subtables[j]->chaining.rule;
			jj += 1;
		}
	}
	lookup->subtableCount = totalRules;
	lookup->subtables = newsts;
}

static void expandChain(caryll_Font *font, otl_Lookup *lookup, table_OTL *table) {
	switch (lookup->type) {
		case otl_type_gsub_chaining:
		case otl_type_gpos_chaining:
			unconsolidate_chaining(font, lookup, table);
			break;
		default:
			break;
	}
}

static void expandChainingLookups(caryll_Font *font) {
	if (font->GSUB) {
		for (uint32_t j = 0; j < font->GSUB->lookupCount; j++) {
			otl_Lookup *lookup = font->GSUB->lookups[j];
			expandChain(font, lookup, font->GSUB);
		}
	}
	if (font->GPOS) {
		for (uint32_t j = 0; j < font->GPOS->lookupCount; j++) {
			otl_Lookup *lookup = font->GPOS->lookups[j];
			expandChain(font, lookup, font->GPOS);
		}
	}
}

static void merge_hmtx(caryll_Font *font) {
	// Merge hmtx table into glyf.
	if (font->hhea && font->hmtx && font->glyf) {
		uint32_t count_a = font->hhea->numberOfMetrics;
		for (glyphid_t j = 0; j < font->glyf->numberGlyphs; j++) {
			font->glyf->glyphs[j]->advanceWidth = font->hmtx->metrics[(j < count_a ? j : count_a - 1)].advanceWidth;
		}
	}
}
static void merge_vmtx(caryll_Font *font) {
	// Merge vmtx table into glyf.
	if (font->vhea && font->vmtx && font->glyf) {
		uint32_t count_a = font->vhea->numOfLongVerMetrics;
		for (glyphid_t j = 0; j < font->glyf->numberGlyphs; j++) {
			font->glyf->glyphs[j]->advanceHeight = font->vmtx->metrics[(j < count_a ? j : count_a - 1)].advanceHeight;
			if (j < count_a) {
				font->glyf->glyphs[j]->verticalOrigin = font->vmtx->metrics[j].tsb + font->glyf->glyphs[j]->stat.yMax;
			} else {
				font->glyf->glyphs[j]->verticalOrigin =
				    font->vmtx->topSideBearing[j - count_a] + font->glyf->glyphs[j]->stat.yMax;
			}
		}
		if (font->VORG) {
			for (glyphid_t j = 0; j < font->glyf->numberGlyphs; j++) {
				font->glyf->glyphs[j]->verticalOrigin = font->VORG->defaultVerticalOrigin;
			}
			for (glyphid_t j = 0; j < font->VORG->numVertOriginYMetrics; j++) {
				if (font->VORG->entries[j].gid < font->glyf->numberGlyphs) {
					font->glyf->glyphs[font->VORG->entries[j].gid]->verticalOrigin =
					    font->VORG->entries[j].verticalOrigin;
				}
			}
		}
	}
}
static void merge_LTSH(caryll_Font *font) {
	if (font->glyf && font->LTSH) {
		for (glyphid_t j = 0; j < font->glyf->numberGlyphs && j < font->LTSH->numGlyphs; j++) {
			font->glyf->glyphs[j]->yPel = font->LTSH->yPels[j];
		}
	}
}
void otfcc_unconsolidateFont(caryll_Font *font, const otfcc_Options *options) {
	// Merge metrics
	merge_hmtx(font);
	merge_vmtx(font);
	merge_LTSH(font);
	// expand chaining lookups
	expandChainingLookups(font);
	// Name glyphs
	caryll_name_glyphs(font, options);
	caryll_name_glyf(font);
}
