#ifndef CARYLL_INCLUDE_TABLE_LTSH_H
#define CARYLL_INCLUDE_TABLE_LTSH_H

#include "table-common.h"

typedef struct {
	uint16_t version;
	glyphid_t numGlyphs;
	uint8_t *yPels;
} table_LTSH;

table_LTSH *otfcc_readTableLTSH(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_deleteTableLTSH(table_LTSH *ltsh);
caryll_Buffer *otfcc_buildTableLTSH(const table_LTSH *ltsh, const otfcc_Options *options);

#endif
