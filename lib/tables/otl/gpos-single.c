#include "gpos-single.h"
#include "gpos-common.h"
void caryll_delete_gpos_single(otl_lookup *lookup) {
	if (lookup) {
		if (lookup->subtables) {
			for (uint16_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) {
					caryll_delete_coverage(lookup->subtables[j]->gpos_single.coverage);
					free(lookup->subtables[j]->gpos_single.values);
					free(lookup->subtables[j]);
				}
			free(lookup->subtables);
		}
		FREE(lookup);
	}
}
otl_subtable *caryll_read_gpos_single(font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	otl_subtable *_subtable;
	NEW(_subtable);
	subtable_gpos_single *subtable = &(_subtable->gpos_single);
	subtable->coverage = NULL;
	subtable->values = NULL;
	checkLength(offset + 6);

	uint16_t subtableFormat = read_16u(data + offset);
	subtable->coverage = caryll_read_coverage(data, tableLength, offset + read_16u(data + offset + 2));
	if (!subtable->coverage || subtable->coverage->numGlyphs == 0) goto FAIL;
	NEW_N(subtable->values, subtable->coverage->numGlyphs);

	if (subtableFormat == 1) {
		otl_position_value v = read_gpos_value(data, tableLength, offset + 6, read_16u(data + offset + 4));
		for (uint16_t j = 0; j < subtable->coverage->numGlyphs; j++) {
			subtable->values[j] = v;
		}
	} else {
		uint16_t valueFormat = read_16u(data + offset + 4);
		uint16_t valueCount = read_16u(data + offset + 6);
		checkLength(offset + 8 + position_format_length(valueFormat) * valueCount);
		if (valueCount != subtable->coverage->numGlyphs) goto FAIL;

		for (uint16_t j = 0; j < subtable->coverage->numGlyphs; j++) {
			subtable->values[j] =
			    read_gpos_value(data, tableLength, offset + 8 + j * position_format_length(valueFormat), valueFormat);
		}
	}
	goto OK;
FAIL:
	if (subtable->coverage) caryll_delete_coverage(subtable->coverage);
	if (subtable->values) free(subtable->values);
	_subtable = NULL;
OK:
	return _subtable;
}

json_value *caryll_gpos_single_to_json(otl_subtable *_subtable) {
	subtable_gpos_single *subtable = &(_subtable->gpos_single);
	json_value *st = json_object_new(subtable->coverage->numGlyphs);
	for (uint16_t j = 0; j < subtable->coverage->numGlyphs; j++) {
		json_object_push(st, subtable->coverage->glyphs[j].name, gpos_value_to_json(subtable->values[j]));
	}
	return st;
}
otl_subtable *caryll_gpos_single_from_json(json_value *_subtable) {
	otl_subtable *_st;
	NEW(_st);
	subtable_gpos_single *subtable = &(_st->gpos_single);
	NEW(subtable->coverage);
	NEW_N(subtable->coverage->glyphs, _subtable->u.object.length);
	NEW_N(subtable->values, _subtable->u.object.length);
	uint16_t jj = 0;
	for (uint16_t j = 0; j < _subtable->u.object.length; j++) {
		if (_subtable->u.object.values[j].value && _subtable->u.object.values[j].value->type == json_object) {
			sds gname = sdsnewlen(_subtable->u.object.values[j].name, _subtable->u.object.values[j].name_length);
			subtable->coverage->glyphs[jj].name = gname;
			subtable->values[jj] = gpos_value_from_json(_subtable->u.object.values[j].value);
			jj++;
		}
	}
	subtable->coverage->numGlyphs = jj;
	return _st;
}

caryll_buffer *caryll_write_gpos_single(otl_subtable *_subtable) {
	subtable_gpos_single *subtable = &(_subtable->gpos_single);
	bool isConst = subtable->coverage->numGlyphs > 0;
	uint16_t format = 0;
	if (subtable->coverage->numGlyphs > 0) {
		for (uint16_t j = 0; j < subtable->coverage->numGlyphs; j++) {
			isConst = isConst && (subtable->values[j].dx == subtable->values[0].dx) &&
			          (subtable->values[j].dy == subtable->values[0].dy) &&
			          (subtable->values[j].dWidth == subtable->values[0].dWidth) &&
			          (subtable->values[j].dHeight == subtable->values[0].dHeight);
			format |= required_position_format(subtable->values[j]);
		}
	}
	if (isConst) {
		return caryll_write_bk(
		    new_bkblock(b16, 1,                                                                  // Format
		                p16, new_bkblock_from_buffer(caryll_write_coverage(subtable->coverage)), // coverage
		                b16, format,                                                             // format
		                bembed, bk_gpos_value(subtable->values[0], format),                      // value
		                bkover));
	} else {
		caryll_bkblock *b =
		    new_bkblock(b16, 2,                                                                  // Format
		                p16, new_bkblock_from_buffer(caryll_write_coverage(subtable->coverage)), // coverage
		                b16, format,                                                             // format
		                b16, subtable->coverage->numGlyphs,                                      // quantity
		                bkover);
		for (uint16_t k = 0; k < subtable->coverage->numGlyphs; k++) {
			bkblock_push(b, bembed, bk_gpos_value(subtable->values[k], format), // value
			             bkover);
		}
		return caryll_write_bk(b);
	}
}
