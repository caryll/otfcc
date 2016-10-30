#ifndef CARYLL_TABLE_VORG_H
#define CARYLL_TABLE_VORG_H

#include "otfcc/table/VORG.h"
void otfcc_deleteVORG(MOVE table_VORG *vorg);
table_VORG *otfcc_readVORG(const otfcc_Packet packet, const otfcc_Options *options);
caryll_Buffer *otfcc_buildVORG(const table_VORG *table, const otfcc_Options *options);

#endif
