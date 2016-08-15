#include "unconsolidate.h"
#include <support/aglfn.h>
// Unconsolidation: Remove redundent data and de-couple internal data
// It does these things:
//   1. Merge hmtx data into glyf
//   2. Replace all glyph IDs into glyph names. Note all glyph references with
//      same name whare one unique string entity stored in font->glyph_order.
//      (Separate?)
static void caryll_name_glyphs(caryll_font *font, caryll_options *options) {
	if (!font->glyf) return;
	glyph_order_hash *glyph_order = malloc(sizeof(glyph_order_hash));
	*glyph_order = NULL;

	glyph_order_hash *aglfn = malloc(sizeof(glyph_order_hash));
	*aglfn = NULL;
	setup_aglfn_glyph_names(aglfn);

	uint16_t numGlyphs = font->glyf->numberGlyphs;

	// pass 1: Map to existing glyph names
	for (uint16_t j = 0; j < numGlyphs; j++) {
		if (font->glyf->glyphs[j]->name) {
			try_name_glyph(glyph_order, j, font->glyf->glyphs[j]->name);
			font->glyf->glyphs[j]->name = NULL;
		}
	}

	// pass 2: Map to `post` names
	if (font->post != NULL && font->post->post_name_map != NULL) {
		glyph_order_entry *s;
		foreach_hash(s, *font->post->post_name_map) {
			try_name_glyph(glyph_order, s->gid, sdsdup(s->name));
		}
	}
	// pass 3: Map to AGLFN & Unicode
	if (font->cmap != NULL) {
		cmap_entry *s;
		foreach_hash(s, *font->cmap) if (s->glyph.gid > 0) {
			sds name = NULL;
			if (s->unicode < 0x10000) lookup_name(aglfn, s->unicode, &name);
			if (name == NULL) {
				name = sdscatprintf(sdsempty(), "uni%04X", s->unicode);
			} else {
				name = sdsdup(name);
			}
			int actuallyNamed = try_name_glyph(glyph_order, s->glyph.gid, name);
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
		int actuallyNamed = try_name_glyph(glyph_order, j, name);
		if (!actuallyNamed) sdsfree(name);
	}

	if (options->glyph_name_prefix) {
		glyph_order_entry *item;
		foreach_hash(item, *glyph_order) {
			sds oldname = item->name;
			item->name = sdscatprintf(sdsempty(), "%s%s", options->glyph_name_prefix, oldname);
			sdsfree(oldname);
		}
	}

	delete_glyph_order_map(aglfn);
	font->glyph_order = glyph_order;
}

static void caryll_name_cmap_entries(caryll_font *font) {
	if (font->glyph_order != NULL && font->cmap != NULL) {
		cmap_entry *s;
		foreach_hash(s, *font->cmap) {
			lookup_name(font->glyph_order, s->glyph.gid, &s->glyph.name);
		}
	}
}
static void caryll_name_glyf(caryll_font *font) {
	if (font->glyph_order != NULL && font->glyf != NULL) {
		for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
			glyf_glyph *g = font->glyf->glyphs[j];
			lookup_name(font->glyph_order, j, &g->name);
			if (g->numberOfReferences > 0 && g->references != NULL) {
				for (uint16_t k = 0; k < g->numberOfReferences; k++) {
					lookup_name(font->glyph_order, g->references[k].glyph.gid, &g->references[k].glyph.name);
				}
			}
		}
	}
}
static void name_coverage(caryll_font *font, otl_coverage *coverage) {
	if (!coverage) return;
	for (uint16_t j = 0; j < coverage->numGlyphs; j++) {
		lookup_name(font->glyph_order, coverage->glyphs[j].gid, &(coverage->glyphs[j].name));
	}
}
static void name_classdef(caryll_font *font, otl_classdef *cd) {
	if (!cd) return;
	for (uint16_t j = 0; j < cd->numGlyphs; j++) {
		lookup_name(font->glyph_order, cd->glyphs[j].gid, &(cd->glyphs[j].name));
	}
}
static void unconsolidate_gsub_chain(caryll_font *font, otl_lookup *lookup, table_otl *table) {
	uint16_t totalRules = 0;
	for (uint16_t j = 0; j < lookup->subtableCount; j++)
		if (lookup->subtables[j]) { totalRules += lookup->subtables[j]->chaining.rulesCount; }
	otl_subtable **newsts;
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
		otl_chaining_rule *rule = subtable->rules[0];
		for (uint16_t k = 0; k < rule->matchCount; k++) {
			name_coverage(font, rule->match[k]);
		}
		for (uint16_t k = 0; k < rule->applyCount; k++)
			if (rule->apply[k].lookup.index < table->lookupCount) {
				rule->apply[k].lookup.name = table->lookups[rule->apply[k].lookup.index]->name;
			}
	}
}

