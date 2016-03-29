#include "unconsolidate.h"
#include "../support/aglfn.h"
// Unconsolidation: Remove redundent data and de-couple internal data
// It does these things:
//   1. Merge hmtx data into glyf
//   2. Replace all glyph IDs into glyph names. Note all glyph references with
//      same name whare one unique string entity stored in font->glyph_order.
//      (Separate?)
void caryll_name_glyphs(caryll_font *font) {
	glyph_order_hash *glyph_order = malloc(sizeof(glyph_order_hash));
	*glyph_order = NULL;

	glyph_order_hash *aglfn = malloc(sizeof(glyph_order_hash));
	*aglfn = NULL;
	setup_aglfn_glyph_names(aglfn);

	uint16_t numGlyphs = font->glyf->numberGlyphs;

	// pass 1: Map to `post` names
	if (font->post != NULL && font->post->post_name_map != NULL) {
		glyph_order_entry *s;
		foreach_hash(s, *font->post->post_name_map) { try_name_glyph(glyph_order, s->gid, sdsdup(s->name)); }
	}
	// pass 2: Map to AGLFN & Unicode
	if (font->cmap != NULL) {
		cmap_entry *s;
		foreach_hash(s, *font->cmap) if (s->glyph.gid > 0) {
			sds name = NULL;
			lookup_name(aglfn, s->unicode, &name);
			if (name == NULL) {
				name = sdscatprintf(sdsempty(), "uni%04X", s->unicode);
			} else {
				name = sdsdup(name);
			}
			int actuallyNamed = try_name_glyph(glyph_order, s->glyph.gid, name);
			if (!actuallyNamed) sdsfree(name);
		}
	}
	// pass 3: Map to GID
	for (uint16_t j = 0; j < numGlyphs; j++) {
		sds name = sdscatfmt(sdsempty(), "glyph%u", j);
		int actuallyNamed = try_name_glyph(glyph_order, j, name);
		if (!actuallyNamed) sdsfree(name);
	}

	delete_glyph_order_map(aglfn);

	font->glyph_order = glyph_order;
}

void caryll_name_cmap_entries(caryll_font *font) {
	if (font->glyph_order != NULL && font->cmap != NULL) {
		cmap_entry *s;
		foreach_hash(s, *font->cmap) { lookup_name(font->glyph_order, s->glyph.gid, &s->glyph.name); }
	}
}
void caryll_name_glyf(caryll_font *font) {
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
void caryll_font_unconsolidate(caryll_font *font) {
	// Merge hmtx table into glyf.
	uint32_t count_a = font->hhea->numberOfMetrics;
	for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
		font->glyf->glyphs[j]->advanceWidth = font->hmtx->metrics[(j < count_a ? j : count_a - 1)].advanceWidth;
	}

	// Name glyphs
	caryll_name_glyphs(font);
	caryll_name_cmap_entries(font);
	caryll_name_glyf(font);
}
