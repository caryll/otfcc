#ifndef CARYLL_INCLUDE_TABLE_LTSH_H
#define CARYLL_INCLUDE_TABLE_LTSH_H

#include "table-common.h"

typedef struct {
	uint16_t version;
	glyphid_t numGlyphs;
	uint8_t *yPels;
} table_LTSH;

table_LTSH *table_read_LTSH(const caryll_Packet packet, const otfcc_Options *options);
void table_delete_LTSH(table_LTSH *ltsh);
caryll_Buffer *table_build_LTSH(const table_LTSH *ltsh, const otfcc_Options *options);

#endif