static void unconsolidate_gsub_reverse(caryll_font *font, otl_lookup *lookup, table_otl *table) {
	for (uint16_t j = 0; j < lookup->subtableCount; j++)
		if (lookup->subtables[j]) {
			subtable_gsub_reverse *subtable = &(lookup->subtables[j]->gsub_reverse);
			for (uint16_t j = 0; j < subtable->matchCount; j++) {
				name_coverage(font, subtable->match[j]);
			}
			name_coverage(font, subtable->to);
		}
}

static void name_lookup(caryll_font *font, otl_lookup *lookup, table_otl *table) {
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
			unconsolidate_gsub_chain(font, lookup, table);
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
		case otl_type_gpos_mark_to_base:
		case otl_type_gpos_mark_to_mark:
			for (uint16_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) {
					name_coverage(font, lookup->subtables[j]->gpos_mark_to_single.marks);
					name_coverage(font, lookup->subtables[j]->gpos_mark_to_single.bases);
				}
			break;
		case otl_type_gpos_mark_to_ligature:
			for (uint16_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) {
					name_coverage(font, lookup->subtables[j]->gpos_mark_to_ligature.marks);
					name_coverage(font, lookup->subtables[j]->gpos_mark_to_ligature.bases);
				}
			break;
		default:
			break;
	}
}

static void caryll_name_features(caryll_font *font) {
	if (font->glyph_order && font->GSUB) {
		for (uint32_t j = 0; j < font->GSUB->lookupCount; j++) {
			otl_lookup *lookup = font->GSUB->lookups[j];
			name_lookup(font, lookup, font->GSUB);
		}
	}
	if (font->glyph_order && font->GPOS) {
		for (uint32_t j = 0; j < font->GPOS->lookupCount; j++) {
			otl_lookup *lookup = font->GPOS->lookups[j];
			name_lookup(font, lookup, font->GPOS);
		}
	}
	if (font->glyph_order && font->GDEF) {
		name_classdef(font, font->GDEF->glyphClassDef);
		name_classdef(font, font->GDEF->markAttachClassDef);
		if (font->GDEF->ligCarets) { name_coverage(font, font->GDEF->ligCarets->coverage); }
	}
}

static void caryll_name_fdselect(caryll_font *font) {
	if (font->CFF_ && font->glyf && font->CFF_->fdArray) {
		for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
			glyf_glyph *g = font->glyf->glyphs[j];
			if (g->fdSelect.index < font->CFF_->fdArrayCount) {
				g->fdSelect.name = font->CFF_->fdArray[g->fdSelect.index]->fontName;
			}
		}
	}
}

static void merge_hmtx(caryll_font *font) {
	// Merge hmtx table into glyf.
	if (font->hhea && font->hmtx && font->glyf) {
		uint32_t count_a = font->hhea->numberOfMetrics;
		for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
			font->glyf->glyphs[j]->advanceWidth = font->hmtx->metrics[(j < count_a ? j : count_a - 1)].advanceWidth;
		}
	}
}
static void merge_vmtx(caryll_font *font) {
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
static void merge_LTSH(caryll_font *font) {
	if (font->glyf && font->LTSH) {
		for (uint16_t j = 0; j < font->glyf->numberGlyphs && j < font->LTSH->numGlyphs; j++) {
			font->glyf->glyphs[j]->yPel = font->LTSH->yPels[j];
		}
	}
}
void caryll_font_unconsolidate(caryll_font *font, caryll_options *options) {
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
