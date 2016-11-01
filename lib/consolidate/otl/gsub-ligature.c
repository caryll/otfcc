#include "gsub-ligature.h"

bool consolidate_gsub_ligature(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable,
                               const otfcc_Options *options) {
	subtable_gsub_ligature *subtable = &(_subtable->gsub_ligature);
	subtable_gsub_ligature *nt = otl_new_gsub_ligature();
	for (glyphid_t k = 0; k < subtable->length; k++) {
		if (!GlyphOrder.consolidateHandle(font->glyph_order, &subtable->data[k].to)) {
			logWarning("[Consolidate] Ignored missing glyph /%s.\n", subtable->data[k].to.name);
			continue;
		}
		fontop_consolidateCoverage(font, subtable->data[k].from, options);
		Coverage.shrink(subtable->data[k].from, false);
		caryll_vecPush(nt, ((otl_GsubLigatureEntry){
		                       .from = subtable->data[k].from, .to = Handle.copy(subtable->data[k].to),
		                   }));
		subtable->data[k].from = NULL;
	}
	caryll_vecReplace(subtable, nt);
	return (subtable->length == 0);
}
