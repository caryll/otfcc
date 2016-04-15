#include "coverage.h"

void caryll_delete_coverage(otl_coverage *coverage) {
	if (coverage && coverage->glyphs) free(coverage->glyphs);
	if (coverage) free(coverage);
}

typedef struct {
	int gid;
	int covIndex;
	UT_hash_handle hh;
} coverage_entry;

static int by_covIndex(coverage_entry *a, coverage_entry *b) { return a->covIndex - b->covIndex; }

otl_coverage *caryll_read_coverage(font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	otl_coverage *coverage;
	NEW(coverage);
	coverage->numGlyphs = 0;
	coverage->glyphs = NULL;
	if (tableLength < offset + 4) return coverage;
	uint16_t format = read_16u(data + offset);
	switch (format) {
		case 1: {
			uint16_t glyphCount = read_16u(data + offset + 2);
			if (tableLength < offset + 4 + glyphCount * 2) return coverage;
			coverage_entry *hash = NULL;
			for (uint16_t j = 0; j < glyphCount; j++) {
				coverage_entry *item = NULL;
				int gid = read_16u(data + offset + 4 + j * 2);
				HASH_FIND_INT(hash, &gid, item);
				if (!item) {
					NEW(item);
					item->gid = gid;
					item->covIndex = j;
					HASH_ADD_INT(hash, gid, item);
				}
			}
			HASH_SORT(hash, by_covIndex);
			coverage->numGlyphs = HASH_COUNT(hash);
			NEW_N(coverage->glyphs, coverage->numGlyphs);
			{
				uint16_t j = 0;
				coverage_entry *e, *tmp;
				HASH_ITER(hh, hash, e, tmp) {
					coverage->glyphs[j].gid = e->gid;
					coverage->glyphs[j].name = NULL;
					HASH_DEL(hash, e);
					free(e);
					j++;
				}
			}
			break;
		}
		case 2: {
			uint16_t rangeCount = read_16u(data + offset + 2);
			if (tableLength < offset + 4 + rangeCount * 6) return coverage;
			coverage_entry *hash = NULL;
			for (uint16_t j = 0; j < rangeCount; j++) {
				uint16_t start = read_16u(data + offset + 4 + 6 * j);
				uint16_t end = read_16u(data + offset + 4 + 6 * j + 2);
				uint16_t startCoverageIndex = read_16u(data + offset + 4 + 6 * j + 4);
				for (int k = start; k <= end; k++) {
					coverage_entry *item = NULL;
					HASH_FIND_INT(hash, &k, item);
					if (!item) {
						NEW(item);
						item->gid = k;
						item->covIndex = startCoverageIndex + k;
						HASH_ADD_INT(hash, gid, item);
					}
				}
			}
			HASH_SORT(hash, by_covIndex);
			coverage->numGlyphs = HASH_COUNT(hash);
			NEW_N(coverage->glyphs, coverage->numGlyphs);
			{
				uint16_t j = 0;
				coverage_entry *e, *tmp;
				HASH_ITER(hh, hash, e, tmp) {
					coverage->glyphs[j].gid = e->gid;
					coverage->glyphs[j].name = NULL;
					HASH_DEL(hash, e);
					free(e);
					j++;
				}
			}
			break;
		}
		default:
			break;
	}
	return coverage;
}

json_value *caryll_coverage_to_json(otl_coverage *coverage) {
	json_value *a = json_array_new(coverage->numGlyphs);
	for (uint16_t j = 0; j < coverage->numGlyphs; j++) {
		json_array_push(a, json_string_new(coverage->glyphs[j].name));
	}
	return preserialize(a);
}

otl_coverage *caryll_coverage_from_json(json_value *cov) {
	otl_coverage *c;
	NEW(c);
	c->numGlyphs = 0;
	c->glyphs = NULL;
	if (!cov || cov->type != json_array) return c;

	c->numGlyphs = cov->u.array.length;
	if (!c->numGlyphs) {
		c->glyphs = NULL;
		return c;
	}
	NEW_N(c->glyphs, c->numGlyphs);
	uint16_t jj = 0;
	for (uint16_t j = 0; j < c->numGlyphs; j++) {
		if (cov->u.array.values[j]->type == json_string) {
			c->glyphs[jj].gid = 0;
			c->glyphs[jj].name = sdsnewlen(cov->u.array.values[j]->u.string.ptr,
			                               cov->u.array.values[j]->u.string.length);
			jj++;
		}
	}
	c->numGlyphs = jj;
	return c;
}

caryll_buffer *caryll_write_coverage(otl_coverage *coverage) {
	caryll_buffer *format1 = bufnew();
	bufwrite16b(format1, 1);
	bufwrite16b(format1, coverage->numGlyphs);
	for (uint16_t j = 0; j < coverage->numGlyphs; j++) {
		bufwrite16b(format1, coverage->glyphs[j].gid);
	}
	if (coverage->numGlyphs < 2) return format1;

	caryll_buffer *format2 = bufnew();
	bufwrite16b(format2, 2);
	caryll_buffer *ranges = bufnew();
	uint16_t startGID = coverage->glyphs[0].gid;
	uint16_t endGID = startGID;
	uint16_t nRanges = 0;
	for (uint16_t j = 1; j < coverage->numGlyphs; j++) {
		uint16_t current = coverage->glyphs[j].gid;
		if (current == endGID + 1) {
			endGID = current;
		} else {
			bufwrite16b(ranges, startGID);
			bufwrite16b(ranges, endGID);
			bufwrite16b(ranges, j + startGID - endGID - 1);
			nRanges += 1;
			startGID = endGID = current;
		}
	}
	bufwrite16b(ranges, startGID);
	bufwrite16b(ranges, endGID);
	bufwrite16b(ranges, coverage->numGlyphs + startGID - endGID - 1);
	nRanges += 1;
	bufwrite16b(format2, nRanges);
	bufwrite_bufdel(format2, ranges);
	if (buflen(format1) < buflen(format2)) {
		buffree(format2);
		return format1;
	} else {
		buffree(format1);
		return format2;
	}
}
