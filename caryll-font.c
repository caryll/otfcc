#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "caryll-sfnt.h"
#include "caryll-font.h"

caryll_font *caryll_font_new() {
	caryll_font *font = calloc(1, sizeof(caryll_font));
	if (!font) return NULL;
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
	font->fpgm = NULL;
	font->prep = NULL;
	font->cvt_ = NULL;
	return font;
}

caryll_font *caryll_font_open(caryll_sfnt *sfnt, uint32_t index) {
	if (sfnt->count - 1 < index)
		return NULL;
	else {
		caryll_font *font = caryll_font_new();
		caryll_packet packet = sfnt->packets[index];

		font->head = caryll_read_head(packet);
		font->maxp = caryll_read_maxp(packet);
		font->hhea = caryll_read_hhea(packet);
		font->name = caryll_read_name(packet);
		font->OS_2 = caryll_read_OS_2(packet);
		font->hmtx = caryll_read_hmtx(packet, font->hhea, font->maxp);
		font->post = caryll_read_post(packet);
		font->hdmx = caryll_read_hdmx(packet, font->maxp);
		font->glyf = caryll_read_glyf(packet, font->head, font->maxp);
		font->cmap = caryll_read_cmap(packet);
		font->fpgm = caryll_read_fpgm_prep(packet, 'fpgm');
		font->prep = caryll_read_fpgm_prep(packet, 'prep');
		font->cvt_ = caryll_read_fpgm_prep(packet, 'cvt ');

		caryll_font_unconsolidate(font);

		return font;
	}
}

void caryll_font_close(caryll_font *font) {
	if (font->glyf != NULL) caryll_delete_glyf(font->glyf);
	if (font->cmap != NULL) caryll_delete_cmap(font->cmap);
	if (font->head != NULL) free(font->head);
	if (font->hhea != NULL) free(font->hhea);
	if (font->maxp != NULL) free(font->maxp);
	if (font->OS_2 != NULL) free(font->OS_2);
	if (font->name != NULL) caryll_delete_name(font->name);
	if (font->hmtx != NULL) caryll_delete_hmtx(font->hmtx);
	if (font->post != NULL) caryll_delete_post(font->post);
	if (font->hdmx != NULL) caryll_delete_hdmx(font->hdmx);
	if (font->fpgm != NULL) caryll_delete_fpgm_prep(font->fpgm);
	if (font->prep != NULL) caryll_delete_fpgm_prep(font->prep);
	if (font->cvt_ != NULL) caryll_delete_fpgm_prep(font->cvt_);
	if (font->glyph_order && *font->glyph_order) { delete_glyph_order_map(font->glyph_order); }
	if (font != NULL) free(font);
}
