#include "otl-chaining.h"
static void deleteRule(otl_chaining_rule *rule) {
	if (rule && rule->match && rule->matchCount) {
		for (uint16_t k = 0; k < rule->matchCount; k++) {
			caryll_delete_coverage(rule->match[k]);
		}
	}
	if (rule && rule->apply) free(rule->apply);
	if (rule) free(rule);
}
static void delete_otl_chaining_subtable(otl_subtable *_subtable) {
	if (_subtable) {
		subtable_gsub_chaining *subtable = &(_subtable->gsub_chaining);
		if (subtable->rules) {
			for (uint16_t j = 0; j < subtable->rulesCount; j++) {
				deleteRule(subtable->rules[j]);
			}
			free(subtable->rules);
		}
		free(_subtable);
	}
}

static INLINE void reverseBacktracks(otl_chaining_rule *rule) {
	if (rule->inputBegins > 0) {
		uint16_t start = 0;
		uint16_t end = rule->inputBegins - 1;
		while (end > start) {
			otl_coverage *tmp = rule->match[start];
			rule->match[start] = rule->match[end];
			rule->match[end] = tmp;
			end--, start++;
		}
	}
}

typedef struct {
	otl_classdef *bc;
	otl_classdef *ic;
	otl_classdef *fc;
} classdefs;

otl_coverage *singleCoverage(font_file_pointer data, uint32_t tableLength, uint16_t gid, uint32_t _offset,
                             uint16_t kind, void *userdata) {
	otl_coverage *cov;
	NEW(cov);
	cov->numGlyphs = 1;
	NEW(cov->glyphs);
	cov->glyphs[0].gid = gid;
	cov->glyphs[0].name = NULL;
	return cov;
}
otl_coverage *classCoverage(font_file_pointer data, uint32_t tableLength, uint16_t cls, uint32_t _offset, uint16_t kind,
                            void *_classdefs) {
	classdefs *defs = (classdefs *)_classdefs;
	otl_classdef *cd = (kind == 1 ? defs->bc : kind == 2 ? defs->ic : defs->fc);
	otl_coverage *cov;
	NEW(cov);
	cov->numGlyphs = 0;
	cov->glyphs = NULL;
	uint16_t count = 0;
	for (uint16_t j = 0; j < cd->numGlyphs; j++)
		if (cd->classes[j] == cls) count++;
	if (!count) return cov;
	cov->numGlyphs = count;
	NEW_N(cov->glyphs, count);
	uint16_t jj = 0;
	for (uint16_t j = 0; j < cd->numGlyphs; j++)
		if (cd->classes[j] == cls) { cov->glyphs[jj++] = cd->glyphs[j]; }
	return cov;
}
otl_coverage *format3Coverage(font_file_pointer data, uint32_t tableLength, uint16_t shift, uint32_t _offset,
                              uint16_t kind, void *userdata) {
	return caryll_read_coverage(data, tableLength, _offset + shift - 2);
}
typedef otl_coverage *(*CoverageReaderHandler)(font_file_pointer, uint32_t, uint16_t, uint32_t, uint16_t, void *);
otl_chaining_rule *GeneralReadRule(font_file_pointer data, uint32_t tableLength, uint32_t offset, uint16_t startGID,
                                   bool minusOne, CoverageReaderHandler fn, void *userdata) {
	otl_chaining_rule *rule;
	NEW(rule);
	rule->match = NULL;
	rule->apply = NULL;

	checkLength(offset + 8);
	uint16_t backtrackGlyphCount = read_16u(data + offset);
	checkLength(offset + 2 + 2 * backtrackGlyphCount + 2);
	uint16_t inputGlyphCount = read_16u(data + offset + 2 + 2 * backtrackGlyphCount);
	checkLength(offset + 4 + 2 * (backtrackGlyphCount + inputGlyphCount - (minusOne ? 1 : 0)) + 2);
	uint16_t lookaheadGlyphCount =
	    read_16u(data + offset + 4 + 2 * (backtrackGlyphCount + inputGlyphCount - (minusOne ? 1 : 0)));
	checkLength(offset + 6 + 2 * (backtrackGlyphCount + inputGlyphCount - (minusOne ? 1 : 0) + lookaheadGlyphCount) +
	            2);
	uint16_t substCount = read_16u(
	    data + offset + 6 + 2 * (backtrackGlyphCount + inputGlyphCount - (minusOne ? 1 : 0) + lookaheadGlyphCount));
	checkLength(offset + 8 + 2 * (backtrackGlyphCount + inputGlyphCount - (minusOne ? 1 : 0) + lookaheadGlyphCount) +
	            substCount * 4);

	rule->matchCount = backtrackGlyphCount + inputGlyphCount + lookaheadGlyphCount;
	rule->inputBegins = backtrackGlyphCount;
	rule->inputEnds = backtrackGlyphCount + inputGlyphCount;

	NEW_N(rule->match, rule->matchCount);
	uint16_t jj = 0;
	for (uint16_t j = 0; j < backtrackGlyphCount; j++) {
		uint32_t gid = read_16u(data + offset + 2 + j * 2);
		rule->match[jj++] = fn(data, tableLength, gid, offset, 1, userdata);
	}
	if (minusOne) { rule->match[jj++] = fn(data, tableLength, startGID, offset, 2, userdata); }
	for (uint16_t j = 0; j < inputGlyphCount - (minusOne ? 1 : 0); j++) {
		uint32_t gid = read_16u(data + offset + 4 + 2 * rule->inputBegins + j * 2);
		rule->match[jj++] = fn(data, tableLength, gid, offset, 2, userdata);
	}
	for (uint16_t j = 0; j < lookaheadGlyphCount; j++) {
		uint32_t gid = read_16u(data + offset + 6 + 2 * (rule->inputEnds - (minusOne ? 1 : 0)) + j * 2);
		rule->match[jj++] = fn(data, tableLength, gid, offset, 3, userdata);
	}
	rule->applyCount = substCount;
	NEW_N(rule->apply, rule->applyCount);
	for (uint16_t j = 0; j < substCount; j++) {
		rule->apply[j].index =
		    rule->inputBegins + read_16u(data + offset + 8 + 2 * (rule->matchCount - (minusOne ? 1 : 0)) + j * 4);
		rule->apply[j].lookupIndex =
		    read_16u(data + offset + 8 + 2 * (rule->matchCount - (minusOne ? 1 : 0)) + j * 4 + 2);
		rule->apply[j].lookupName = NULL;
	}
	reverseBacktracks(rule);
	return rule;

FAIL:
	DELETE(deleteRule, rule);
	return NULL;
}

