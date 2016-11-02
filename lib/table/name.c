#include "name.h"

#include "support/util.h"
#include "support/unicodeconv/unicodeconv.h"

static void nameRecordDtor(otfcc_NameRecord *entry) {
	DELETE(sdsfree, entry->nameString);
}
static const caryll_VectorEntryTypeInfo(otfcc_NameRecord) otfcc_NameRecordTI = {
    .ctor = NULL, .dtor = nameRecordDtor, .copyctor = NULL,
};

void otfcc_deleteName(table_name *table) {
	caryll_vecReset(table);
	FREE(table);
}

table_name *otfcc_newName() {
	return caryll_vecNew(otfcc_NameRecord, otfcc_NameRecordTI);
}

static bool shouldDecodeAsUTF16(const otfcc_NameRecord *record) {
	return (record->platformID == 0)                               // Unicode, all
	       || (record->platformID == 2 && record->encodingID == 1) // ISO, 1
	       || (record->platformID == 3 &&                          // Microsoft, 0, 1, 10
	           (record->encodingID == 0 || record->encodingID == 1 || record->encodingID == 10));
}
static bool shouldDecodeAsBytes(const otfcc_NameRecord *record) {
	return record->platformID == 1 && record->encodingID == 0 && record->languageID == 0; // Mac Roman English - I hope
}

table_name *otfcc_readName(const otfcc_Packet packet, const otfcc_Options *options) {
	FOR_TABLE('name', table) {
		table_name *name = NULL;
		font_file_pointer data = table.data;

		uint32_t length = table.length;
		if (length < 6) goto TABLE_NAME_CORRUPTED;
		uint32_t count = read_16u(data + 2);
		uint32_t stringOffset = read_16u(data + 4);
		if (length < 6 + 12 * count) goto TABLE_NAME_CORRUPTED;

		name = otfcc_newName();

		for (uint16_t j = 0; j < count; j++) {
			otfcc_NameRecord record;
			record.platformID = read_16u(data + 6 + j * 12);
			record.encodingID = read_16u(data + 6 + j * 12 + 2);
			record.languageID = read_16u(data + 6 + j * 12 + 4);
			record.nameID = read_16u(data + 6 + j * 12 + 6);
			record.nameString = NULL;
			uint16_t length = read_16u(data + 6 + j * 12 + 8);
			uint16_t offset = read_16u(data + 6 + j * 12 + 10);

			if (shouldDecodeAsBytes(&record)) {
				// Mac Roman. Note that this is not very correct, but works for most fonts
				sds nameString = sdsnewlen(data + stringOffset + offset, length);
				record.nameString = nameString;
			} else if (shouldDecodeAsUTF16(&record)) {
				sds nameString = utf16be_to_utf8(data + stringOffset + offset, length);
				record.nameString = nameString;
			} else {
				size_t len = 0;
				uint8_t *buf = base64_encode(data + stringOffset + offset, length, &len);
				record.nameString = sdsnewlen(buf, len);
				FREE(buf);
			}
			caryll_vecPush(name, record);
		}
		return name;
	TABLE_NAME_CORRUPTED:
		logWarning("table 'name' corrupted.\n");
		if (name) { DELETE(otfcc_deleteName, name); }
	}
	return NULL;
}

void otfcc_dumpName(const table_name *name, json_value *root, const otfcc_Options *options) {
	if (!name) return;
	loggedStep("name") {
		json_value *_name = json_array_new(name->length);
		for (uint16_t j = 0; j < name->length; j++) {
			otfcc_NameRecord *r = &(name->data[j]);
			json_value *record = json_object_new(5);
			json_object_push(record, "platformID", json_integer_new(r->platformID));
			json_object_push(record, "encodingID", json_integer_new(r->encodingID));
			json_object_push(record, "languageID", json_integer_new(r->languageID));
			json_object_push(record, "nameID", json_integer_new(r->nameID));
			json_object_push(record, "nameString",
			                 json_string_new_length((uint32_t)sdslen(r->nameString), r->nameString));
			json_array_push(_name, record);
		}
		json_object_push(root, "name", _name);
	}
}
static int name_record_sort(const void *_a, const void *_b) {
	const otfcc_NameRecord *a = (const otfcc_NameRecord *)_a;
	const otfcc_NameRecord *b = (const otfcc_NameRecord *)_b;
	if (a->platformID != b->platformID) return a->platformID - b->platformID;
	if (a->encodingID != b->encodingID) return a->encodingID - b->encodingID;
	if (a->languageID != b->languageID) return a->languageID - b->languageID;
	return a->nameID - b->nameID;
}
table_name *otfcc_parseName(const json_value *root, const otfcc_Options *options) {
	table_name *name = caryll_vecNew(otfcc_NameRecord, otfcc_NameRecordTI);
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "name", json_array))) {
		loggedStep("name") {

			for (uint32_t j = 0; j < table->u.array.length; j++) {
				if (!(table->u.array.values[j] && table->u.array.values[j]->type == json_object)) continue;
				json_value *_record = table->u.array.values[j];
				if (!json_obj_get_type(_record, "platformID", json_integer)) continue;
				if (!json_obj_get_type(_record, "encodingID", json_integer)) continue;
				if (!json_obj_get_type(_record, "languageID", json_integer)) continue;
				if (!json_obj_get_type(_record, "nameID", json_integer)) continue;
				if (!json_obj_get_type(_record, "nameString", json_string)) continue;
				otfcc_NameRecord record;
				record.platformID = json_obj_getint(_record, "platformID");
				record.encodingID = json_obj_getint(_record, "encodingID");
				record.languageID = json_obj_getint(_record, "languageID");
				record.nameID = json_obj_getint(_record, "nameID");

				json_value *str = json_obj_get_type(_record, "nameString", json_string);
				record.nameString = sdsnewlen(str->u.string.ptr, str->u.string.length);
				caryll_vecPush(name, record);
			}

			qsort(name->data, name->length, sizeof(otfcc_NameRecord), name_record_sort);
		}
	}
	return name;
}
caryll_Buffer *otfcc_buildName(const table_name *name, const otfcc_Options *options) {
	caryll_Buffer *buf = bufnew();
	if (!name) return buf;
	bufwrite16b(buf, 0);
	bufwrite16b(buf, name->length);
	bufwrite16b(buf, 0); // fill later
	caryll_Buffer *strings = bufnew();
	for (uint16_t j = 0; j < name->length; j++) {
		otfcc_NameRecord *record = &(name->data[j]);
		bufwrite16b(buf, record->platformID);
		bufwrite16b(buf, record->encodingID);
		bufwrite16b(buf, record->languageID);
		bufwrite16b(buf, record->nameID);
		size_t cbefore = strings->cursor;
		if (shouldDecodeAsUTF16(record)) {
			size_t words;
			uint8_t *u16 = utf8toutf16be(record->nameString, &words);
			bufwrite_bytes(strings, words, u16);
			FREE(u16);
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
