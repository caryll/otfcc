#include "otl-gsub-single.h"
void caryll_delete_gsub_single(otl_lookup *lookup) {
	if (lookup) {
		if (lookup->subtables)
			for (uint16_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) {
					caryll_delete_coverage(lookup->subtables[j]->gsub_single.from);
					caryll_delete_coverage(lookup->subtables[j]->gsub_single.to);
				}
		FREE(lookup);
	}
}

void caryll_read_gsub_single(font_file_pointer data, uint32_t tableLength, otl_lookup *lookup) {
	for (uint16_t j = 0; j < lookup->subtableCount; j++) {
		otl_subtable *subtable;
		NEW(subtable);

		uint32_t subtableOffset = lookup->_offset + caryll_blt16u(data + lookup->_offset + 6 + j * 2);
		if (tableLength < subtableOffset + 6) goto FAIL;
		uint16_t subtableFormat = caryll_blt16u(data + subtableOffset);
		otl_coverage *from =
		    caryll_read_coverage(data, tableLength, subtableOffset + caryll_blt16u(data + subtableOffset + 2));
		subtable->gsub_single.from = from;
		if (!from || from->numGlyphs == 0) goto FAIL;

		if (subtableFormat == 1) {
			otl_coverage *to;
			NEW(to);
			to->numGlyphs = from->numGlyphs;
			NEW_N(to->glyphs, to->numGlyphs);

			uint16_t delta = caryll_blt16u(data + subtableOffset + 4);
			for (uint16_t j = 0; j < from->numGlyphs; j++) {
				to->glyphs[j].gid = from->glyphs[j].gid + delta;
				to->glyphs[j].name = NULL;
			}
			subtable->gsub_single.to = to;
		} else {
			uint16_t toglyphs = caryll_blt16u(data + subtableOffset + 4);
			if (tableLength < subtableOffset + 6 + toglyphs * 2 || toglyphs != from->numGlyphs) goto FAIL;
			otl_coverage *to;
			NEW(to);
			to->numGlyphs = toglyphs;
			NEW_N(to->glyphs, to->numGlyphs);
			
			for (uint16_t j = 0; j < to->numGlyphs; j++) {
				to->glyphs[j].gid = caryll_blt16u(data + subtableOffset + 6 + j * 2);
				to->glyphs[j].name = NULL;
			}
			subtable->gsub_single.to = to;
		}
		goto OK;
	FAIL:
		if (subtable->gsub_single.from) caryll_delete_coverage(subtable->gsub_single.from);
		if (subtable->gsub_single.to) caryll_delete_coverage(subtable->gsub_single.to);
		subtable = NULL;
	OK:
		lookup->subtables[j] = subtable;
	}
}
void caryll_gsub_single_to_json(otl_lookup *lookup, json_value *dump) {
	json_object_push(dump, "type", json_string_new("gsub_single"));
	json_value *subtables = json_array_new(lookup->subtableCount);
	for (uint16_t j = 0; j < lookup->subtableCount; j++)
		if (lookup->subtables[j]) {
			json_value *st = json_object_new(2);
			json_object_push(st, "from", caryll_coverage_to_json(lookup->subtables[j]->gsub_single.from));
			json_object_push(st, "to", caryll_coverage_to_json(lookup->subtables[j]->gsub_single.to));
			json_array_push(subtables, st);
		}
	json_object_push(dump, "subtables", subtables);
}

otl_lookup *caryll_gsub_single_from_json(json_value *_lookup) {
	otl_lookup *lookup = NULL;
	json_value *_subtables = json_obj_get_type(_lookup, "subtables", json_array);
	if (!_subtables) goto FAIL;

	NEW(lookup);
	lookup->type = otl_type_gsub_single;
	lookup->subtableCount = _subtables->u.array.length;
	NEW_N(lookup->subtables, lookup->subtableCount);

	uint16_t jj = 0;
	for (uint16_t j = 0; j < lookup->subtableCount; j++) {
		json_value *_subtable = _subtables->u.array.values[j];
		if (_subtable && _subtable->type == json_object) {
			json_value *_from = json_obj_get_type(_subtable, "from", json_array);
			json_value *_to = json_obj_get_type(_subtable, "to", json_array);
			if (_from && _to) {
				otl_subtable *st;
				NEW(st);
				st->gsub_single.from = caryll_coverage_from_json(_from);
				st->gsub_single.to = caryll_coverage_from_json(_to);
				lookup->subtables[jj] = st;
				jj += 1;
			}
		}
	}
	lookup->subtableCount = jj;
	return lookup;
FAIL:
	DELETE(caryll_delete_gsub_single, lookup);
	return NULL;
};
