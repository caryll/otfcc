#ifndef CARYLL_TABLES_NAME_H
#define CARYLL_TABLES_NAME_H

#include "../support/util.h"
#include "../caryll-sfnt.h"

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

table_name *caryll_read_name(caryll_packet packet);
void caryll_delete_name(table_name *table);
void caryll_name_to_json(table_name *table, json_value *root);

#endif
