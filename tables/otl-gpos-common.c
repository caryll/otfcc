#include "otl-gpos-common.h"

void otl_delete_mark_array(otl_mark_array *array) {
	if (array) {
		if (array->records) free(array->records);
		free(array);
	};
}

otl_anchor otl_read_anchor(font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	otl_anchor anchor = {.x = 0, .y = 0};
	checkLength(offset + 6);
	anchor.present = true;
	anchor.x = read_16s(data + offset + 2);
	anchor.y = read_16s(data + offset + 4);
	return anchor;
FAIL:
	anchor.present = false;
	anchor.x = 0;
	anchor.y = 0;
	return anchor;
}

otl_mark_array *otl_read_mark_array(font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	otl_mark_array *array = NULL;
	NEW(array);
	checkLength(offset + 2);
	array->markCount = read_16u(data + offset);
	checkLength(offset + 2 + 4 * array->markCount);
	NEW_N(array->records, array->markCount);
	for (uint16_t j = 0; j < array->markCount; j++) {
		array->records[j].markClass = read_16u(data + offset + 2 + j * 4);
		uint16_t delta = read_16u(data + offset + 2 + j * 4 + 2);
		if (delta) {
			array->records[j].anchor = otl_read_anchor(data, tableLength, offset + delta);
		} else {
			array->records[j].anchor.present = false;
			array->records[j].anchor.x = 0;
			array->records[j].anchor.y = 0;
		}
	}
	return array;
FAIL:
	DELETE(otl_delete_mark_array, array);
	return NULL;
}

json_value *gpos_value_to_json(otl_position_value value) {
	json_value *v = json_object_new(4);
	if (value.dx) json_object_push(v, "dx", json_integer_new(value.dx));
	if (value.dy) json_object_push(v, "dy", json_integer_new(value.dy));
	if (value.dWidth) json_object_push(v, "dWidth", json_integer_new(value.dWidth));
	if (value.dHeight) json_object_push(v, "dHeight", json_integer_new(value.dHeight));
	return preserialize(v);
}
otl_position_value gpos_value_from_json(json_value *pos) {
	otl_position_value v = {0, 0, 0, 0};
	v.dx = json_obj_getnum(pos, "dx");
	v.dy = json_obj_getnum(pos, "dy");
	v.dWidth = json_obj_getnum(pos, "dWidth");
	v.dHeight = json_obj_getnum(pos, "dHeight");
	return v;
}
