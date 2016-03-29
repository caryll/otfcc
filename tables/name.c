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
			} else if ((record->platformID == 0 && record->encodingID == 1) ||
			           (record->platformID == 3 && record->encodingID == 1)) {
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
		if (name) { free(name), name = NULL; }
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

void caryll_name_to_json(table_name *table, json_value *root, caryll_dump_options dumpopts) {
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
table_name *caryll_name_from_json(json_value *root, caryll_dump_options dumpopts) {
	table_name *name = calloc(1, sizeof(table_name));
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "name", json_array))) {
		uint16_t validCount = 0;
		for (uint32_t j = 0; j < table->u.array.length; j++) {
			if (table->u.array.values[j] && table->u.array.values[j]->type == json_object) {
				json_value *record = table->u.array.values[j];
				if (json_obj_get_type(record, "platformID", json_integer) &&
				    json_obj_get_type(record, "encodingID", json_integer) &&
				    json_obj_get_type(record, "languageID", json_integer) &&
				    json_obj_get_type(record, "nameID", json_integer) &&
				    json_obj_get_type(record, "nameString", json_string)) {
					validCount += 1;
				}
			}
		}
		name->count = validCount;
		name->records = calloc(validCount, sizeof(name_record *));
		validCount = 0;
		for (uint32_t j = 0; j < table->u.array.length; j++) {
			if (table->u.array.values[j] && table->u.array.values[j]->type == json_object) {
				json_value *record = table->u.array.values[j];
				if (json_obj_get_type(record, "platformID", json_integer) &&
				    json_obj_get_type(record, "encodingID", json_integer) &&
				    json_obj_get_type(record, "languageID", json_integer) &&
				    json_obj_get_type(record, "nameID", json_integer) &&
				    json_obj_get_type(record, "nameString", json_string)) {

					name->records[validCount] = malloc(sizeof(name_record));
					name->records[validCount]->platformID = json_obj_getint(record, "platformID");
					name->records[validCount]->encodingID = json_obj_getint(record, "encodingID");
					name->records[validCount]->languageID = json_obj_getint(record, "languageID");
					name->records[validCount]->nameID = json_obj_getint(record, "nameID");

					json_value *str = json_obj_get_type(record, "nameString", json_string);
					name->records[validCount]->nameString = sdsnewlen(str->u.string.ptr, str->u.string.length);
					validCount += 1;
				}
			}
		}
	}
	return name;
}
caryll_buffer *caryll_write_name(table_name *name) {
	caryll_buffer *buf = bufnew();
	if (!name) return buf;
	bufwrite16b(buf, 0);
	bufwrite16b(buf, name->count);
	bufwrite16b(buf, 0); // fill later
	caryll_buffer *strings = bufnew();
	for (uint16_t j = 0; j < name->count; j++) {
		name_record *record = name->records[j];
		bufwrite16b(buf, record->platformID);
		bufwrite16b(buf, record->encodingID);
		bufwrite16b(buf, record->languageID);
		bufwrite16b(buf, record->nameID);
		size_t cbefore = strings->cursor;
		if (record->platformID == 3 && record->encodingID == 1) {
			size_t words;
			uint8_t *u16 = utf8toutf16be(record->nameString, &words);
			bufwrite_bytes(strings, words, u16);
			free(u16);
		} else {
			bufwrite_bytes(strings, sdslen(record->nameString), (uint8_t *)record->nameString);
		}
		size_t cafter = strings->cursor;
		bufwrite16b(buf, cafter - cbefore);
		bufwrite16b(buf, cbefore);
	}
	size_t stringsOffset = buf->cursor;
	bufwrite_buf(buf, strings);
	bufseek(buf, 4);
	bufwrite16b(buf, stringsOffset);
	buffree(strings);
	return buf;
}
