#include "fpgm-prep.h"

table_fpgm_prep *caryll_read_fpgm_prep(caryll_packet packet, uint32_t tag) {
	table_fpgm_prep *t = NULL;
	FOR_TABLE(tag, table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		NEW(t);
		t->tag = NULL;
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
	if (table->tag) sdsfree(table->tag);
	if (table->bytes) free(table->bytes);
	free(table);
}

void caryll_fpgm_prep_to_json(table_fpgm_prep *table, json_value *root, caryll_options *options,
                              const char *tag) {
	if (!table) return;
	if (options->verbose) fprintf(stderr, "Dumping %s.\n", tag);

	json_object_push(root, tag, instr_to_json(table->bytes, table->length, options));
}

void makeFpgmPrepInstr(void *_t, uint8_t *instrs, uint32_t length) {
	table_fpgm_prep *t = (table_fpgm_prep *)_t;
	t->length = length;
	t->bytes = instrs;
}
void wrongFpgmPrepInstr(void *_t, char *reason, int pos) {
	table_fpgm_prep *t = (table_fpgm_prep *)_t;
	fprintf(stderr, "[OTFCC] TrueType instructions parse error : %s, at %d in /%s\n", reason, pos,
	        t->tag);
}

table_fpgm_prep *caryll_fpgm_prep_from_json(json_value *root, caryll_options *options,
                                            const char *tag) {
	table_fpgm_prep *t = NULL;
	json_value *table = NULL;
	if ((table = json_obj_get(root, tag))) {
		if (options->verbose) fprintf(stderr, "Parsing %s.\n", tag);
		NEW(t);
		t->tag = sdsnew(tag);
		instr_from_json(table, t, makeFpgmPrepInstr, wrongFpgmPrepInstr);
	}
	return t;
}

caryll_buffer *caryll_write_fpgm_prep(table_fpgm_prep *table, caryll_options *options) {
	caryll_buffer *buf = bufnew();
	if (!table) return buf;
	bufwrite_bytes(buf, table->length, table->bytes);
	return buf;
}
