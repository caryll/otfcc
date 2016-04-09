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

static void __declare_consolidate_type(otl_lookup_type type,
                                       bool (*fn)(caryll_font *, table_otl *, otl_subtable *, sds),
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
#define LOOKUP_CONSOLIDATOR(llt, fn) __declare_consolidate_type(llt, fn, font, table, lookup);
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
	if (font->glyph_order && font->GDEF) {
		if (font->GDEF->glyphClassDef)
			consolidate_classdef(font, font->GDEF->glyphClassDef, "GDEF");
	}
}

void caryll_font_consolidate(caryll_font *font, caryll_dump_options *dumpopts) {
	caryll_font_consolidate_glyf(font);
	caryll_font_consolidate_cmap(font);
	caryll_font_consolidate_otl(font);
}
