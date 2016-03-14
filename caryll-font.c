#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "caryll-sfnt.h"
#include "caryll-font.h"
#include "caryll-io.h"

void caryll_font_consolidate_read(caryll_font *font) {
	// Merge hmtx table into glyf.
	uint32_t count_a = font->hhea->numberOfMetrics;
	for(uint16_t j = 0; j < font->glyf->numberGlyphs; j++){
		font->glyf->glyphs[j].advanceWidth = font->hmtx->metrics[(j < count_a ? j : count_a - 1)].advanceWidth;
	}
}

void caryll_read_table(caryll_font font, caryll_packet packet, uint32_t name, void (*handler)(caryll_piece piece)){
	for (uint32_t i = 0; i < packet.numTables; i++) {
		if (packet.pieces[i].tag == name) {
			handler(packet.pieces[i]);
		}
	}
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
		font->hdmx = NULL;
		font->glyf = NULL;
		font->cmap = NULL;

		caryll_read_head(font, packet);
		caryll_read_hhea(font, packet);
		caryll_read_maxp(font, packet);
		caryll_read_OS_2(font, packet);
		caryll_read_hmtx(font, packet);
		caryll_read_post(font, packet);
		caryll_read_hdmx(font, packet);
		caryll_read_glyf(font, packet);
		caryll_read_cmap(font, packet);

		caryll_font_consolidate_read(font);

		return font;
	}
}

void caryll_font_close(caryll_font *font) {
	if (font->glyf != NULL) {
		for (uint16_t j = 0; j < font->glyf->numberGlyphs; j++) {
			glyf_glyph g = font->glyf->glyphs[j];
			if (g.numberOfContours > 0) {
				for (uint16_t k = 0; k < g.numberOfContours; k++) {
					free(g.content.contours[k].points);
				}
				free(g.content.contours);
			} else if (g.numberOfReferences > 0) {
				free(g.content.references);
			}
			if (g.instructions != NULL) { free(g.instructions); }
		}
		free(font->glyf->glyphs);
		free(font->glyf);
	}
	if(font->cmap != NULL){
		cmap_entry *s, *tmp;
		HASH_ITER(hh, *(font->cmap), s, tmp) {
			// delete and free all cmap entries
			HASH_DEL(*(font->cmap), s);
			free(s);
		}
		free(font->cmap);
	}
	if (font->head != NULL) free(font->head);
	if (font->hhea != NULL) free(font->hhea);
	if (font->maxp != NULL) free(font->maxp);
	if (font->OS_2 != NULL) free(font->OS_2);
	if (font->hmtx != NULL) {
		if (font->hmtx->metrics != NULL) free(font->hmtx->metrics);
		if (font->hmtx->leftSideBearing != NULL) free(font->hmtx->leftSideBearing);
		free(font->hmtx);
	}
	if (font->post != NULL) free(font->post);
	if (font->hdmx != NULL) {
		if (font->hdmx->records != NULL) {
			for (uint32_t i = 0; i < font->hdmx->numRecords; i++) {
				if (font->hdmx->records[i].widths != NULL) free(font->hdmx->records[i].widths);
			}
			free(font->hdmx->records);
		}
		free(font->hdmx);
	}
	if (font != NULL) free(font);
}
