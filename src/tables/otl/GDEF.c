#include "GDEF.h"

void caryll_delete_GDEF(table_GDEF *gdef) {
	if (gdef) {
		if (gdef->glyphClassDef) caryll_delete_classdef(gdef->glyphClassDef);
		free(gdef);
	}
}

table_GDEF *caryll_new_GDEF() {
	table_GDEF *gdef;
	NEW(gdef);
	gdef->glyphClassDef = NULL;
	return gdef;
}

table_GDEF *caryll_read_GDEF(caryll_packet packet) {
	table_GDEF *gdef = NULL;
	FOR_TABLE('GDEF', table) {
		font_file_pointer data = table.data;
		uint32_t tableLength = table.length;
		checkLength(12);
		gdef = caryll_new_GDEF();
		uint16_t classdefOffset = read_16u(data + 4);
		fprintf(stderr, "%d\n", classdefOffset);
		if (classdefOffset) {
			gdef->glyphClassDef = caryll_raad_classdef(data, tableLength, classdefOffset);
		}
		return gdef;

	FAIL:
		DELETE(caryll_delete_GDEF, gdef);
	}
	return gdef;
}

void caryll_GDEF_to_json(table_GDEF *gdef, json_value *root, caryll_dump_options *dumpopts) {
	if (!gdef) return;
	json_value *_gdef = json_object_new(4);
	json_object_push(_gdef, "glyphClassDef", caryll_classdef_to_json(gdef->glyphClassDef));
	json_object_push(root, "GDEF", _gdef);
}

table_GDEF *caryll_GDEF_from_json(json_value *root, caryll_dump_options *dumpopts) {
	table_GDEF *gdef = NULL;
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "GDEF", json_object))) {
		gdef = caryll_new_GDEF();
		gdef->glyphClassDef = caryll_classdef_from_json(json_obj_get(table, "glyphClassDef"));
	}
	return gdef;
}

caryll_buffer *caryll_write_GDEF(table_GDEF *gdef) {
	caryll_buffer *buf = bufnew();
	bufwrite32b(buf, 0x10000);
	size_t offset = 12;
	size_t cp = 0;
	if (gdef->glyphClassDef) {
		bufpingpong16b(buf, caryll_write_classdef(gdef->glyphClassDef), &offset, &cp);
	} else {
		bufwrite16b(buf, 0);
	}
	bufwrite16b(buf, 0);
	bufwrite16b(buf, 0);
	bufwrite16b(buf, 0);
	return buf;
}
