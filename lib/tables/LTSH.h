#ifndef CARYLL_TABLES_LTSH_H
#define CARYLL_TABLES_LTSH_H

#include <support/util.h>
#include <font/caryll-sfnt.h>

typedef struct {
	uint16_t version;
	glyphid_t numGlyphs;
	uint8_t *yPels;
} table_LTSH;

table_LTSH *table_read_LTSH(caryll_Packet packet);
void table_delete_LTSH(table_LTSH *ltsh);
caryll_buffer *table_build_LTSH(table_LTSH *ltsh, const caryll_Options *options);

#endif
