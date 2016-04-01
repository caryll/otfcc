#include "otl-gsub-single.h"
void caryll_delete_gsub_single(otl_lookup *lookup) {
	for (uint16_t j = 0; j < lookup->subtableCount; j++)
		if (lookup->subtables[j]) {
			caryll_delete_coverage(lookup->subtables[j]->gsub_single.from);
			caryll_delete_coverage(lookup->subtables[j]->gsub_single.to);
		}
}

void caryll_read_gsub_single(font_file_pointer data, uint32_t tableLength, otl_lookup *lookup) {
	for (uint16_t j = 0; j < lookup->subtableCount; j++) {
		otl_subtable *subtable = malloc(sizeof(otl_subtable));
		if (!subtable) goto FAIL;
		uint32_t subtableOffset = lookup->_offset + caryll_blt16u(data + lookup->_offset + 6 + j * 2);
		if (tableLength < subtableOffset + 6) goto FAIL;
		uint16_t subtableFormat = caryll_blt16u(data + subtableOffset);
		otl_coverage *from =
		    caryll_read_coverage(data, tableLength, subtableOffset + caryll_blt16u(data + subtableOffset + 2));
		subtable->gsub_single.from = from;
		if (!from || from->numGlyphs == 0) goto FAIL;

		if (subtableFormat == 1) {
			otl_coverage *to = malloc(sizeof(otl_coverage));
			if (!to) goto FAIL;
			to->numGlyphs = from->numGlyphs;
			to->glyphs = malloc(to->numGlyphs * sizeof(glyph_handle));
			if (!to->glyphs) goto FAIL;

			uint16_t delta = caryll_blt16u(data + subtableOffset + 4);
			for (uint16_t j = 0; j < from->numGlyphs; j++) {
				to->glyphs[j].gid = from->glyphs[j].gid + delta;
				to->glyphs[j].name = NULL;
			}
			subtable->gsub_single.to = to;
		} else {
			uint16_t toglyphs = caryll_blt16u(data + subtableOffset + 4);
			if (tableLength < subtableOffset + 6 + toglyphs * 2 || toglyphs != from->numGlyphs) goto FAIL;
			otl_coverage *to = malloc(sizeof(otl_coverage));
			to->numGlyphs = toglyphs;
			to->glyphs = malloc(to->numGlyphs * sizeof(glyph_handle));
			if (!to->glyphs) goto FAIL;

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
