#include "gsub-multi.h"
void otl_delete_gsub_multi(otl_Subtable *_subtable) {
	if (!_subtable) return;
	subtable_gsub_multi *subtable = &(_subtable->gsub_multi);
	if (subtable->from && subtable->to) {
		for (glyphid_t j = 0; j < subtable->from->numGlyphs; j++) {
			otl_delete_Coverage(subtable->to[j]);
		}
		FREE(subtable->to);
	}
	otl_delete_Coverage(subtable->from);
	FREE(subtable);
}

otl_Subtable *otl_read_gsub_multi(font_file_pointer data, uint32_t tableLength, uint32_t offset,
                                  const otfcc_Options *options) {
	otl_Subtable *_subtable;
	NEW(_subtable);
	subtable_gsub_multi *subtable = &(_subtable->gsub_multi);
	subtable->from = NULL;
	subtable->to = NULL;
	checkLength(offset + 6);

	subtable->from = otl_read_Coverage(data, tableLength, offset + read_16u(data + offset + 2));
	if (!subtable->from) goto FAIL;
	glyphid_t seqCount = read_16u(data + offset + 4);
	if (seqCount != subtable->from->numGlyphs) goto FAIL;
	checkLength(offset + 6 + seqCount * 2);

	NEW(subtable->to, seqCount);
	for (glyphid_t j = 0; j < seqCount; j++) {
		uint32_t seqOffset = offset + read_16u(data + offset + 6 + j * 2);
		otl_Coverage *cov;
		NEW(cov);
		cov->numGlyphs = read_16u(data + seqOffset);
		NEW(cov->glyphs, cov->numGlyphs);
		for (glyphid_t k = 0; k < cov->numGlyphs; k++) {
			cov->glyphs[k] = handle_fromIndex(read_16u(data + seqOffset + 2 + k * 2));
		}
		subtable->to[j] = cov;
	}
	return _subtable;

FAIL:
	otl_delete_gsub_multi(_subtable);
	return NULL;
}

json_value *otl_gsub_dump_multi(const otl_Subtable *_subtable) {
	const subtable_gsub_multi *subtable = &(_subtable->gsub_multi);
	json_value *st = json_object_new(subtable->from->numGlyphs);
	for (glyphid_t j = 0; j < subtable->from->numGlyphs; j++) {
		json_object_push(st, subtable->from->glyphs[j].name, otl_dump_Coverage(subtable->to[j]));
	}
	return st;
}

otl_Subtable *otl_gsub_parse_multi(const json_value *_subtable, const otfcc_Options *options) {
	otl_Subtable *_st;
	NEW(_st);
	subtable_gsub_multi *st = &(_st->gsub_multi);
	NEW(st->from);
	st->from->numGlyphs = _subtable->u.object.length;
	NEW(st->from->glyphs, st->from->numGlyphs);
	NEW(st->to, st->from->numGlyphs);

	glyphid_t jj = 0;
	for (glyphid_t k = 0; k < st->from->numGlyphs; k++) {
		json_value *_to = _subtable->u.object.values[k].value;
		if (!_to || _to->type != json_array) continue;
		st->from->glyphs[jj] =
		    handle_fromName(sdsnewlen(_subtable->u.object.values[k].name, _subtable->u.object.values[k].name_length));
		st->to[jj] = otl_parse_Coverage(_to);
		jj += 1;
	}
	st->from->numGlyphs = jj;
	return _st;
}

caryll_Buffer *caryll_build_gsub_multi_subtable(const otl_Subtable *_subtable) {
	const subtable_gsub_multi *subtable = &(_subtable->gsub_multi);

	bk_Block *root = bk_new_Block(b16, 1,                                                         // format
	                              p16, bk_newBlockFromBuffer(otl_build_Coverage(subtable->from)), // coverage
	                              b16, subtable->from->numGlyphs,                                 // quantity
	                              bkover);
	for (glyphid_t j = 0; j < subtable->from->numGlyphs; j++) {
		bk_Block *b = bk_new_Block(b16, subtable->to[j]->numGlyphs, bkover);
		for (glyphid_t k = 0; k < subtable->to[j]->numGlyphs; k++) {
			bk_push(b, b16, subtable->to[j]->glyphs[k].index, bkover);
		}
		bk_push(root, p16, b, bkover);
	}
	return bk_build_Block(root);
}
