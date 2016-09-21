#include "unconsolidate.h"
// Unconsolidation: Remove redundent data and de-couple internal data
// It does these things:
//   1. Merge hmtx data into glyf
//   2. Replace all glyph IDs into glyph names. Note all glyph references with
//      same name whare one unique string entity stored in font->glyph_order.
//      (Separate?)
static void caryll_name_glyphs(caryll_Font *font, const caryll_Options *options) {
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
			caryll_nameAFieldUsingGlyphOrder(aglfn, s->unicode, &name);
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
			name = sdscatfmt(sdsempty(), "glyph%u", j);
		} else {
			name = sdsnew(".notdef");
		}
		caryll_setGlyphOrderByGID(glyph_order, j, name);
	}

	caryll_delete_GlyphOrder(aglfn);
	font->glyph_order = glyph_order;
}

static void caryll_name_cmap_entries(caryll_Font *font) {
	if (font->glyph_order != NULL && font->cmap != NULL) {
		cmap_Entry *s;
		foreach_hash(s, *font->cmap) {
			caryll_nameAHandleUsingGlyphOrder(font->glyph_order, &s->glyph);
		}
	}
}
static void caryll_name_glyf(caryll_Font *font) {
	if (font->glyph_order != NULL && font->glyf != NULL) {
		for (glyphid_t j = 0; j < font->glyf->numberGlyphs; j++) {
			glyf_Glyph *g = font->glyf->glyphs[j];
			sds glyphName = NULL;
			caryll_nameAFieldUsingGlyphOrder(font->glyph_order, j, &glyphName);
			g->name = sdsdup(glyphName);
			if (g->numberOfReferences > 0 && g->references != NULL) {
				for (shapeid_t k = 0; k < g->numberOfReferences; k++) {
					caryll_nameAHandleUsingGlyphOrder(font->glyph_order, &g->references[k].glyph);
				}
			}
		}
	}
}
static void name_coverage(caryll_Font *font, otl_Coverage *coverage) {
	if (!coverage) return;
	for (glyphid_t j = 0; j < coverage->numGlyphs; j++) {
		caryll_nameAHandleUsingGlyphOrder(font->glyph_order, &coverage->glyphs[j]);
	}
}
static void name_classdef(caryll_Font *font, otl_ClassDef *cd) {
	if (!cd) return;
	for (glyphid_t j = 0; j < cd->numGlyphs; j++) {
		caryll_nameAHandleUsingGlyphOrder(font->glyph_order, &cd->glyphs[j]);
	}
}
static void unconsolidate_chaining(caryll_Font *font, otl_Lookup *lookup, table_OTL *table) {
	tableid_t totalRules = 0;
	for (tableid_t j = 0; j < lookup->subtableCount; j++)
		if (lookup->subtables[j]) { totalRules += lookup->subtables[j]->chaining.rulesCount; }
	otl_Subtable **newsts;
	NEW_N(newsts, totalRules);
	tableid_t jj = 0;
	for (tableid_t j = 0; j < lookup->subtableCount; j++)
		if (lookup->subtables[j]) {
			for (tableid_t k = 0; k < lookup->subtables[j]->chaining.rulesCount; k++) {
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
	for (tableid_t j = 0; j < lookup->subtableCount; j++) {
		subtable_chaining *subtable = &(lookup->subtables[j]->chaining);
		otl_ChainingRule *rule = subtable->rules[0];
		for (tableid_t k = 0; k < rule->matchCount; k++) {
			name_coverage(font, rule->match[k]);
		}
		for (tableid_t k = 0; k < rule->applyCount; k++) {
			if (rule->apply[k].lookup.index >= table->lookupCount) { rule->apply[k].lookup.index = 0; }
			rule->apply[k].lookup =
			    handle_fromConsolidated(rule->apply[k].lookup.index, table->lookups[rule->apply[k].lookup.index]->name);
		}
	}
}

static void unconsolidate_gsub_reverse(caryll_Font *font, otl_Lookup *lookup, table_OTL *table) {
	for (tableid_t j = 0; j < lookup->subtableCount; j++)
		if (lookup->subtables[j]) {
			subtable_gsub_reverse *subtable = &(lookup->subtables[j]->gsub_reverse);
			for (tableid_t j = 0; j < subtable->matchCount; j++) {
				name_coverage(font, subtable->match[j]);
			}
			name_coverage(font, subtable->to);
		}
}

static void name_lookup(caryll_Font *font, otl_Lookup *lookup, table_OTL *table) {
	switch (lookup->type) {
		case otl_type_gsub_single:
			for (tableid_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) {
					name_coverage(font, lookup->subtables[j]->gsub_single.from);
					name_coverage(font, lookup->subtables[j]->gsub_single.to);
				}
			break;
		case otl_type_gsub_multiple:
		case otl_type_gsub_alternate:
			for (tableid_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) {
					name_coverage(font, lookup->subtables[j]->gsub_multi.from);
					for (glyphid_t k = 0; k < lookup->subtables[j]->gsub_multi.from->numGlyphs; k++) {
						name_coverage(font, lookup->subtables[j]->gsub_multi.to[k]);
					}
				}
			break;
		case otl_type_gsub_ligature:
			for (tableid_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) {
					name_coverage(font, lookup->subtables[j]->gsub_ligature.to);
					for (glyphid_t k = 0; k < lookup->subtables[j]->gsub_ligature.to->numGlyphs; k++) {
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
			for (tableid_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) { name_coverage(font, lookup->subtables[j]->gpos_single.coverage); }
			break;
		case otl_type_gpos_pair:
			for (tableid_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) {
					name_classdef(font, lookup->subtables[j]->gpos_pair.first);
					name_classdef(font, lookup->subtables[j]->gpos_pair.second);
				}
			break;
		case otl_type_gpos_cursive:
			for (tableid_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) { name_coverage(font, lookup->subtables[j]->gpos_cursive.coverage); }
			break;
		case otl_type_gpos_markToBase:
		case otl_type_gpos_markToMark:
			for (tableid_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) {
					name_coverage(font, lookup->subtables[j]->gpos_markToSingle.marks);
					name_coverage(font, lookup->subtables[j]->gpos_markToSingle.bases);
				}
			break;
		case otl_type_gpos_markToLigature:
			for (tableid_t j = 0; j < lookup->subtableCount; j++)
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
		for (glyphid_t j = 0; j < font->glyf->numberGlyphs; j++) {
			glyf_Glyph *g = font->glyf->glyphs[j];
			if (g->fdSelect.index >= font->CFF_->fdArrayCount) { g->fdSelect.index = 0; }
			g->fdSelect = handle_fromConsolidated(g->fdSelect.index, font->CFF_->fdArray[g->fdSelect.index]->fontName);
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
static double qround(double x) {
	return round(x * 65536.0) / (65536.0);
}
static void applyCffMatrix(caryll_Font *font) {
	if (!font || !font->head || !font->glyf || !font->CFF_) return;
	for (glyphid_t jj = 0; jj < font->glyf->numberGlyphs; jj++) {
		glyf_Glyph *g = font->glyf->glyphs[jj];
		table_CFF *fd = font->CFF_;
		if (fd->fdArray && g->fdSelect.index < fd->fdArrayCount) { fd = fd->fdArray[g->fdSelect.index]; }
		if (fd->fontMatrix) {
			pos_t a = qround(font->head->unitsPerEm * fd->fontMatrix->a);
			pos_t b = qround(font->head->unitsPerEm * fd->fontMatrix->b);
			pos_t c = qround(font->head->unitsPerEm * fd->fontMatrix->c);
			pos_t d = qround(font->head->unitsPerEm * fd->fontMatrix->d);
			pos_t x = qround(font->head->unitsPerEm * fd->fontMatrix->x);
			pos_t y = qround(font->head->unitsPerEm * fd->fontMatrix->y);
			for (shapeid_t j = 0; j < g->numberOfContours; j++) {
				for (shapeid_t k = 0; k < g->contours[j].pointsCount; k++) {
					pos_t zx = g->contours[j].points[k].x;
					pos_t zy = g->contours[j].points[k].y;
					g->contours[j].points[k].x = a * zx + b * zy + x;
					g->contours[j].points[k].y = c * zx + d * zy + y;
				}
			}
		}
	}
}
void caryll_font_unconsolidate(caryll_Font *font, const caryll_Options *options) {
	// Apply CFF FontMatrix
	applyCffMatrix(font);
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
