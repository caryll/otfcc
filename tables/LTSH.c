#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

void caryll_read_LTSH(caryll_font *font, caryll_packet packet) {
	for (uint32_t i = 0; i < packet.numTables; i++) {
		if (packet.pieces[i].tag == 'LTSH') {
			uint8_t *data = packet.pieces[i].data;

			table_LTSH *LTSH = (table_LTSH *)malloc(sizeof(table_LTSH) * 1);
			LTSH->version = caryll_blt16u(data);
			LTSH->numGlyphs = caryll_blt16u(data + 2);
			LTSH->yPels = (uint8_t *)malloc(sizeof(uint8_t) * LTSH->numGlyphs);
			memcpy(LTSH->yPels, data + 4, LTSH->numGlyphs);

			font->LTSH = LTSH;
		}
	}
}
