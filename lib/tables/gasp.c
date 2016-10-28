#include "support/util.h"
#include "otfcc/table/gasp.h"

#define GASP_DOGRAY 0x0002
#define GASP_GRIDFIT 0x0001
#define GASP_SYMMETRIC_GRIDFIT 0x0004
#define GASP_SYMMETRIC_SMOOTHING 0x0008

table_gasp *otfcc_newGasp() {
	table_gasp *gasp;
	NEW(gasp);
	gasp->version = 1;
	gasp->numRanges = 0;
	gasp->records = NULL;
	return gasp;
}
void otfcc_deleteGasp(table_gasp *table) {
	if (!table) return;
	if (table->records) FREE(table->records);
	FREE(table);
}
table_gasp *otfcc_readGasp(const otfcc_Packet packet, const otfcc_Options *options) {
	table_gasp *gasp = NULL;
	FOR_TABLE('gasp', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		if (length < 4) { goto FAIL; }
		NEW(gasp);
		gasp->version = read_16u(data);
		gasp->numRanges = read_16u(data + 2);
		if (length < 4 + gasp->numRanges * 4) { goto FAIL; }

		NEW(gasp->records, gasp->numRanges);
		if (!gasp->records) goto FAIL;

		for (uint32_t j = 0; j < gasp->numRanges; j++) {
			gasp->records[j].rangeMaxPPEM = read_16u(data + 4 + j * 4);
			uint16_t rangeGaspBehavior = read_16u(data + 4 + j * 4 + 2);
			gasp->records[j].dogray = !!(rangeGaspBehavior & GASP_DOGRAY);
			gasp->records[j].gridfit = !!(rangeGaspBehavior & GASP_GRIDFIT);
			gasp->records[j].symmetric_smoothing = !!(rangeGaspBehavior & GASP_SYMMETRIC_SMOOTHING);
			gasp->records[j].symmetric_gridfit = !!(rangeGaspBehavior & GASP_SYMMETRIC_GRIDFIT);
		}
		return gasp;

	FAIL:
		logWarning("table 'gasp' corrupted.\n");
		otfcc_deleteGasp(gasp);
		gasp = NULL;
	}
	return NULL;
}
void otfcc_dumpGasp(const table_gasp *table, json_value *root, const otfcc_Options *options) {
	if (!table) return;
	loggedStep("gasp") {
		json_value *t = json_array_new(table->numRanges);
		for (uint16_t j = 0; j < table->numRanges; j++) {
			json_value *rec = json_object_new(5);
			json_object_push(rec, "rangeMaxPPEM", json_integer_new(table->records[j].rangeMaxPPEM));
			json_object_push(rec, "dogray", json_boolean_new(table->records[j].dogray));
			json_object_push(rec, "gridfit", json_boolean_new(table->records[j].gridfit));
			json_object_push(rec, "symmetric_smoothing", json_boolean_new(table->records[j].symmetric_smoothing));
			json_object_push(rec, "symmetric_gridfit", json_boolean_new(table->records[j].symmetric_gridfit));
			json_array_push(t, rec);
		}
		json_object_push(root, "gasp", t);
	}
}

table_gasp *otfcc_parseGasp(const json_value *root, const otfcc_Options *options) {
	table_gasp *gasp = NULL;
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "gasp", json_array))) {
		gasp = otfcc_newGasp();
		if (!gasp) goto FAIL;
		loggedStep("gasp") {
			gasp->numRanges = table->u.array.length;
			NEW(gasp->records, gasp->numRanges);
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
	}
	return gasp;
FAIL:
	if (gasp) otfcc_deleteGasp(gasp);
	return NULL;
}

caryll_Buffer *otfcc_buildGasp(const table_gasp *gasp, const otfcc_Options *options) {
	caryll_Buffer *buf = bufnew();
	if (!gasp || !gasp->records) return buf;
	bufwrite16b(buf, 1);
	bufwrite16b(buf, gasp->numRanges);
	for (uint16_t j = 0; j < gasp->numRanges; j++) {
		gasp_Record *r = &(gasp->records[j]);
		bufwrite16b(buf, r->rangeMaxPPEM);
		bufwrite16b(buf, (r->dogray ? GASP_DOGRAY : 0) | (r->gridfit ? GASP_GRIDFIT : 0) |
		                     (r->symmetric_gridfit ? GASP_SYMMETRIC_GRIDFIT : 0) |
		                     (r->symmetric_smoothing ? GASP_SYMMETRIC_SMOOTHING : 0));
	}
	return buf;
}
