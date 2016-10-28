#ifndef CARYLL_INCLUDE_TABLE_CVT_H
#define CARYLL_INCLUDE_TABLE_CVT_H

#include "table-common.h"

typedef struct {
	uint32_t length;
	uint16_t *words;
} table_cvt;
void otfcc_deleteTablecvt(table_cvt *table);

table_cvt *otfcc_readTablecvt(const otfcc_Packet packet, const otfcc_Options *options, uint32_t tag);
void otfcc_dumpTablecvt(const table_cvt *table, json_value *root, const otfcc_Options *options, const char *tag);
table_cvt *otfcc_parseTablecvt(const json_value *root, const otfcc_Options *options, const char *tag);
caryll_Buffer *otfcc_buildTablecvt(const table_cvt *table, const otfcc_Options *options);

#endif
