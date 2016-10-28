#ifndef CARYLL_INCLUDE_TABLE_VORG_H
#define CARYLL_INCLUDE_TABLE_VORG_H

#include "table-common.h"

typedef struct {
	glyphid_t gid;
	int16_t verticalOrigin;
} VORG_entry;

typedef struct {
	glyphid_t numVertOriginYMetrics;
	pos_t defaultVerticalOrigin;
	VORG_entry *entries;
} table_VORG;

void otfcc_deleteVORG(MOVE table_VORG *vorg);
table_VORG *otfcc_readVORG(const otfcc_Packet packet, const otfcc_Options *options);
caryll_Buffer *otfcc_buildVORG(const table_VORG *table, const otfcc_Options *options);
#endif
