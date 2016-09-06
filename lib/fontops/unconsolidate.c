#include "unconsolidate.h"
#include <support/aglfn.h>
// Unconsolidation: Remove redundent data and de-couple internal data
// It does these things:
//   1. Merge hmtx data into glyf
//   2. Replace all glyph IDs into glyph names. Note all glyph references with
//      same name whare one unique string entity stored in font->glyph_order.
//      (Separate?)
static void caryll_name_glyphs(caryll_Font *font, const caryll_Options *options) {
	if (!font->glyf) return;
	glyphorder_Map *glyph_order = malloc(sizeof(glyphorder_Map));
	*glyph_order = NULL;

	glyphorder_Map *aglfn = malloc(sizeof(glyphorder_Map));
	*aglfn = NULL;
	aglfn_setupNames(aglfn);

	uint16_t numGlyphs = font->glyf->numberGlyphs;

	// pass 1: Map to existing glyph names
	for (uint16_t j = 0; j < numGlyphs; j++) {
		if (font->glyf->glyphs[j]->name) {
			glyphorder_tryAssignName(glyph_order, j, font->glyf->glyphs[j]->name);
			font->glyf->glyphs[j]->name = NULL;
		}
	}

	// pass 2: Map to `post` names
	if (font->post != NULL && font->post->post_name_map != NULL) {
		glyphorder_Entry *s;
		foreach_hash(s, *font->post->post_name_map) {
			glyphorder_tryAssignName(glyph_order, s->gid, sdsdup(s->name));
		}
	}

	// pass 3: Map to AGLFN & Unicode
	if (font->cmap != NULL) {
		cmap_Entry *s;
		foreach_hash(s, *font->cmap) if (s->glyph.index > 0) {
			sds name = NULL;
			if (s->unicode < 0x10000) glyphorder_nameAnIndex(aglfn, s->unicode, &name);
			if (name == NULL) {
				name = sdscatprintf(sdsempty(), "uni%04X", s->unicode);
			} else {
				name = sdsdup(name);
			}
			int actuallyNamed = glyphorder_tryAssignName(glyph_order, s->glyph.index, name);
			if (!actuallyNamed) sdsfree(name);
		}
	}

	// pass 4 : Map to GID
	for (uint16_t j = 0; j < numGlyphs; j++) {
		sds name;
		if (j) {
			name = sdscatfmt(sdsempty(), "glyph%u", j);
		} else {
			name = sdsnew(".notdef");
		}
		int actuallyNamed = glyphorder_tryAssignName(glyph_order, j, name);
		if (!actuallyNamed) sdsfree(name);
	}

	if (options->glyph_name_prefix) {
		glyphorder_Entry *item;
		foreach_hash(item, *glyph_order) {
			sds oldname = item->name;
			item->name = sdscatprintf(sdsempty(), "%s%s", options->glyph_name_prefix, oldname);
			sdsfree(oldname);
		}
	}

	glyphorder_deleteMap(aglfn);
	font->glyph_order = glyph_order;
}

