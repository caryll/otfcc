#ifndef CARYLL_TABLES_LTSH_H
#define CARYLL_TABLES_LTSH_H

#include <stdint.h>
#include "../caryll-font.h"

typedef struct {
	uint16_t version;
	uint16_t numGlyphs;
	uint8_t *yPels;
} table_LTSH;

void caryll_read_LTSH(caryll_font *font, caryll_packet packet);

#endif
