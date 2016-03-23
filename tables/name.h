#ifndef CARYLL_TABLES_NAME_H
#define CARYLL_TABLES_NAME_H

#include <stdint.h>
#include "../caryll-font.h"
#include "../extern/json-builder.h"

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

void caryll_read_name(caryll_font *font, caryll_packet packet);
void caryll_delete_table_name(caryll_font *font);
void caryll_name_to_json(caryll_font *font, json_value *root);

#endif
