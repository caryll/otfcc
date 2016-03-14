#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

void caryll_read_hmtx(caryll_font *font, caryll_packet packet) {
	for (uint32_t i = 0; i < packet.numTables; i++) {
		if (packet.pieces[i].tag == 'hmtx') {
			font_file_pointer data = packet.pieces[i].data;

			if (font->hhea == NULL)
				font->hmtx = NULL;
			else {
				if (font->hhea->numberOfMetrics == 0)
					font->hmtx = NULL;
				else {
					table_hmtx *hmtx = (table_hmtx *)malloc(sizeof(table_hmtx) * 1);

					uint32_t count_a = font->hhea->numberOfMetrics;
					uint32_t count_k = font->maxp->numGlyphs - font->hhea->numberOfMetrics;

					hmtx->metrics = (horizontal_metric *)malloc(sizeof(horizontal_metric) * count_a);
					hmtx->leftSideBearing = (int16_t *)malloc(sizeof(int16_t) * count_k);

					for (uint32_t ia = 0; ia < count_a; ia++) {
						hmtx->metrics[ia].advanceWidth = caryll_blt16u(data + ia * 4);
						hmtx->metrics[ia].lsb = caryll_blt16u(data + ia * 4 + 2);
					}

					for (uint32_t ik = 0; ik < count_k; ik++) {
						hmtx->leftSideBearing[ik] = caryll_blt16u(data + count_a * 4 + ik * 2);
					}

					font->hmtx = hmtx;
				}
			}
		}
	}
}
