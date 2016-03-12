#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "caryll-sfnt.h"
#include "caryll-font.h"
#include "caryll-io.h"

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

		caryll_read_head(font, packet);
		caryll_read_hhea(font, packet);
		caryll_read_maxp(font, packet);
		caryll_read_OS_2(font, packet);
		caryll_read_hmtx(font, packet);
		caryll_read_post(font, packet);
		caryll_read_hdmx(font, packet);

		return font;
	}
}

void caryll_font_close(caryll_font *font) {
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
				if (font->hdmx->records[i].widths != NULL)
					;
				free(font->hdmx->records[i].widths);
			}
			free(font->hdmx->records);
		}
		free(font->hdmx);
	}
	if (font != NULL) free(font);
}
