#include "support/util.h"
#include "otfcc/table/otl/coverage.h"

void otl_delete_Coverage(MOVE otl_Coverage *coverage) {
	if (coverage && coverage->glyphs) {
		for (glyphid_t j = 0; j < coverage->numGlyphs; j++) {
			handle_dispose(&coverage->glyphs[j]);
		}
		FREE(coverage->glyphs);
	}
	if (coverage) FREE(coverage);
}

typedef struct {
	int gid;
	int covIndex;
	UT_hash_handle hh;
} coverage_entry;

static int by_covIndex(coverage_entry *a, coverage_entry *b) {
	return a->covIndex - b->covIndex;
}

otl_Coverage *otl_read_Coverage(const uint8_t *data, uint32_t tableLength, uint32_t offset) {
	otl_Coverage *coverage;
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
			if (coverage->numGlyphs) {
				NEW_N(coverage->glyphs, coverage->numGlyphs);
				{
					glyphid_t j = 0;
					coverage_entry *e, *tmp;
					HASH_ITER(hh, hash, e, tmp) {
						coverage->glyphs[j] = handle_fromIndex(e->gid);
						HASH_DEL(hash, e);
						FREE(e);
						j++;
					}
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
			if (coverage->numGlyphs) {
				NEW_N(coverage->glyphs, coverage->numGlyphs);
				{
					glyphid_t j = 0;
					coverage_entry *e, *tmp;
					HASH_ITER(hh, hash, e, tmp) {
						coverage->glyphs[j] = handle_fromIndex(e->gid);
						HASH_DEL(hash, e);
						FREE(e);
						j++;
					}
				}
			}
			break;
		}
		default:
			break;
	}
	return coverage;
}

json_value *otl_dump_Coverage(const otl_Coverage *coverage) {
	json_value *a = json_array_new(coverage->numGlyphs);
	for (glyphid_t j = 0; j < coverage->numGlyphs; j++) {
		json_array_push(a, json_string_new(coverage->glyphs[j].name));
	}
	return preserialize(a);
}

otl_Coverage *otl_parse_Coverage(const json_value *cov) {
	otl_Coverage *c;
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
	glyphid_t jj = 0;
	for (glyphid_t j = 0; j < c->numGlyphs; j++) {
		if (cov->u.array.values[j]->type == json_string) {
			c->glyphs[jj] = handle_fromName(
			    sdsnewlen(cov->u.array.values[j]->u.string.ptr, cov->u.array.values[j]->u.string.length));
			jj++;
		}
	}
	c->numGlyphs = jj;
	return c;
}

static int by_gid(const void *a, const void *b) {
	return *((glyphid_t *)a) - *((glyphid_t *)b);
}
caryll_Buffer *otl_build_Coverage(const otl_Coverage *coverage) {
	// sort the gids in coverage
	if (!coverage->numGlyphs) {
		caryll_Buffer *buf = bufnew();
		bufwrite16b(buf, 2);
		bufwrite16b(buf, 0);
		return buf;
	}
	glyphid_t *r;
	NEW_N(r, coverage->numGlyphs);
	glyphid_t jj = 0;
	for (glyphid_t j = 0; j < coverage->numGlyphs; j++) {
		r[jj] = coverage->glyphs[j].index;
		jj++;
	}
	qsort(r, jj, sizeof(glyphid_t), by_gid);

	caryll_Buffer *format1 = bufnew();
	bufwrite16b(format1, 1);
	bufwrite16b(format1, jj);
	for (glyphid_t j = 0; j < jj; j++) {
		bufwrite16b(format1, r[j]);
	}
	if (jj < 2) {
		FREE(r);
		return format1;
	}

	caryll_Buffer *format2 = bufnew();
	bufwrite16b(format2, 2);
	caryll_Buffer *ranges = bufnew();
	glyphid_t startGID = r[0];
	glyphid_t endGID = startGID;
	glyphid_t lastGID = startGID;
	glyphid_t nRanges = 0;
	for (glyphid_t j = 1; j < jj; j++) {
		glyphid_t current = r[j];
		if (current <= lastGID) continue;
		if (current == endGID + 1) {
			endGID = current;
		} else {
			bufwrite16b(ranges, startGID);
			bufwrite16b(ranges, endGID);
			bufwrite16b(ranges, j + startGID - endGID - 1);
			nRanges += 1;
			startGID = endGID = current;
		}
		lastGID = current;
	}
	bufwrite16b(ranges, startGID);
	bufwrite16b(ranges, endGID);
	bufwrite16b(ranges, jj + startGID - endGID - 1);
	nRanges += 1;
	bufwrite16b(format2, nRanges);
	bufwrite_bufdel(format2, ranges);
	if (buflen(format1) < buflen(format2)) {
		buffree(format2);
		FREE(r);
		return format1;
	} else {
		buffree(format1);
		FREE(r);
		return format2;
	}
}

static int byHandleGID(const void *a, const void *b) {
	return ((glyph_handle *)a)->index - ((glyph_handle *)b)->index;
}

void fontop_shrinkCoverage(otl_Coverage *coverage, bool dosort) {
	if (!coverage) return;
	glyphid_t k = 0;
	for (glyphid_t j = 0; j < coverage->numGlyphs; j++) {
		if (coverage->glyphs[j].name) {
			coverage->glyphs[k++] = coverage->glyphs[j];
		} else {
			handle_dispose(&coverage->glyphs[j]);
		}
	}
	if (dosort) {
		qsort(coverage->glyphs, k, sizeof(glyph_handle), byHandleGID);
		glyphid_t skip = 0;
		for (glyphid_t rear = 1; rear < coverage->numGlyphs; rear++) {
			if (coverage->glyphs[rear].index == coverage->glyphs[rear - skip - 1].index) {
				handle_dispose(&coverage->glyphs[rear]);
				skip += 1;
			} else {
				coverage->glyphs[rear - skip] = coverage->glyphs[rear];
			}
		}
		k -= skip;
	}
	coverage->numGlyphs = k;
}
