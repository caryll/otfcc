#include "name.h"
#include "../support/unicodeconv.h"

table_name *caryll_read_name(caryll_packet packet) {
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
		return name;
	TABLE_NAME_CORRUPTED:
		fprintf(stderr, "table 'name' corrupted.\n");
		if (name) free(name);
	}
	return NULL;
}

void caryll_delete_name(table_name *table) {
	for (uint16_t j = 0; j < table->count; j++) {
		if (table->records[j]->nameString) sdsfree(table->records[j]->nameString);
		free(table->records[j]);
	}
	free(table->records);
	free(table);
}

void caryll_name_to_json(table_name *table, json_value *root) {
	if (!table) return;
	json_value *name = json_array_new(table->count);
	for (uint16_t j = 0; j < table->count; j++) {
		name_record *r = table->records[j];
		json_value *record = json_object_new(5);
		json_object_push(record, "platformID", json_integer_new(r->platformID));
		json_object_push(record, "encodingID", json_integer_new(r->encodingID));
		json_object_push(record, "languageID", json_integer_new(r->languageID));
		json_object_push(record, "nameID", json_integer_new(r->nameID));
		json_object_push(record, "nameString", json_string_new_length(sdslen(r->nameString), r->nameString));
		json_array_push(name, record);
	}
	json_object_push(root, "name", name);
}
