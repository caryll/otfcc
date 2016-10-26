#include "support/util.h"
#include "support/ttinstr.h"
#include "otfcc/table/fpgm-prep.h"

table_fpgm_prep *table_read_fpgm_prep(const caryll_Packet packet, uint32_t tag) {
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
		table_delete_fpgm_prep(t);
		t = NULL;
	}
	return NULL;
}

void table_delete_fpgm_prep(table_fpgm_prep *table) {
	if (!table) return;
	if (table->tag) sdsfree(table->tag);
	if (table->bytes) free(table->bytes);
	free(table);
}

void table_fpgm_dump_prep(const table_fpgm_prep *table, json_value *root, const otfcc_Options *options,
                          const char *tag) {
	if (!table) return;
	if (options->verbose) fprintf(stderr, "Dumping %s.\n", tag);

	json_object_push(root, tag, dump_ttinstr(table->bytes, table->length, options));
}

void makeFpgmPrepInstr(void *_t, uint8_t *instrs, uint32_t length) {
	table_fpgm_prep *t = (table_fpgm_prep *)_t;
	t->length = length;
	t->bytes = instrs;
}
void wrongFpgmPrepInstr(void *_t, char *reason, int pos) {
	table_fpgm_prep *t = (table_fpgm_prep *)_t;
	fprintf(stderr, "[OTFCC] TrueType instructions parse error : %s, at %d in /%s\n", reason, pos, t->tag);
}

table_fpgm_prep *table_fpgm_parse_prep(const json_value *root, const otfcc_Options *options, const char *tag) {
	table_fpgm_prep *t = NULL;
	json_value *table = NULL;
	if ((table = json_obj_get(root, tag))) {
		if (options->verbose) fprintf(stderr, "Parsing %s.\n", tag);
		NEW(t);
		t->tag = sdsnew(tag);
		parse_ttinstr(table, t, makeFpgmPrepInstr, wrongFpgmPrepInstr);
	}
	return t;
}

caryll_Buffer *table_build_fpgm_prep(const table_fpgm_prep *table, const otfcc_Options *options) {
	caryll_Buffer *buf = bufnew();
	if (!table) return buf;
	bufwrite_bytes(buf, table->length, table->bytes);
	return buf;
}
