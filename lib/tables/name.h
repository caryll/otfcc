#ifndef CARYLL_TABLES_NAME_H
#define CARYLL_TABLES_NAME_H

#include "support/util.h"
#include "font/caryll-sfnt.h"
#include "support/base64.h"

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

table_name *table_read_name(caryll_Packet packet);
void table_delete_name(table_name *table);
void table_dump_name(table_name *table, json_value *root, const caryll_Options *options);
table_name *table_parse_name(json_value *root, const caryll_Options *options);
caryll_buffer *table_build_name(table_name *name, const caryll_Options *options);
#endif