static void caryll_name_cmap_entries(caryll_Font *font) {
	if (font->glyph_order != NULL && font->cmap != NULL) {
		cmap_Entry *s;
		foreach_hash(s, *font->cmap) {
			glyphorder_nameAIndexedHandle(font->glyph_order, &s->glyph);
		}
	}
}
static void caryll_name_glyf(caryll_Font *font) {
	if (font->glyph_order != NULL && font->glyf != NULL) {
		for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
			glyf_Glyph *g = font->glyf->glyphs[j];
			sds glyphName = NULL;
			glyphorder_nameAnIndex(font->glyph_order, j, &glyphName);
			g->name = sdsdup(glyphName);
			if (g->numberOfReferences > 0 && g->references != NULL) {
				for (uint16_t k = 0; k < g->numberOfReferences; k++) {
					glyphorder_nameAIndexedHandle(font->glyph_order, &g->references[k].glyph);
				}
			}
		}
	}
}
static void name_coverage(caryll_Font *font, otl_Coverage *coverage) {
	if (!coverage) return;
	for (uint16_t j = 0; j < coverage->numGlyphs; j++) {
		glyphorder_nameAIndexedHandle(font->glyph_order, &coverage->glyphs[j]);
	}
}
static void name_classdef(caryll_Font *font, otl_ClassDef *cd) {
	if (!cd) return;
	for (uint16_t j = 0; j < cd->numGlyphs; j++) {
		glyphorder_nameAIndexedHandle(font->glyph_order, &cd->glyphs[j]);
	}
}
static void unconsolidate_chaining(caryll_Font *font, otl_Lookup *lookup, table_OTL *table) {
	uint16_t totalRules = 0;
	for (uint16_t j = 0; j < lookup->subtableCount; j++)
		if (lookup->subtables[j]) { totalRules += lookup->subtables[j]->chaining.rulesCount; }
	otl_Subtable **newsts;
	NEW_N(newsts, totalRules);
	uint16_t jj = 0;
	for (uint16_t j = 0; j < lookup->subtableCount; j++)
		if (lookup->subtables[j]) {
			for (uint16_t k = 0; k < lookup->subtables[j]->chaining.rulesCount; k++) {
				NEW(newsts[jj]);
				newsts[jj]->chaining.rulesCount = 1;
				newsts[jj]->chaining.bc = NULL;
				newsts[jj]->chaining.ic = NULL;
				newsts[jj]->chaining.fc = NULL;
				NEW(newsts[jj]->chaining.rules);
				newsts[jj]->chaining.rules[0] = lookup->subtables[j]->chaining.rules[k];
				jj += 1;
			}
			free(lookup->subtables[j]->chaining.rules);
			free(lookup->subtables[j]);
		}
	lookup->subtableCount = totalRules;
	lookup->subtables = newsts;
	for (uint16_t j = 0; j < lookup->subtableCount; j++) {
		subtable_chaining *subtable = &(lookup->subtables[j]->chaining);
		otl_ChainingRule *rule = subtable->rules[0];
		for (uint16_t k = 0; k < rule->matchCount; k++) {
			name_coverage(font, rule->match[k]);
		}
		for (uint16_t k = 0; k < rule->applyCount; k++) {
			if (rule->apply[k].lookup.index < table->lookupCount) {
				rule->apply[k].lookup.name = table->lookups[rule->apply[k].lookup.index]->name;
				rule->apply[k].lookup.state = HANDLE_STATE_CONSOLIDATED;
			} else {
				rule->apply[k].lookup.index = 0;
				rule->apply[k].lookup.name = table->lookups[rule->apply[k].lookup.index]->name;
				rule->apply[k].lookup.state = HANDLE_STATE_CONSOLIDATED;
			}
		}
	}
}

static void unconsolidate_gsub_reverse(caryll_Font *font, otl_Lookup *lookup, table_OTL *table) {
	for (uint16_t j = 0; j < lookup->subtableCount; j++)
		if (lookup->subtables[j]) {
			subtable_gsub_reverse *subtable = &(lookup->subtables[j]->gsub_reverse);
			for (uint16_t j = 0; j < subtable->matchCount; j++) {
				name_coverage(font, subtable->match[j]);
			}
			name_coverage(font, subtable->to);
		}
}

static void name_lookup(caryll_Font *font, otl_Lookup *lookup, table_OTL *table) {
	switch (lookup->type) {
		case otl_type_gsub_single:
			for (uint16_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) {
					name_coverage(font, lookup->subtables[j]->gsub_single.from);
					name_coverage(font, lookup->subtables[j]->gsub_single.to);
				}
			break;
		case otl_type_gsub_multiple:
		case otl_type_gsub_alternate:
			for (uint16_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) {
					name_coverage(font, lookup->subtables[j]->gsub_multi.from);
					for (uint16_t k = 0; k < lookup->subtables[j]->gsub_multi.from->numGlyphs; k++) {
						name_coverage(font, lookup->subtables[j]->gsub_multi.to[k]);
					}
				}
			break;
		case otl_type_gsub_ligature:
			for (uint16_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) {
					name_coverage(font, lookup->subtables[j]->gsub_ligature.to);
					for (uint16_t k = 0; k < lookup->subtables[j]->gsub_ligature.to->numGlyphs; k++) {
						name_coverage(font, lookup->subtables[j]->gsub_ligature.from[k]);
					}
				}
			break;
		case otl_type_gsub_chaining:
		case otl_type_gpos_chaining:
			unconsolidate_chaining(font, lookup, table);
			break;
		case otl_type_gsub_reverse:
			unconsolidate_gsub_reverse(font, lookup, table);
			break;
		case otl_type_gpos_single:
			for (uint16_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) { name_coverage(font, lookup->subtables[j]->gpos_single.coverage); }
			break;
		case otl_type_gpos_pair:
			for (uint16_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) {
					name_classdef(font, lookup->subtables[j]->gpos_pair.first);
					name_classdef(font, lookup->subtables[j]->gpos_pair.second);
				}
			break;
		case otl_type_gpos_cursive:
			for (uint16_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) { name_coverage(font, lookup->subtables[j]->gpos_cursive.coverage); }
			break;
		case otl_type_gpos_markToBase:
		case otl_type_gpos_markToMark:
			for (uint16_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) {
					name_coverage(font, lookup->subtables[j]->gpos_markToSingle.marks);
					name_coverage(font, lookup->subtables[j]->gpos_markToSingle.bases);
				}
			break;
		case otl_type_gpos_markToLigature:
			for (uint16_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) {
					name_coverage(font, lookup->subtables[j]->gpos_markToLigature.marks);
					name_coverage(font, lookup->subtables[j]->gpos_markToLigature.bases);
				}
			break;
		default:
			break;
	}
}

