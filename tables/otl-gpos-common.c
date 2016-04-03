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
