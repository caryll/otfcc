#include "gsub-single.h"
void otl_delete_gsub_single(otl_Subtable *subtable) {
	if (subtable) {
		otl_delete_Coverage(subtable->gsub_single.from);
		otl_delete_Coverage(subtable->gsub_single.to);
		FREE(subtable);
	}
}

otl_Subtable *otl_read_gsub_single(const font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset,
                                   const otfcc_Options *options) {
	otl_Subtable *subtable;
	NEW(subtable);
	if (tableLength < subtableOffset + 6) goto FAIL;
	uint16_t subtableFormat = read_16u(data + subtableOffset);
	otl_Coverage *from = otl_read_Coverage(data, tableLength, subtableOffset + read_16u(data + subtableOffset + 2));
	subtable->gsub_single.from = from;
	if (!from || from->numGlyphs == 0) goto FAIL;

	if (subtableFormat == 1) {
		otl_Coverage *to;
		NEW(to);
		to->numGlyphs = from->numGlyphs;
		NEW(to->glyphs, to->numGlyphs);

		uint16_t delta = read_16u(data + subtableOffset + 4);
		for (glyphid_t j = 0; j < from->numGlyphs; j++) {
			to->glyphs[j] = handle_fromIndex(from->glyphs[j].index + delta);
		}
		subtable->gsub_single.to = to;
	} else {
		glyphid_t toglyphs = read_16u(data + subtableOffset + 4);
		if (tableLength < subtableOffset + 6 + toglyphs * 2 || toglyphs != from->numGlyphs) goto FAIL;
		otl_Coverage *to;
		NEW(to);
		to->numGlyphs = toglyphs;
		NEW(to->glyphs, to->numGlyphs);

		for (glyphid_t j = 0; j < to->numGlyphs; j++) {
			to->glyphs[j] = handle_fromIndex(read_16u(data + subtableOffset + 6 + j * 2));
		}
		subtable->gsub_single.to = to;
	}
	goto OK;
FAIL:
	if (subtable->gsub_single.from) otl_delete_Coverage(subtable->gsub_single.from);
	if (subtable->gsub_single.to) otl_delete_Coverage(subtable->gsub_single.to);
	subtable = NULL;
OK:
	return subtable;
}

json_value *otl_gsub_dump_single(const otl_Subtable *_subtable) {
	const subtable_gsub_single *subtable = &(_subtable->gsub_single);
	json_value *st = json_object_new(subtable->from->numGlyphs);
	for (glyphid_t j = 0; j < subtable->from->numGlyphs && j < subtable->from->numGlyphs; j++) {
		json_object_push(st, subtable->from->glyphs[j].name, json_string_new(subtable->to->glyphs[j].name));
	}
	return st;
}

otl_Subtable *otl_gsub_parse_single(const json_value *_subtable, const otfcc_Options *options) {
	otl_Subtable *_st;
	NEW(_st);
	subtable_gsub_single *subtable = &(_st->gsub_single);
	NEW(subtable->from);
	NEW(subtable->to);
	subtable->from->numGlyphs = subtable->to->numGlyphs = _subtable->u.object.length;
	NEW(subtable->from->glyphs, subtable->from->numGlyphs);
	NEW(subtable->to->glyphs, subtable->to->numGlyphs);
	glyphid_t jj = 0;
	for (glyphid_t j = 0; j < _subtable->u.object.length; j++) {
		if (_subtable->u.object.values[j].value && _subtable->u.object.values[j].value->type == json_string) {
			subtable->from->glyphs[jj] = handle_fromName(
			    sdsnewlen(_subtable->u.object.values[j].name, _subtable->u.object.values[j].name_length));
			subtable->to->glyphs[jj] = handle_fromName(sdsnewlen(_subtable->u.object.values[j].value->u.string.ptr,
			                                                     _subtable->u.object.values[j].value->u.string.length));
			jj++;
		}
	}
	subtable->from->numGlyphs = subtable->to->numGlyphs = jj;
	return _st;
};

caryll_Buffer *otfcc_build_gsub_single_subtable(const otl_Subtable *_subtable) {
	const subtable_gsub_single *subtable = &(_subtable->gsub_single);
	bool isConstantDifference = true;
	if (subtable->from->numGlyphs > 1) {
		int32_t difference = subtable->to->glyphs[0].index - subtable->from->glyphs[0].index;
		for (glyphid_t j = 1; j < subtable->from->numGlyphs; j++) {
			isConstantDifference = isConstantDifference &&
			                       ((subtable->to->glyphs[j].index - subtable->from->glyphs[j].index) == difference);
		}
	}
	if (isConstantDifference && subtable->from->numGlyphs > 0) {
		return bk_build_Block(bk_new_Block(b16, 1,                                                         // Format
		                                   p16, bk_newBlockFromBuffer(otl_build_Coverage(subtable->from)), // coverage
		                                   b16,
		                                   subtable->to->glyphs[0].index - subtable->from->glyphs[0].index, // delta
		                                   bkover));
	} else {
		bk_Block *b = bk_new_Block(b16, 2,                                                         // Format
		                           p16, bk_newBlockFromBuffer(otl_build_Coverage(subtable->from)), // coverage
		                           b16, subtable->to->numGlyphs,                                   // quantity
		                           bkover);
		for (glyphid_t k = 0; k < subtable->to->numGlyphs; k++) {
			bk_push(b, b16, subtable->to->glyphs[k].index, bkover);
		}
		return bk_build_Block(b);
	}
}
