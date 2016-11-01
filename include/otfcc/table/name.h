#ifndef CARYLL_INCLUDE_TABLE_NAME_H
#define CARYLL_INCLUDE_TABLE_NAME_H

#include "table-common.h"

typedef struct {
	uint16_t platformID;
	uint16_t encodingID;
	uint16_t languageID;
	uint16_t nameID;
	sds nameString;
} otfcc_NameRecord;

typedef struct {
	uint16_t format;
	uint16_t count;
	uint16_t stringOffset;
	OWNING otfcc_NameRecord *records;
} table_name;

#endif
