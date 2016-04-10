#ifndef CARYLL_TABLES_LTSH_H
#define CARYLL_TABLES_LTSH_H

#include "../support/util.h"
#include "../caryll-sfnt.h"

typedef struct {
	uint16_t version;
	uint16_t numGlyphs;
	uint8_t *yPels;
} table_LTSH;

table_LTSH *caryll_read_LTSH(caryll_packet packet);

#endif
