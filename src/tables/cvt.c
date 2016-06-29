#include "cvt.h"

table_cvt *caryll_read_cvt(caryll_packet packet, uint32_t tag) {
	table_cvt *t = NULL;
	FOR_TABLE(tag, table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		NEW(t);
		t->length = length >> 1;
		NEW_N(t->words, (t->length + 1));
		for (uint16_t j = 0; j < t->length; j++) { t->words[j] = read_16u(data + 2 * j); }
		return t;
	}
	return NULL;
}
void caryll_delete_cvt(table_cvt *table) {
	if (!table) return;
	if (table->words) free(table->words);
	free(table);
}
void caryll_cvt_to_json(table_cvt *table, json_value *root, caryll_dump_options *dumpopts,
                        const char *tag) {
	if (!table) return;
	json_value *arr = json_array_new(table->length);
	for (uint16_t j = 0; j < table->length; j++) {
		json_array_push(arr, json_integer_new(table->words[j]));
	}
	json_object_push(root, tag, arr);
}
table_cvt *caryll_cvt_from_json(json_value *root, const char *tag) {
	table_cvt *t = NULL;
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, tag, json_array))) {
		// Meaningful CVT dump
		NEW(t);
		t->length = table->u.array.length;
		NEW_N(t->words, (t->length + 1));
		for (uint16_t j = 0; j < t->length; j++) {
			json_value *record = table->u.array.values[j];
			if (record->type == json_integer) {
				t->words[j] = record->u.integer;
			} else if (record->type == json_double) {
				t->words[j] = (uint16_t)record->u.dbl;
			} else {
				t->words[j] = 0;
			}
		}
	} else if ((table = json_obj_get_type(root, tag, json_string))) {
		// Bytes CVT dump
		NEW(t);
		size_t len;
		uint8_t *raw = base64_decode((uint8_t *)table->u.string.ptr, table->u.string.length, &len);
		t->length = len >> 1;
		NEW_N(t->words, (t->length + 1));
		for (uint16_t j = 0; j < t->length; j++) { t->words[j] = read_16u(raw + 2 * j); }
		FREE(raw);
	}
	return t;
}

caryll_buffer *caryll_write_cvt(table_cvt *table) {
	caryll_buffer *buf = bufnew();
	if (!table) return buf;
	for (uint16_t j = 0; j < table->length; j++) { bufwrite16b(buf, table->words[j]); }
	return buf;
}
