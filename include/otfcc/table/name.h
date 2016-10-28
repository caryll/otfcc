#ifndef CARYLL_INCLUDE_TABLE_NAME_H
#define CARYLL_INCLUDE_TABLE_NAME_H

#include "table-common.h"

typedef struct {
	uint16_t platformID;
	uint16_t encodingID;
	uint16_t languageID;
	uint16_t nameID;
	sds nameString;
} name_record;

typedef struct {
	uint16_t format;
	uint16_t count;
	uint16_t stringOffset;
	name_record **records;
} table_name;

void otfcc_deleteTablename(table_name *table);

table_name *otfcc_readTablename(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpTablename(const table_name *table, json_value *root, const otfcc_Options *options);
table_name *otfcc_parseTablename(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildTablename(const table_name *name, const otfcc_Options *options);
#endif
