#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "../support/unicodeconv.h"

#include "../caryll-sfnt.h"
#include "../caryll-font.h"
#include "../caryll-io.h"

void caryll_read_name(caryll_font *font, caryll_packet packet) {
	FOR_TABLE('name', table) {
		table_name *name = NULL;
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		if (length < 6) goto TABLE_NAME_CORRUPTED;

		name = calloc(1, sizeof(table_name));
		if (!name) goto TABLE_NAME_CORRUPTED;
		name->format = caryll_blt16u(data);
		name->count = caryll_blt16u(data + 2);
		name->stringOffset = caryll_blt16u(data + 4);
		if (length < 6 + 12 * name->count) goto TABLE_NAME_CORRUPTED;

		name->records = calloc(name->count, sizeof(name_record *));
		for (uint16_t j = 0; j < name->count; j++) {
			name_record *record = calloc(1, sizeof(name_record));
			record->platformID = caryll_blt16u(data + 6 + j * 12);
			record->encodingID = caryll_blt16u(data + 6 + j * 12 + 2);
			record->languageID = caryll_blt16u(data + 6 + j * 12 + 4);
			record->nameID = caryll_blt16u(data + 6 + j * 12 + 6);
			record->nameString = NULL;
			uint16_t length = caryll_blt16u(data + 6 + j * 12 + 8);
			uint16_t offset = caryll_blt16u(data + 6 + j * 12 + 10);

			if (record->platformID == 1 && record->encodingID == 0) {
				// Mac Roman
				sds nameString = sdsnewlen(data + (name->stringOffset) + offset, length);
				record->nameString = nameString;
			} else if (record->platformID == 0 || (record->platformID == 3 && record->encodingID == 1)) {
				sds nameString = utf16be_to_utf8(data + (name->stringOffset) + offset, length);
				record->nameString = nameString;
			} else {
				record->nameString = sdsnew("(Unsupported encoding)");
			}
			name->records[j] = record;
		}
		font->name = name;

		goto FINIS;
	TABLE_NAME_CORRUPTED:
		fprintf(stderr, "table 'name' corrupted.\n");
		if (name) free(name);
		font->name = NULL;
	FINIS:;
	}
}

void caryll_delete_table_name(caryll_font *font) {
	for (uint16_t j = 0; j < font->name->count; j++) {
		if (font->name->records[j]->nameString) sdsfree(font->name->records[j]->nameString);
		free(font->name->records[j]);
	}
	free(font->name->records);
	free(font->name);
}

void caryll_name_to_json(caryll_font *font, JSON_Object *root){
	if(!font->name) return;
	JSON_Value *_name = json_value_init_array();
	JSON_Array *name = json_value_get_array(_name);
	for(uint16_t j = 0; j < font->name->count; j++){
		name_record *r = font->name->records[j];
		JSON_Value *_record = json_value_init_object();
		JSON_Object *record = json_value_get_object(_record);
		json_object_set_number(record, "platformID", r->platformID);
		json_object_set_number(record, "encodingID", r->encodingID);
		json_object_set_number(record, "languageID", r->languageID);
		json_object_set_number(record, "nameID", r->nameID);
		json_object_set_string(record, "nameString", r->nameString);
		json_array_append_value(name, _record);
	}
	json_object_set_value(root, "name", _name);
}
