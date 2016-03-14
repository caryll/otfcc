#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

void caryll_read_LTSH(caryll_font *font, caryll_packet packet) {
	FOR_TABLE('LTSH', table) {
		font_file_pointer data = table.data;

		table_LTSH *LTSH = (table_LTSH *)malloc(sizeof(table_LTSH) * 1);
		LTSH->version = caryll_blt16u(data);
		LTSH->numGlyphs = caryll_blt16u(data + 2);
		LTSH->yPels = (uint8_t *)malloc(sizeof(uint8_t) * LTSH->numGlyphs);
		memcpy(LTSH->yPels, data + 4, LTSH->numGlyphs);

		font->LTSH = LTSH;
	}
}