static void caryll_name_features(caryll_Font *font) {
	if (font->glyph_order && font->GSUB) {
		for (uint32_t j = 0; j < font->GSUB->lookupCount; j++) {
			otl_Lookup *lookup = font->GSUB->lookups[j];
			name_lookup(font, lookup, font->GSUB);
		}
	}
	if (font->glyph_order && font->GPOS) {
		for (uint32_t j = 0; j < font->GPOS->lookupCount; j++) {
			otl_Lookup *lookup = font->GPOS->lookups[j];
			name_lookup(font, lookup, font->GPOS);
		}
	}
	if (font->glyph_order && font->GDEF) {
		name_classdef(font, font->GDEF->glyphClassDef);
		name_classdef(font, font->GDEF->markAttachClassDef);
		if (font->GDEF->ligCarets) { name_coverage(font, font->GDEF->ligCarets->coverage); }
	}
}

static void caryll_name_fdselect(caryll_Font *font) {
	if (font->CFF_ && font->glyf && font->CFF_->fdArray && font->CFF_->fdArrayCount) {
		for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
			glyf_Glyph *g = font->glyf->glyphs[j];
			if (g->fdSelect.index < font->CFF_->fdArrayCount) {
				g->fdSelect.name = font->CFF_->fdArray[g->fdSelect.index]->fontName;
				g->fdSelect.state = HANDLE_STATE_CONSOLIDATED;
			} else {
				g->fdSelect.index = 0;
				g->fdSelect.name = font->CFF_->fdArray[g->fdSelect.index]->fontName;
				g->fdSelect.state = HANDLE_STATE_CONSOLIDATED;
			}
		}
	}
}

static void merge_hmtx(caryll_Font *font) {
	// Merge hmtx table into glyf.
	if (font->hhea && font->hmtx && font->glyf) {
		uint32_t count_a = font->hhea->numberOfMetrics;
		for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
			font->glyf->glyphs[j]->advanceWidth = font->hmtx->metrics[(j < count_a ? j : count_a - 1)].advanceWidth;
		}
	}
}
static void merge_vmtx(caryll_Font *font) {
	// Merge vmtx table into glyf.
	if (font->vhea && font->vmtx && font->glyf) {
		uint32_t count_a = font->vhea->numOfLongVerMetrics;
		for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
			font->glyf->glyphs[j]->advanceHeight = font->vmtx->metrics[(j < count_a ? j : count_a - 1)].advanceHeight;
			if (j < count_a) {
				font->glyf->glyphs[j]->verticalOrigin = font->vmtx->metrics[j].tsb + font->glyf->glyphs[j]->stat.yMax;
			} else {
				font->glyf->glyphs[j]->verticalOrigin =
				    font->vmtx->topSideBearing[j - count_a] + font->glyf->glyphs[j]->stat.yMax;
			}
		}
		if (font->VORG) {
			for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
				font->glyf->glyphs[j]->verticalOrigin = font->VORG->defaultVerticalOrigin;
			}
			for (uint16_t j = 0; j < font->VORG->numVertOriginYMetrics; j++) {
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
		for (uint16_t j = 0; j < font->glyf->numberGlyphs && j < font->LTSH->numGlyphs; j++) {
			font->glyf->glyphs[j]->yPel = font->LTSH->yPels[j];
		}
	}
}
void caryll_font_unconsolidate(caryll_Font *font, const caryll_Options *options) {
	// Merge metrics
	merge_hmtx(font);
	merge_vmtx(font);
	merge_LTSH(font);

	// Name glyphs
	caryll_name_glyphs(font, options);
	caryll_name_cmap_entries(font);
	caryll_name_glyf(font);
	caryll_name_features(font);
	caryll_name_fdselect(font);
}
