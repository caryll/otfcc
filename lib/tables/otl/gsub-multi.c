#include "gsub-multi.h"
static void deleteGSUBMultiSubtable(otl_subtable *_subtable) {
	if (!_subtable) return;
	subtable_gsub_multi *subtable = &(_subtable->gsub_multi);
	if (subtable->from && subtable->to) {
		for (uint16_t j = 0; j < subtable->from->numGlyphs; j++) {
			caryll_delete_coverage(subtable->to[j]);
		}
		free(subtable->to);
	}
	caryll_delete_coverage(subtable->from);
	free(subtable);
}
void caryll_delete_gsub_multi(otl_lookup *lookup) {
	if (lookup) {
		if (lookup->subtables) {
			for (uint16_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) { deleteGSUBMultiSubtable(lookup->subtables[j]); }
			free(lookup->subtables);
		}
		FREE(lookup);
	}
}

otl_subtable *caryll_read_gsub_multi(font_file_pointer data, uint32_t tableLength,
                                     uint32_t offset) {
	otl_subtable *_subtable;
	NEW(_subtable);
	subtable_gsub_multi *subtable = &(_subtable->gsub_multi);
	subtable->from = NULL;
	subtable->to = NULL;
	checkLength(offset + 6);

	subtable->from = caryll_read_coverage(data, tableLength, offset + read_16u(data + offset + 2));
	if (!subtable->from) goto FAIL;
	uint16_t seqCount = read_16u(data + offset + 4);
	if (seqCount != subtable->from->numGlyphs) goto FAIL;
	checkLength(offset + 6 + seqCount * 2);

	NEW_N(subtable->to, seqCount);
	for (uint16_t j = 0; j < seqCount; j++) {
		uint32_t seqOffset = offset + read_16u(data + offset + 6 + j * 2);
		otl_coverage *cov;
		NEW(cov);
		cov->numGlyphs = read_16u(data + seqOffset);
		NEW_N(cov->glyphs, cov->numGlyphs);
		for (uint16_t k = 0; k < cov->numGlyphs; k++) {
			cov->glyphs[k].gid = read_16u(data + seqOffset + 2 + k * 2);
		}
		subtable->to[j] = cov;
	}
	return _subtable;

FAIL:
	deleteGSUBMultiSubtable(_subtable);
	return NULL;
}

json_value *caryll_gsub_multi_to_json(otl_subtable *_subtable) {
	subtable_gsub_multi *subtable = &(_subtable->gsub_multi);
	json_value *st = json_object_new(subtable->from->numGlyphs);
	for (uint16_t j = 0; j < subtable->from->numGlyphs; j++) {
		json_object_push(st, subtable->from->glyphs[j].name,
		                 caryll_coverage_to_json(subtable->to[j]));
	}
	return st;
}

otl_subtable *caryll_gsub_multi_from_json(json_value *_subtable) {
	otl_subtable *_st;
	NEW(_st);
	subtable_gsub_multi *st = &(_st->gsub_multi);
	NEW(st->from);
	st->from->numGlyphs = _subtable->u.object.length;
	NEW_N(st->from->glyphs, st->from->numGlyphs);
	NEW_N(st->to, st->from->numGlyphs);

	uint16_t jj = 0;
	for (uint16_t k = 0; k < st->from->numGlyphs; k++) {
		json_value *_to = _subtable->u.object.values[k].value;
		if (!_to || _to->type != json_array) continue;
		st->from->glyphs[jj].name = sdsnewlen(_subtable->u.object.values[k].name,
		                                      _subtable->u.object.values[k].name_length);
		st->to[jj] = caryll_coverage_from_json(_to);
		jj += 1;
	}
	st->from->numGlyphs = jj;
	return _st;
}

caryll_buffer *caryll_write_gsub_multi_subtable(otl_subtable *_subtable) {
	subtable_gsub_multi *subtable = &(_subtable->gsub_multi);
	caryll_buffer *buf = bufnew();
	bufwrite16b(buf, 1);

	size_t offset = 6 + 2 * subtable->from->numGlyphs;
	size_t cp = buf->cursor;
	bufpingpong16b(buf, caryll_write_coverage(subtable->from), &offset, &cp);
	bufwrite16b(buf, subtable->from->numGlyphs);
	for (uint16_t j = 0; j < subtable->from->numGlyphs; j++) {
		bufping16b(buf, &offset, &cp);
		bufwrite16b(buf, subtable->to[j]->numGlyphs);
		for (uint16_t k = 0; k < subtable->to[j]->numGlyphs; k++) {
			bufwrite16b(buf, subtable->to[j]->glyphs[k].gid);
		}
		bufpong(buf, &offset, &cp);
	}
	return buf;
}
