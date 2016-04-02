#include "otl-gpos-mark-to-base.h"
#include "otl-gpos-common.h"

static void delete_mtb_subtable(otl_subtable *_subtable) {
	if (_subtable) {
		subtable_gpos_mark_to_base *subtable = &(_subtable->gpos_mark_to_base);
		if (subtable->marks) { caryll_delete_coverage(subtable->marks); }
		if (subtable->markArray) { otl_delete_mark_array(subtable->markArray); }
		if (subtable->bases) {
			if (subtable->baseArray) {
				for (uint16_t j = 0; j < subtable->bases->numGlyphs; j++) {
					if (subtable->baseArray[j]) free(subtable->baseArray[j]);
				}
				free(subtable->baseArray);
			}
			caryll_delete_coverage(subtable->bases);
		}
		free(_subtable);
	}
}

void caryll_delete_gpos_mark_to_base(otl_lookup *lookup) {
	if (lookup) {
		if (lookup->subtables)
			for (uint16_t j = 0; j < lookup->subtableCount; j++)
				delete_mtb_subtable(lookup->subtables[j]);
		free(lookup);
	}
}

otl_subtable *caryll_read_gpos_mark_to_base(font_file_pointer data, uint32_t tableLength,
                                                     uint32_t subtableOffset) {
	otl_subtable *_subtable;
	NEW(_subtable);
	subtable_gpos_mark_to_base *subtable = &(_subtable->gpos_mark_to_base);
	if (tableLength < subtableOffset + 12) goto FAIL;

	subtable->marks = caryll_read_coverage(data, tableLength, subtableOffset + read_16u(data + subtableOffset + 2));
	subtable->bases = caryll_read_coverage(data, tableLength, subtableOffset + read_16u(data + subtableOffset + 4));
	if (!subtable->marks || subtable->marks->numGlyphs == 0 || !subtable->bases || subtable->bases->numGlyphs == 0)
		goto FAIL;

	subtable->classCount = read_16u(data + subtableOffset + 6);
	uint16_t markArrayOffset = subtableOffset + read_16u(data + subtableOffset + 8);
	subtable->markArray = otl_read_mark_array(data, tableLength, markArrayOffset);
	if (!subtable->markArray || subtable->markArray->markCount != subtable->marks->numGlyphs) goto FAIL;

	uint16_t baseArrayOffset = subtableOffset + read_16u(data + subtableOffset + 10);
	checkLength(baseArrayOffset + 2 + 2 * subtable->bases->numGlyphs * subtable->classCount);
	if (read_16u(data + baseArrayOffset) != subtable->bases->numGlyphs) goto FAIL;
	NEW_N(subtable->baseArray, subtable->bases->numGlyphs);
	uint16_t _offset = baseArrayOffset + 2;
	for (uint16_t j = 0; j < subtable->bases->numGlyphs; j++) {
		NEW_N(subtable->baseArray[j], subtable->classCount);
		for (uint16_t k = 0; k < subtable->classCount; k++) {
			if (read_16u(data + _offset)) {
				subtable->baseArray[j][k] =
				    otl_read_anchor(data, tableLength, baseArrayOffset + read_16u(data + _offset));
			} else {
				subtable->baseArray[j][k].x = 0;
				subtable->baseArray[j][k].y = 0;
			}
			_offset += 2;
		}
	}
	goto OK;
FAIL:
	DELETE(delete_mtb_subtable, _subtable);
OK:
	return _subtable;
}

void caryll_gpos_mark_to_base_to_json(otl_lookup *lookup, json_value *dump) {
	json_object_push(dump, "type", json_string_new("gpos_mark_to_base"));
	json_value *_subtables = json_array_new(lookup->subtableCount);
	for (uint16_t j = 0; j < lookup->subtableCount; j++)
		if (lookup->subtables[j]) {
			subtable_gpos_mark_to_base *subtable = &(lookup->subtables[j]->gpos_mark_to_base);
			json_value *_subtable = json_object_new(3);
			json_value *_marks = json_object_new(subtable->marks->numGlyphs);
			json_value *_bases = json_object_new(subtable->bases->numGlyphs);
			for (uint16_t j = 0; j < subtable->marks->numGlyphs; j++) {
				json_value *_mark = json_object_new(3);
				json_object_push(_mark, "class", json_integer_new(subtable->markArray->records[j].markClass));
				json_object_push(_mark, "x", json_integer_new(subtable->markArray->records[j].anchor.x));
				json_object_push(_mark, "y", json_integer_new(subtable->markArray->records[j].anchor.y));
				json_object_push(_marks, subtable->marks->glyphs[j].name, preserialize(_mark));
			}
			for (uint16_t j = 0; j < subtable->bases->numGlyphs; j++) {
				json_value *_base = json_array_new(subtable->classCount);
				for (uint16_t k = 0; k < subtable->classCount; k++) {
					json_value *_anchor = json_object_new(2);
					json_object_push(_anchor, "x", json_integer_new(subtable->baseArray[j][k].x));
					json_object_push(_anchor, "y", json_integer_new(subtable->baseArray[j][k].y));
					json_array_push(_base, _anchor);
				}
				json_object_push(_bases, subtable->bases->glyphs[j].name, preserialize(_base));
			}
			json_object_push(_subtable, "classCount", json_integer_new(subtable->classCount));
			json_object_push(_subtable, "marks", _marks);
			json_object_push(_subtable, "bases", _bases);
			json_array_push(_subtables, _subtable);
		}
	json_object_push(dump, "subtables", _subtables);
}
