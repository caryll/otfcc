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

void caryll_font_consolidate(caryll_font *font, caryll_dump_options *dumpopts) {
	caryll_font_consolidate_glyf(font);
	caryll_font_consolidate_cmap(font);
}
