#include "gsub-reverse.h"

void otl_delete_gsub_reverse(otl_Subtable *_subtable) {
	if (_subtable) {
		subtable_gsub_reverse *subtable = &(_subtable->gsub_reverse);
		if (subtable->match)
			for (uint16_t j = 0; j < subtable->matchCount; j++) {
				otl_delete_Coverage(subtable->match[j]);
			}
		if (subtable->to) otl_delete_Coverage(subtable->to);
		free(_subtable);
	}
}

static void reverseBacktracks(subtable_gsub_reverse *subtable) {
	if (subtable->inputIndex > 0) {
		uint16_t start = 0;
		uint16_t end = subtable->inputIndex - 1;
		while (end > start) {
			otl_Coverage *tmp = subtable->match[start];
			subtable->match[start] = subtable->match[end];
			subtable->match[end] = tmp;
			end--, start++;
		}
	}
}

otl_Subtable *otl_read_gsub_reverse(font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	otl_Subtable *_subtable;
	NEW(_subtable);
	subtable_gsub_reverse *subtable = &(_subtable->gsub_reverse);
	subtable->match = NULL;
	subtable->to = NULL;
	checkLength(offset + 6);

	uint16_t nBacktrack = read_16u(data + offset + 4);
	checkLength(offset + 6 + nBacktrack * 2);

	uint16_t nForward = read_16u(data + offset + 6 + nBacktrack * 2);
	checkLength(offset + 8 + (nBacktrack + nForward) * 2);

	uint16_t nReplacement = read_16u(data + offset + 8 + (nBacktrack + nForward) * 2);
	checkLength(offset + 10 + (nBacktrack + nForward + nReplacement) * 2);

	subtable->matchCount = nBacktrack + nForward + 1;
	NEW_N(subtable->match, subtable->matchCount);
	subtable->inputIndex = nBacktrack;

	for (uint16_t j = 0; j < nBacktrack; j++) {
		uint32_t covOffset = offset + read_16u(data + offset + 6 + j * 2);
		subtable->match[j] = otl_read_Coverage(data, tableLength, covOffset);
	}
	{
		uint32_t covOffset = offset + read_16u(data + offset + 2);
		subtable->match[subtable->inputIndex] = otl_read_Coverage(data, tableLength, covOffset);
		if (nReplacement != subtable->match[subtable->inputIndex]->numGlyphs) goto FAIL;
	}
	for (uint16_t j = 0; j < nForward; j++) {
		uint32_t covOffset = offset + read_16u(data + offset + 8 + nBacktrack * 2 + j * 2);
		subtable->match[nBacktrack + 1 + j] = otl_read_Coverage(data, tableLength, covOffset);
	}

	NEW(subtable->to);
	subtable->to->numGlyphs = nReplacement;
	NEW_N(subtable->to->glyphs, nReplacement);
	for (uint16_t j = 0; j < nReplacement; j++) {
		subtable->to->glyphs[j] = handle_fromIndex(read_16u(data + offset + 10 + (nBacktrack + nForward + j) * 2));
	}
	reverseBacktracks(subtable);
	return _subtable;

FAIL:
	otl_delete_gsub_reverse(_subtable);
	return NULL;
}

json_value *otl_gsub_dump_reverse(otl_Subtable *_subtable) {
	subtable_gsub_reverse *subtable = &(_subtable->gsub_reverse);
	json_value *_st = json_object_new(3);
	json_value *_match = json_array_new(subtable->matchCount);
	for (uint16_t j = 0; j < subtable->matchCount; j++) {
		json_array_push(_match, otl_dump_Coverage(subtable->match[j]));
	}
	json_object_push(_st, "match", _match);
	json_object_push(_st, "to", otl_dump_Coverage(subtable->to));
	json_object_push(_st, "inputIndex", json_integer_new(subtable->inputIndex));
	return _st;
}

otl_Subtable *otl_gsub_parse_reverse(json_value *_subtable) {
	json_value *_match = json_obj_get_type(_subtable, "match", json_array);
	json_value *_to = json_obj_get_type(_subtable, "to", json_array);
	if (!_match || !_to) return NULL;

	otl_Subtable *_st;
	NEW(_st);
	subtable_gsub_reverse *subtable = &(_st->gsub_reverse);

	subtable->matchCount = _match->u.array.length;
	NEW_N(subtable->match, subtable->matchCount);

	subtable->inputIndex = json_obj_getnum_fallback(_subtable, "inputIndex", 0);

	for (uint16_t j = 0; j < subtable->matchCount; j++) {
		subtable->match[j] = otl_parse_Coverage(_match->u.array.values[j]);
	}
	subtable->to = otl_parse_Coverage(_to);
	return _st;
}

caryll_buffer *caryll_build_gsub_reverse(otl_Subtable *_subtable) {
	subtable_gsub_reverse *subtable = &(_subtable->gsub_reverse);
	reverseBacktracks(subtable);

	bk_Block *root =
	    bk_new_Block(b16, 1,                                                                                // format
	                 p16, bk_newBlockFromBuffer(otl_build_Coverage(subtable->match[subtable->inputIndex])), // coverage
	                 bkover);
	bk_push(root, b16, subtable->inputIndex, bkover);
	for (uint16_t j = 0; j < subtable->inputIndex; j++) {
		bk_push(root, p16, bk_newBlockFromBuffer(otl_build_Coverage(subtable->match[j])), bkover);
	}
	bk_push(root, b16, subtable->matchCount - subtable->inputIndex - 1, bkover);
	for (uint16_t j = subtable->inputIndex + 1; j < subtable->matchCount; j++) {
		bk_push(root, p16, bk_newBlockFromBuffer(otl_build_Coverage(subtable->match[j])), bkover);
	}
	bk_push(root, b16, subtable->to->numGlyphs, bkover);
	for (uint16_t j = 0; j < subtable->to->numGlyphs; j++) {
		bk_push(root, b16, subtable->to->glyphs[j].index, bkover);
	}

	return bk_build_Block(root);
}
