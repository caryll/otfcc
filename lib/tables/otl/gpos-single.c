#include "gpos-single.h"
#include "gpos-common.h"
void otl_delete_gpos_single(otl_Subtable *subtable) {
	if (subtable) {
		otl_delete_Coverage(subtable->gpos_single.coverage);
		free(subtable->gpos_single.values);
		free(subtable);
	}
}

otl_Subtable *otl_read_gpos_single(const font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	otl_Subtable *_subtable;
	NEW(_subtable);
	subtable_gpos_single *subtable = &(_subtable->gpos_single);
	subtable->coverage = NULL;
	subtable->values = NULL;
	checkLength(offset + 6);

	uint16_t subtableFormat = read_16u(data + offset);
	subtable->coverage = otl_read_Coverage(data, tableLength, offset + read_16u(data + offset + 2));
	if (!subtable->coverage || subtable->coverage->numGlyphs == 0) goto FAIL;
	NEW_N(subtable->values, subtable->coverage->numGlyphs);

	if (subtableFormat == 1) {
		otl_PositionValue v = read_gpos_value(data, tableLength, offset + 6, read_16u(data + offset + 4));
		for (glyphid_t j = 0; j < subtable->coverage->numGlyphs; j++) {
			subtable->values[j] = v;
		}
	} else {
		uint16_t valueFormat = read_16u(data + offset + 4);
		uint16_t valueCount = read_16u(data + offset + 6);
		checkLength(offset + 8 + position_format_length(valueFormat) * valueCount);
		if (valueCount != subtable->coverage->numGlyphs) goto FAIL;

		for (glyphid_t j = 0; j < subtable->coverage->numGlyphs; j++) {
			subtable->values[j] =
			    read_gpos_value(data, tableLength, offset + 8 + j * position_format_length(valueFormat), valueFormat);
		}
	}
	goto OK;
FAIL:
	if (subtable->coverage) otl_delete_Coverage(subtable->coverage);
	if (subtable->values) free(subtable->values);
	_subtable = NULL;
OK:
	return _subtable;
}

json_value *otl_gpos_dump_single(const otl_Subtable *_subtable) {
	const subtable_gpos_single *subtable = &(_subtable->gpos_single);
	json_value *st = json_object_new(subtable->coverage->numGlyphs);
	for (glyphid_t j = 0; j < subtable->coverage->numGlyphs; j++) {
		json_object_push(st, subtable->coverage->glyphs[j].name, gpos_dump_value(subtable->values[j]));
	}
	return st;
}
otl_Subtable *otl_gpos_parse_single(const json_value *_subtable, const otfcc_Options *options) {
	otl_Subtable *_st;
	NEW(_st);
	subtable_gpos_single *subtable = &(_st->gpos_single);
	NEW(subtable->coverage);
	NEW_N(subtable->coverage->glyphs, _subtable->u.object.length);
	NEW_N(subtable->values, _subtable->u.object.length);
	glyphid_t jj = 0;
	for (glyphid_t j = 0; j < _subtable->u.object.length; j++) {
		if (_subtable->u.object.values[j].value && _subtable->u.object.values[j].value->type == json_object) {
			sds gname = sdsnewlen(_subtable->u.object.values[j].name, _subtable->u.object.values[j].name_length);
			subtable->coverage->glyphs[jj] = handle_fromName(gname);
			subtable->values[jj] = gpos_parse_value(_subtable->u.object.values[j].value);
			jj++;
		}
	}
	subtable->coverage->numGlyphs = jj;
	return _st;
}

caryll_Buffer *caryll_build_gpos_single(const otl_Subtable *_subtable) {
	const subtable_gpos_single *subtable = &(_subtable->gpos_single);
	bool isConst = subtable->coverage->numGlyphs > 0;
	uint16_t format = 0;
	if (subtable->coverage->numGlyphs > 0) {
		for (glyphid_t j = 0; j < subtable->coverage->numGlyphs; j++) {
			isConst = isConst && (subtable->values[j].dx == subtable->values[0].dx) &&
			          (subtable->values[j].dy == subtable->values[0].dy) &&
			          (subtable->values[j].dWidth == subtable->values[0].dWidth) &&
			          (subtable->values[j].dHeight == subtable->values[0].dHeight);
			format |= required_position_format(subtable->values[j]);
		}
	}
	if (isConst) {
		return bk_build_Block(bk_new_Block(b16, 1, // Format
		                                   p16,
		                                   bk_newBlockFromBuffer(otl_build_Coverage(subtable->coverage)), // coverage
		                                   b16, format,                                                   // format
		                                   bkembed, bk_gpos_value(subtable->values[0], format),           // value
		                                   bkover));
	} else {
		bk_Block *b = bk_new_Block(b16, 2,                                                             // Format
		                           p16, bk_newBlockFromBuffer(otl_build_Coverage(subtable->coverage)), // coverage
		                           b16, format,                                                        // format
		                           b16, subtable->coverage->numGlyphs,                                 // quantity
		                           bkover);
		for (glyphid_t k = 0; k < subtable->coverage->numGlyphs; k++) {
			bk_push(b, bkembed, bk_gpos_value(subtable->values[k], format), // value
			        bkover);
		}
		return bk_build_Block(b);
	}
}
