#include "gasp.h"

#define GASP_DOGRAY 0x0002
#define GASP_GRIDFIT 0x0001
#define GASP_SYMMETRIC_GRIDFIT 0x0004
#define GASP_SYMMETRIC_SMOOTHING 0x0008

table_gasp *caryll_new_gasp() {
	table_gasp *gasp = malloc(sizeof(table_gasp));
	if (!gasp) return NULL;
	gasp->version = 1;
	gasp->numRanges = 0;
	gasp->records = NULL;
	return gasp;
}
void caryll_delete_gasp(table_gasp *table) {
	if (!table) return;
	if (table->records) free(table->records);
	free(table);
}
table_gasp *caryll_read_gasp(caryll_packet packet) {
	table_gasp *gasp = NULL;
	FOR_TABLE('gasp', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		if (length < 4) { goto FAIL; }
		gasp = malloc(sizeof(table_gasp));
		gasp->version = read_16u(data);
		gasp->numRanges = read_16u(data + 2);
		if (length < 4 + gasp->numRanges * 4) { goto FAIL; }

		gasp->records = malloc(gasp->numRanges * sizeof(gasp_record));
		if (!gasp->records) goto FAIL;

		for (uint32_t j = 0; j < gasp->numRanges; j++) {
			gasp->records[j].rangeMaxPPEM = read_16u(data + 4 + j * 4);
			uint16_t rangeGaspBehavior = read_16u(data + 4 + j * 4 + 2);
			gasp->records[j].dogray = !!(rangeGaspBehavior & GASP_DOGRAY);
			gasp->records[j].gridfit = !!(rangeGaspBehavior & GASP_GRIDFIT);
			gasp->records[j].symmetric_smoothing = !!(rangeGaspBehavior & GASP_SYMMETRIC_SMOOTHING);
			gasp->records[j].symmetric_gridfit = !!(rangeGaspBehavior & GASP_SYMMETRIC_SMOOTHING);
		}
		return gasp;

	FAIL:
		fprintf(stderr, "table 'head' corrupted.\n");
		caryll_delete_gasp(gasp);
		gasp = NULL;
	}
	return NULL;
}
void caryll_gasp_to_json(table_gasp *table, json_value *root, caryll_dump_options *dumpopts) {
	if (!table) return;
	json_value *t = json_array_new(table->numRanges);
	for (uint16_t j = 0; j < table->numRanges; j++) {
		json_value *rec = json_object_new(5);
		json_object_push(rec, "rangeMaxPPEM", json_integer_new(table->records[j].rangeMaxPPEM));
		json_object_push(rec, "dogray", json_boolean_new(table->records[j].dogray));
		json_object_push(rec, "gridfit", json_boolean_new(table->records[j].gridfit));
		json_object_push(rec, "symmetric_smoothing",
		                 json_boolean_new(table->records[j].symmetric_smoothing));
		json_object_push(rec, "symmetric_gridfit",
		                 json_boolean_new(table->records[j].symmetric_gridfit));
		json_array_push(t, rec);
	}
	json_object_push(root, "gasp", t);
}

table_gasp *caryll_gasp_from_json(json_value *root) {
	table_gasp *gasp = NULL;
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "gasp", json_array))) {
		gasp = caryll_new_gasp();
		if (!gasp) goto FAIL;
		gasp->numRanges = table->u.array.length;
		gasp->records = malloc(gasp->numRanges * sizeof(gasp_record));
		if (!gasp->records) goto FAIL;
		for (uint16_t j = 0; j < gasp->numRanges; j++) {
			json_value *r = table->u.array.values[j];
			if (!r || r->type != json_object) goto FAIL;
			gasp->records[j].rangeMaxPPEM = json_obj_getint_fallback(r, "rangeMaxPPEM", 0xFFFF);
			gasp->records[j].dogray = json_obj_getbool(r, "dogray");
			gasp->records[j].gridfit = json_obj_getbool(r, "gridfit");
			gasp->records[j].symmetric_smoothing = json_obj_getbool(r, "symmetric_smoothing");
			gasp->records[j].symmetric_gridfit = json_obj_getbool(r, "symmetric_gridfit");
		}
	}
	return gasp;
FAIL:
	if (gasp) caryll_delete_gasp(gasp);
	return NULL;
}

caryll_buffer *caryll_write_gasp(table_gasp *gasp) {
	caryll_buffer *buf = bufnew();
	if (!gasp || !gasp->records) return buf;
	bufwrite16b(buf, 1);
	bufwrite16b(buf, gasp->numRanges);
	for (uint16_t j = 0; j < gasp->numRanges; j++) {
		gasp_record *r = &(gasp->records[j]);
		bufwrite16b(buf, r->rangeMaxPPEM);
		bufwrite16b(buf, (r->dogray ? GASP_DOGRAY : 0) | (r->gridfit ? GASP_GRIDFIT : 0) |
		                     (r->symmetric_gridfit ? GASP_SYMMETRIC_GRIDFIT : 0) |
		                     (r->symmetric_smoothing ? GASP_SYMMETRIC_SMOOTHING : 0));
	}
	return buf;
}
