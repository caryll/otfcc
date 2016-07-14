#include "libcff.h"

// INDEX util functions
CFF_INDEX *cff_index_init(void) {
	CFF_INDEX *out = calloc(1, sizeof(CFF_INDEX));
	return out;
}

void cff_index_fini(CFF_INDEX *out) {
	if (out != NULL) {
		if (out->offset != NULL) free(out->offset);
		if (out->data != NULL) free(out->data);
		free(out);
	}
}

// DICT util functions
static void printf_cff_val(CFF_Value val) {
	if (val.t == CFF_INTEGER) fprintf(stderr, "%d", val.i);
	if (val.t == CFF_DOUBLE) fprintf(stderr, "%f", val.d);
}
static void callback_print_dict(uint32_t op, uint8_t top, CFF_Value *stack, void *context) {
	for (uint32_t i = 0; i < top; i++) printf_cff_val(stack[i]), fprintf(stderr, " ");
	fprintf(stderr, "%s\n", op_cff_name(op));
}
void print_dict(uint8_t *data, uint32_t len) {
	return cff_dict_callback(data, len, NULL, callback_print_dict);
}

void cff_delete_dict(CFF_Dict *dict) {
	if (!dict) return;
	for (uint32_t j = 0; j < dict->count; j++) { free(dict->ents[j].vals); }
	free(dict->ents);
	free(dict);
}
