#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "caryll-io.h"
#include "caryll-sfnt.h"
#include "caryll-font.h"

// Unconsolidation: Remove redundent data and de-couple internal data
// It does these things:
//   1. Merge hmtx data into glyf
//   2. Replace all glyph IDs into glyph names. Note all glyph references with
//      same name whare one unique string entity stored in font->glyph_order.
//      (Separate?)
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

caryll_font *caryll_font_open(caryll_sfnt *sfnt, uint32_t index) {
	if (sfnt->count - 1 < index)
		return NULL;
	else {
		caryll_font *font = (caryll_font *)malloc(sizeof(caryll_font) * 1);
		caryll_packet packet = sfnt->packets[index];

		font->head = NULL;
		font->hhea = NULL;
		font->maxp = NULL;
		font->hmtx = NULL;
		font->post = NULL;
		font->name = NULL;
		font->hdmx = NULL;
		font->glyf = NULL;
		font->cmap = NULL;
		font->glyph_order = NULL;

		caryll_read_head(font, packet);
		caryll_read_hhea(font, packet);
		caryll_read_maxp(font, packet);
		caryll_read_OS_2(font, packet);
		caryll_read_hmtx(font, packet);
		caryll_read_post(font, packet);
		caryll_read_hdmx(font, packet);
		caryll_read_glyf(font, packet);
		caryll_read_cmap(font, packet);
		caryll_read_name(font, packet);

		caryll_font_unconsolidate(font);

		return font;
	}
}

void caryll_font_close(caryll_font *font) {
	if (font->glyf != NULL) caryll_delete_table_glyf(font);
	if (font->cmap != NULL) caryll_delete_table_cmap(font);
	if (font->glyph_order != NULL) { delete_glyph_order_map(font->glyph_order); }
	if (font->head != NULL) free(font->head);
	if (font->hhea != NULL) free(font->hhea);
	if (font->maxp != NULL) free(font->maxp);
	if (font->OS_2 != NULL) free(font->OS_2);
	if (font->name != NULL) caryll_delete_table_name(font);
	if (font->hmtx != NULL) caryll_delete_table_hmtx(font);
	if (font->post != NULL) caryll_delete_table_post(font);
	if (font->hdmx != NULL) caryll_delete_table_hdmx(font);
	if (font != NULL) free(font);
}
