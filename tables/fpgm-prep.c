#include "fpgm-prep.h"

table_fpgm_prep *caryll_read_fpgm_prep(caryll_packet packet, uint32_t tag) {
	table_fpgm_prep *t = NULL;
	FOR_TABLE(tag, table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		t = malloc(sizeof(table_fpgm_prep));
		if (!t) goto FAIL;
		t->length = length;
		t->bytes = malloc(sizeof(uint8_t) * length);
		if (!t->bytes) goto FAIL;
		memcpy(t->bytes, data, length);
		return t;
	FAIL:
		caryll_delete_fpgm_prep(t);
		t = NULL;
	}
	return NULL;
}
void caryll_delete_fpgm_prep(table_fpgm_prep *table) {
	if (!table) return;
	if (table->bytes) free(table->bytes);
	free(table);
}
void caryll_fpgm_prep_to_json(table_fpgm_prep *table, json_value *root, caryll_dump_options dumpopts, const char *tag) {
	if (!table) return;
	json_value *t = json_array_new(table->length);
	for (uint32_t j = 0; j < table->length; j++) {
		json_array_push(t, json_integer_new(table->bytes[j]));
	}
	json_object_push(root, tag, preserialize(t));
}
table_fpgm_prep *caryll_fpgm_prep_from_json(json_value *root, const char *tag) {
	table_fpgm_prep *t = NULL;
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, tag, json_array))) {
		t = malloc(sizeof(table_fpgm_prep));
		if (!t) goto FAIL;
		t->length = table->u.array.length;
		t->bytes = malloc(sizeof(uint8_t) * t->length);
		if (!t->bytes) goto FAIL;
		for (uint32_t j = 0; j < table->u.array.length; j++) {
			json_value *v = table->u.array.values[j];
			if (v->type == json_integer) {
				t->bytes[j] = v->u.integer;
			} else {
				t->bytes[j] = 0;
			}
		}
	}
	return t;
FAIL:
	if (t) caryll_delete_fpgm_prep(t);
	return NULL;
}

caryll_buffer *caryll_write_fpgm_prep(table_fpgm_prep *table) {
	caryll_buffer *buf = bufnew();
	if (!table) return buf;
	bufwrite_bytes(buf, table->length, table->bytes);
	return buf;
}
