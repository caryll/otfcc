#include "gsub-reverse.h"

void caryll_delete_gsub_reverse(otl_subtable *_subtable) {
	if (_subtable) {
		subtable_gsub_reverse *subtable = &(_subtable->gsub_reverse);
		if (subtable->match)
			for (uint16_t j = 0; j < subtable->matchCount; j++) {
				caryll_delete_coverage(subtable->match[j]);
			}
		if (subtable->to) caryll_delete_coverage(subtable->to);
		free(_subtable);
	}
}

static void reverseBacktracks(subtable_gsub_reverse *subtable) {
	if (subtable->inputIndex > 0) {
		uint16_t start = 0;
		uint16_t end = subtable->inputIndex - 1;
		while (end > start) {
			otl_coverage *tmp = subtable->match[start];
			subtable->match[start] = subtable->match[end];
			subtable->match[end] = tmp;
			end--, start++;
		}
	}
}

otl_subtable *caryll_read_gsub_reverse(font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	otl_subtable *_subtable;
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
		subtable->match[j] = caryll_read_coverage(data, tableLength, covOffset);
	}
	{
		uint32_t covOffset = offset + read_16u(data + offset + 2);
		subtable->match[subtable->inputIndex] = caryll_read_coverage(data, tableLength, covOffset);
		if (nReplacement != subtable->match[subtable->inputIndex]->numGlyphs) goto FAIL;
	}
	for (uint16_t j = 0; j < nForward; j++) {
		uint32_t covOffset = offset + read_16u(data + offset + 8 + nBacktrack * 2 + j * 2);
		subtable->match[nBacktrack + 1 + j] = caryll_read_coverage(data, tableLength, covOffset);
	}

	NEW(subtable->to);
	subtable->to->numGlyphs = nReplacement;
	NEW_N(subtable->to->glyphs, nReplacement);
	for (uint16_t j = 0; j < nReplacement; j++) {
		subtable->to->glyphs[j] = handle_from_id(read_16u(data + offset + 10 + (nBacktrack + nForward + j) * 2));
	}
	reverseBacktracks(subtable);
	return _subtable;

FAIL:
	caryll_delete_gsub_reverse(_subtable);
	return NULL;
}

json_value *caryll_gsub_reverse_to_json(otl_subtable *_subtable) {
	subtable_gsub_reverse *subtable = &(_subtable->gsub_reverse);
	json_value *_st = json_object_new(3);
	json_value *_match = json_array_new(subtable->matchCount);
	for (uint16_t j = 0; j < subtable->matchCount; j++) {
		json_array_push(_match, caryll_coverage_to_json(subtable->match[j]));
	}
	json_object_push(_st, "match", _match);
	json_object_push(_st, "to", caryll_coverage_to_json(subtable->to));
	json_object_push(_st, "inputIndex", json_integer_new(subtable->inputIndex));
	return _st;
}

otl_subtable *caryll_gsub_reverse_from_json(json_value *_subtable) {
	json_value *_match = json_obj_get_type(_subtable, "match", json_array);
	json_value *_to = json_obj_get_type(_subtable, "to", json_array);
	if (!_match || !_to) return NULL;

	otl_subtable *_st;
	NEW(_st);
	subtable_gsub_reverse *subtable = &(_st->gsub_reverse);

	subtable->matchCount = _match->u.array.length;
	NEW_N(subtable->match, subtable->matchCount);

	subtable->inputIndex = json_obj_getnum_fallback(_subtable, "inputIndex", 0);

	for (uint16_t j = 0; j < subtable->matchCount; j++) {
		subtable->match[j] = caryll_coverage_from_json(_match->u.array.values[j]);
	}
	subtable->to = caryll_coverage_from_json(_to);
	return _st;
}

caryll_buffer *caryll_write_gsub_reverse(otl_subtable *_subtable) {
	subtable_gsub_reverse *subtable = &(_subtable->gsub_reverse);
	reverseBacktracks(subtable);

	caryll_bkblock *root = new_bkblock(
	    b16, 1,                                                                                     // format
	    p16, new_bkblock_from_buffer(caryll_write_coverage(subtable->match[subtable->inputIndex])), // coverage
	    bkover);
	bkblock_push(root, b16, subtable->inputIndex, bkover);
	for (uint16_t j = 0; j < subtable->inputIndex; j++) {
		bkblock_push(root, p16, new_bkblock_from_buffer(caryll_write_coverage(subtable->match[j])), bkover);
	}
	bkblock_push(root, b16, subtable->matchCount - subtable->inputIndex - 1, bkover);
	for (uint16_t j = subtable->inputIndex + 1; j < subtable->matchCount; j++) {
		bkblock_push(root, p16, new_bkblock_from_buffer(caryll_write_coverage(subtable->match[j])), bkover);
	}
	bkblock_push(root, b16, subtable->to->numGlyphs, bkover);
	for (uint16_t j = 0; j < subtable->to->numGlyphs; j++) {
		bkblock_push(root, b16, subtable->to->glyphs[j].index, bkover);
	}

	return caryll_write_bk(root);
}
