#include "name.h"
#include "support/unicodeconv.h"

static bool shouldDecodeAsUTF16(const name_record *record) {
	return (record->platformID == 0)                               // Unicode, all
	       || (record->platformID == 2 && record->encodingID == 1) // ISO, 1
	       || (record->platformID == 3 &&                          // Microsoft, 0, 1, 10
	           (record->encodingID == 0 || record->encodingID == 1 || record->encodingID == 10));
}
static bool shouldDecodeAsBytes(const name_record *record) {
	return record->platformID == 1 && record->encodingID == 0 && record->languageID == 0; // Mac Roman English - I hope
}

table_name *table_read_name(const caryll_Packet packet) {
	FOR_TABLE('name', table) {
		table_name *name = NULL;
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		if (length < 6) goto TABLE_NAME_CORRUPTED;

		name = calloc(1, sizeof(table_name));
		if (!name) goto TABLE_NAME_CORRUPTED;
		name->format = read_16u(data);
		name->count = read_16u(data + 2);
		name->stringOffset = read_16u(data + 4);
		if (length < 6 + 12 * name->count) goto TABLE_NAME_CORRUPTED;

		name->records = calloc(name->count, sizeof(name_record *));
		for (uint16_t j = 0; j < name->count; j++) {
			name_record *record = calloc(1, sizeof(name_record));
			record->platformID = read_16u(data + 6 + j * 12);
			record->encodingID = read_16u(data + 6 + j * 12 + 2);
			record->languageID = read_16u(data + 6 + j * 12 + 4);
			record->nameID = read_16u(data + 6 + j * 12 + 6);
			record->nameString = NULL;
			uint16_t length = read_16u(data + 6 + j * 12 + 8);
			uint16_t offset = read_16u(data + 6 + j * 12 + 10);

			if (shouldDecodeAsBytes(record)) {
				// Mac Roman. Note that this is not very correct, but works for most fonts
				sds nameString = sdsnewlen(data + (name->stringOffset) + offset, length);
				record->nameString = nameString;
			} else if (shouldDecodeAsUTF16(record)) {
				sds nameString = utf16be_to_utf8(data + (name->stringOffset) + offset, length);
				record->nameString = nameString;
			} else {
				size_t len = 0;
				uint8_t *buf = base64_encode(data + (name->stringOffset) + offset, length, &len);
				record->nameString = sdsnewlen(buf, len);
				FREE(buf);
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

void table_delete_name(table_name *table) {
	for (uint16_t j = 0; j < table->count; j++) {
		if (table->records[j]->nameString) sdsfree(table->records[j]->nameString);
		free(table->records[j]);
	}
	free(table->records);
	free(table);
}

void table_dump_name(const table_name *table, json_value *root, const caryll_Options *options) {
	if (!table) return;
	if (options->verbose) fprintf(stderr, "Dumping name.\n");

	json_value *name = json_array_new(table->count);
	for (uint16_t j = 0; j < table->count; j++) {
		name_record *r = table->records[j];
		json_value *record = json_object_new(5);
		json_object_push(record, "platformID", json_integer_new(r->platformID));
		json_object_push(record, "encodingID", json_integer_new(r->encodingID));
		json_object_push(record, "languageID", json_integer_new(r->languageID));
		json_object_push(record, "nameID", json_integer_new(r->nameID));
		json_object_push(record, "nameString", json_string_new_length((uint32_t)sdslen(r->nameString), r->nameString));
		json_array_push(name, record);
	}
	json_object_push(root, "name", name);
}
static int name_record_sort(const void *_a, const void *_b) {
	const name_record **a = (const name_record **)_a;
	const name_record **b = (const name_record **)_b;
	if ((*a)->platformID != (*b)->platformID) return (*a)->platformID - (*b)->platformID;
	if ((*a)->encodingID != (*b)->encodingID) return (*a)->encodingID - (*b)->encodingID;
	if ((*a)->languageID != (*b)->languageID) return (*a)->languageID - (*b)->languageID;
	return (*a)->nameID - (*b)->nameID;
}
table_name *table_parse_name(const json_value *root, const caryll_Options *options) {
	table_name *name = calloc(1, sizeof(table_name));
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "name", json_array))) {
		if (options->verbose) fprintf(stderr, "Parsing name.\n");
		int validCount = 0;
		for (uint32_t j = 0; j < table->u.array.length; j++) {
			if (table->u.array.values[j] && table->u.array.values[j]->type == json_object) {
				json_value *record = table->u.array.values[j];
				if (!json_obj_get_type(record, "platformID", json_integer))
					fprintf(stderr, "Missing or invalid platformID for name entry %d\n", j);
				if (!json_obj_get_type(record, "encodingID", json_integer))
					fprintf(stderr, "Missing or invalid encodingID for name entry %d\n", j);
				if (!json_obj_get_type(record, "languageID", json_integer))
					fprintf(stderr, "Missing or invalid languageID for name entry %d\n", j);
				if (!json_obj_get_type(record, "nameID", json_integer))
					fprintf(stderr, "Missing or invalid nameID for name entry %d\n", j);
				if (!json_obj_get_type(record, "nameString", json_string))
					fprintf(stderr, "Missing or invalid nameString for name entry %d\n", j);
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
		NEW_N(name->records, validCount);
		int jj = 0;
		for (uint32_t j = 0; j < table->u.array.length; j++) {
			if (table->u.array.values[j] && table->u.array.values[j]->type == json_object) {
				json_value *record = table->u.array.values[j];
				if (json_obj_get_type(record, "platformID", json_integer) &&
				    json_obj_get_type(record, "encodingID", json_integer) &&
				    json_obj_get_type(record, "languageID", json_integer) &&
				    json_obj_get_type(record, "nameID", json_integer) &&
				    json_obj_get_type(record, "nameString", json_string)) {
					NEW(name->records[jj]);
					name->records[jj]->platformID = json_obj_getint(record, "platformID");
					name->records[jj]->encodingID = json_obj_getint(record, "encodingID");
					name->records[jj]->languageID = json_obj_getint(record, "languageID");
					name->records[jj]->nameID = json_obj_getint(record, "nameID");

					json_value *str = json_obj_get_type(record, "nameString", json_string);
					name->records[jj]->nameString = sdsnewlen(str->u.string.ptr, str->u.string.length);
					jj += 1;
				}
			}
		}
		qsort(name->records, validCount, sizeof(name_record *), name_record_sort);
	}
	return name;
}
caryll_Buffer *table_build_name(const table_name *name, const caryll_Options *options) {
	caryll_Buffer *buf = bufnew();
	if (!name) return buf;
	bufwrite16b(buf, 0);
	bufwrite16b(buf, name->count);
	bufwrite16b(buf, 0); // fill later
	caryll_Buffer *strings = bufnew();
	for (uint16_t j = 0; j < name->count; j++) {
		name_record *record = name->records[j];
		bufwrite16b(buf, record->platformID);
		bufwrite16b(buf, record->encodingID);
		bufwrite16b(buf, record->languageID);
		bufwrite16b(buf, record->nameID);
		size_t cbefore = strings->cursor;
		if (shouldDecodeAsUTF16(record)) {
			size_t words;
			uint8_t *u16 = utf8toutf16be(record->nameString, &words);
			bufwrite_bytes(strings, words, u16);
			free(u16);
		} else if (shouldDecodeAsBytes(record)) {
			bufwrite_bytes(strings, sdslen(record->nameString), (uint8_t *)record->nameString);
		} else {
			size_t length;
			uint8_t *decoded = base64_decode((uint8_t *)record->nameString, sdslen(record->nameString), &length);
			bufwrite_bytes(strings, length, decoded);
			FREE(decoded);
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
