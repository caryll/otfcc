#ifndef CARYLL_INCLUDE_TABLE_FPGM_PREP_H
#define CARYLL_INCLUDE_TABLE_FPGM_PREP_H

#include "table-common.h"

typedef struct {
	sds tag;
	uint32_t length;
	uint8_t *bytes;
} table_fpgm_prep;
void otfcc_deleteFpgm_prep(table_fpgm_prep *table);
table_fpgm_prep *otfcc_readFpgmPrep(const otfcc_Packet packet, const otfcc_Options *options, uint32_t tag);
void table_dumpTableFpgmPrep(const table_fpgm_prep *table, json_value *root, const otfcc_Options *options,
                             const char *tag);
table_fpgm_prep *otfcc_parseFpgmPrep(const json_value *root, const otfcc_Options *options, const char *tag);
caryll_Buffer *otfcc_buildFpgmPrep(const table_fpgm_prep *table, const otfcc_Options *options);

#endif