otl_subtable *caryll_read_gsub_chaining(font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	uint16_t format = 0;
	otl_subtable *_subtable;
	NEW(_subtable);
	subtable_gsub_chaining *subtable = &(_subtable->gsub_chaining);
	subtable->rulesCount = 0;
	subtable->rules = NULL;
	checkLength(offset + 2);
	format = read_16u(data + offset);
	if (format == 1) {
		checkLength(offset + 6);
		otl_coverage *firstCoverage = caryll_read_coverage(data, tableLength, offset + read_16u(data + offset + 2));
		uint16_t chainSubRuleSetCount = read_16u(data + offset + 4);
		if (chainSubRuleSetCount != firstCoverage->numGlyphs) goto FAIL;
		checkLength(offset + 6 + 2 * chainSubRuleSetCount);
		uint16_t totalRules = 0;
		for (uint16_t j = 0; j < chainSubRuleSetCount; j++) {
			uint32_t srsOffset = offset + read_16u(data + offset + 6 + j * 2);
			checkLength(srsOffset + 2);
			totalRules += read_16u(data + srsOffset);
			checkLength(srsOffset + 2 + 2 * read_16u(data + srsOffset));
		}
		subtable->rulesCount = totalRules;
		NEW_N(subtable->rules, totalRules);
		uint16_t jj = 0;
		for (uint16_t j = 0; j < chainSubRuleSetCount; j++) {
			uint32_t srsOffset = offset + read_16u(data + offset + 6 + j * 2);
			uint16_t srsCount = read_16u(data + srsOffset);
			for (uint16_t k = 0; k < srsCount; k++) {
				uint32_t srOffset = srsOffset + read_16u(data + srsOffset + 2 + k * 2);
				subtable->rules[jj] = GeneralReadRule(data, tableLength, srOffset, firstCoverage->glyphs[j].gid, true,
				                                      singleCoverage, NULL);
				jj += 1;
			}
		}
		caryll_delete_coverage(firstCoverage);
		return _subtable;
	} else if (format == 2) {
		checkLength(offset + 12);
		classdefs *cds;
		NEW(cds);
		cds->bc = caryll_raad_classdef(data, tableLength, offset + read_16u(data + offset + 4));
		cds->ic = caryll_raad_classdef(data, tableLength, offset + read_16u(data + offset + 6));
		cds->fc = caryll_raad_classdef(data, tableLength, offset + read_16u(data + offset + 8));
		uint16_t chainSubClassSetCnt = read_16u(data + offset + 10);
		checkLength(offset + 12 + 2 * chainSubClassSetCnt);
		uint16_t totalRules = 0;
		for (uint16_t j = 0; j < chainSubClassSetCnt; j++) {
			uint16_t srcOffset = read_16u(data + offset + 12 + j * 2);
			if (srcOffset) { totalRules += read_16u(data + offset + srcOffset); }
		}
		subtable->rulesCount = totalRules;
		NEW_N(subtable->rules, totalRules);
		uint16_t jj = 0;
		for (uint16_t j = 0; j < chainSubClassSetCnt; j++) {
			uint16_t srcOffset = read_16u(data + offset + 12 + j * 2);
			if (srcOffset) {
				uint16_t srsCount = read_16u(data + offset + srcOffset);
				for (uint16_t k = 0; k < srsCount; k++) {
					uint32_t srOffset = offset + srcOffset + read_16u(data + offset + srcOffset + 2 + k * 2);
					subtable->rules[jj] = GeneralReadRule(data, tableLength, srOffset, j, true, classCoverage, cds);
					jj += 1;
				}
			}
		}
		return _subtable;
	} else if (format == 3) {
		// Chaining Contextual Substitution Subtable, Coverage based.
		// This table has exactly one rule within it, and i love it.
		subtable->rulesCount = 1;
		NEW_N(subtable->rules, 1);
		subtable->rules[0] = GeneralReadRule(data, tableLength, offset + 2, 0, false, format3Coverage, NULL);
		return _subtable;
	}
FAIL:
	fprintf(stderr, "Unsupported format %d.\n", format);
	DELETE(delete_otl_chaining_subtable, _subtable);
	return NULL;
}
void caryll_gsub_chaining_to_json(otl_lookup *lookup, json_value *dump) {
	json_object_push(dump, "type", json_string_new("gsub_chaining"));
	json_value *subtables = json_array_new(lookup->subtableCount);
	for (uint16_t j = 0; j < lookup->subtableCount; j++)
		if (lookup->subtables[j]) {
			subtable_gsub_chaining *subtable = &(lookup->subtables[j]->gsub_chaining);
			otl_chaining_rule *rule = subtable->rules[0];
			json_value *_st = json_object_new(4);
			json_value *_match = json_array_new(rule->matchCount);
			for (uint16_t j = 0; j < rule->matchCount; j++) {
				json_array_push(_match, caryll_coverage_to_json(rule->match[j]));
			}
			json_value *_apply = json_array_new(rule->applyCount);
			for (uint16_t j = 0; j < rule->applyCount; j++) {
				json_value *_application = json_object_new(2);
				json_object_push(_application, "at", json_integer_new(rule->apply[j].index));
				json_object_push(_application, "lookup", json_string_new(rule->apply[j].lookupName));
				json_array_push(_apply, _application);
			}
			json_object_push(_st, "match", _match);
			json_object_push(_st, "apply", preserialize(_apply));
			json_object_push(_st, "inputBegins", json_integer_new(rule->inputBegins));
			json_object_push(_st, "inputEnds", json_integer_new(rule->inputEnds));
			json_array_push(subtables, _st);
		}
	json_object_push(dump, "subtables", subtables);
}
