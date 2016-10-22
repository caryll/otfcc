#include "chaining.h"
static void deleteRule(otl_ChainingRule *rule) {
	if (!rule) return;
	if (rule && rule->match && rule->matchCount) {
		for (tableid_t k = 0; k < rule->matchCount; k++) {
			otl_delete_Coverage(rule->match[k]);
		}
		FREE(rule->match);
	}
	if (rule && rule->apply) {
		for (tableid_t j = 0; j < rule->applyCount; j++) {
			handle_delete(&rule->apply[j].lookup);
		}
		FREE(rule->apply);
	}
	FREE(rule);
}
void otl_delete_chaining(otl_Subtable *_subtable) {
	if (_subtable) {
		subtable_chaining *subtable = &(_subtable->chaining);
		if (subtable->rules) {
			for (tableid_t j = 0; j < subtable->rulesCount; j++) {
				deleteRule(subtable->rules[j]);
			}
			free(subtable->rules);
		}
		if (subtable->bc) { otl_delete_ClassDef(subtable->bc); }
		if (subtable->ic) { otl_delete_ClassDef(subtable->ic); }
		if (subtable->fc) { otl_delete_ClassDef(subtable->fc); }
		free(_subtable);
	}
}

static void reverseBacktracks(otl_ChainingRule *rule) {
	if (rule->inputBegins > 0) {
		tableid_t start = 0;
		tableid_t end = rule->inputBegins - 1;
		while (end > start) {
			otl_Coverage *tmp = rule->match[start];
			rule->match[start] = rule->match[end];
			rule->match[end] = tmp;
			end--, start++;
		}
	}
}

typedef struct {
	otl_ClassDef *bc;
	otl_ClassDef *ic;
	otl_ClassDef *fc;
} classdefs;

otl_Coverage *singleCoverage(font_file_pointer data, uint32_t tableLength, uint16_t gid, uint32_t _offset,
                             uint16_t kind, void *userdata) {
	otl_Coverage *cov;
	NEW(cov);
	cov->numGlyphs = 1;
	NEW(cov->glyphs);
	cov->glyphs[0] = handle_fromIndex((glyphid_t)gid);
	return cov;
}
otl_Coverage *classCoverage(font_file_pointer data, uint32_t tableLength, uint16_t cls, uint32_t _offset, uint16_t kind,
                            void *_classdefs) {
	classdefs *defs = (classdefs *)_classdefs;
	otl_ClassDef *cd = (kind == 1 ? defs->bc : kind == 2 ? defs->ic : defs->fc);
	otl_Coverage *cov;
	NEW(cov);
	cov->numGlyphs = 0;
	cov->glyphs = NULL;
	glyphid_t count = 0;
	for (glyphid_t j = 0; j < cd->numGlyphs; j++) {
		if (cd->classes[j] == cls) count++;
	}
	if (!count) return cov;
	cov->numGlyphs = count;
	NEW_N(cov->glyphs, count);
	glyphid_t jj = 0;
	for (glyphid_t j = 0; j < cd->numGlyphs; j++) {
		if (cd->classes[j] == cls) { cov->glyphs[jj++] = cd->glyphs[j]; }
	}
	return cov;
}
otl_Coverage *format3Coverage(font_file_pointer data, uint32_t tableLength, uint16_t shift, uint32_t _offset,
                              uint16_t kind, void *userdata) {
	return otl_read_Coverage(data, tableLength, _offset + shift - 2);
}

typedef otl_Coverage *(*CoverageReaderHandler)(font_file_pointer, uint32_t, uint16_t, uint32_t, uint16_t, void *);

otl_ChainingRule *GeneralReadContextualRule(font_file_pointer data, uint32_t tableLength, uint32_t offset,
                                            uint16_t startGID, bool minusOne, CoverageReaderHandler fn,
                                            void *userdata) {
	otl_ChainingRule *rule;
	NEW(rule);
	rule->match = NULL;
	rule->apply = NULL;

	uint16_t minusOneQ = (minusOne ? 1 : 0);

	checkLength(offset + 4);
	uint16_t nInput = read_16u(data + offset);
	uint16_t nApply = read_16u(data + offset + 2);
	checkLength(offset + 4 + 2 * nInput + 4 * nApply);

	rule->matchCount = nInput;
	rule->inputBegins = 0;
	rule->inputEnds = nInput;

	NEW_N(rule->match, rule->matchCount);
	uint16_t jj = 0;
	if (minusOne) { rule->match[jj++] = fn(data, tableLength, startGID, offset, 2, userdata); }
	for (uint16_t j = 0; j < nInput - minusOneQ; j++) {
		uint32_t gid = read_16u(data + offset + 4 + j * 2);
		rule->match[jj++] = fn(data, tableLength, gid, offset, 2, userdata);
	}
	rule->applyCount = nApply;
	NEW_N(rule->apply, rule->applyCount);
	for (tableid_t j = 0; j < nApply; j++) {
		rule->apply[j].index =
		    rule->inputBegins + read_16u(data + offset + 4 + 2 * (rule->matchCount - minusOneQ) + j * 4);
		rule->apply[j].lookup =
		    handle_fromIndex(read_16u(data + offset + 4 + 2 * (rule->matchCount - minusOneQ) + j * 4 + 2));
	}
	reverseBacktracks(rule);
	return rule;

FAIL:
	DELETE(deleteRule, rule);
	return NULL;
}

otl_Subtable *otl_read_contextual(const font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	uint16_t format = 0;
	otl_Subtable *_subtable;
	NEW(_subtable);
	subtable_chaining *subtable = &(_subtable->chaining);
	subtable->rulesCount = 0;
	subtable->classified = false;
	subtable->bc = NULL;
	subtable->ic = NULL;
	subtable->fc = NULL;
	subtable->rules = NULL;
	checkLength(offset + 2);
	format = read_16u(data + offset);
	if (format == 1) {
		// Contextual Substitution Subtable, Simple.
		checkLength(offset + 6);

		uint16_t covOffset = offset + read_16u(data + offset + 2);
		otl_Coverage *firstCoverage = otl_read_Coverage(data, tableLength, covOffset);

		tableid_t chainSubRuleSetCount = read_16u(data + offset + 4);
		if (chainSubRuleSetCount != firstCoverage->numGlyphs) goto FAIL;
		checkLength(offset + 6 + 2 * chainSubRuleSetCount);

		tableid_t totalRules = 0;
		for (tableid_t j = 0; j < chainSubRuleSetCount; j++) {
			uint32_t srsOffset = offset + read_16u(data + offset + 6 + j * 2);
			checkLength(srsOffset + 2);
			totalRules += read_16u(data + srsOffset);
			checkLength(srsOffset + 2 + 2 * read_16u(data + srsOffset));
		}
		subtable->rulesCount = totalRules;
		NEW_N(subtable->rules, totalRules);

		tableid_t jj = 0;
		for (tableid_t j = 0; j < chainSubRuleSetCount; j++) {
			uint32_t srsOffset = offset + read_16u(data + offset + 6 + j * 2);
			tableid_t srsCount = read_16u(data + srsOffset);
			for (tableid_t k = 0; k < srsCount; k++) {
				uint32_t srOffset = srsOffset + read_16u(data + srsOffset + 2 + k * 2);
				subtable->rules[jj] = GeneralReadContextualRule(
				    data, tableLength, srOffset, firstCoverage->glyphs[j].index, true, singleCoverage, NULL);
				jj += 1;
			}
		}

		otl_delete_Coverage(firstCoverage);
		return _subtable;
	} else if (format == 2) {
		// Contextual Substitution Subtable, Class based.
		checkLength(offset + 8);

		classdefs *cds;
		NEW(cds);
		cds->bc = NULL;
		cds->ic = otl_read_ClassDef(data, tableLength, offset + read_16u(data + offset + 4));
		cds->fc = NULL;

		tableid_t chainSubClassSetCnt = read_16u(data + offset + 6);
		checkLength(offset + 12 + 2 * chainSubClassSetCnt);

		tableid_t totalRules = 0;
		for (tableid_t j = 0; j < chainSubClassSetCnt; j++) {
			uint32_t srcOffset = read_16u(data + offset + 8 + j * 2);
			if (srcOffset) { totalRules += read_16u(data + offset + srcOffset); }
		}
		subtable->rulesCount = totalRules;
		NEW_N(subtable->rules, totalRules);

		tableid_t jj = 0;
		for (tableid_t j = 0; j < chainSubClassSetCnt; j++) {
			uint32_t srcOffset = read_16u(data + offset + 8 + j * 2);
			if (srcOffset) {
				tableid_t srsCount = read_16u(data + offset + srcOffset);
				for (tableid_t k = 0; k < srsCount; k++) {
					uint32_t srOffset = offset + srcOffset + read_16u(data + offset + srcOffset + 2 + k * 2);
					subtable->rules[jj] =
					    GeneralReadContextualRule(data, tableLength, srOffset, j, true, classCoverage, cds);
					jj += 1;
				}
			}
		}

		if (cds && cds->ic && cds->ic->glyphs) free(cds->ic->glyphs);
		if (cds && cds->ic && cds->ic->classes) free(cds->ic->classes);
		if (cds && cds->ic) free(cds->ic);
		return _subtable;
	} else if (format == 3) {
		// Contextual Substitution Subtable, Coverage based.
		subtable->rulesCount = 1;
		NEW_N(subtable->rules, 1);
		subtable->rules[0] = GeneralReadContextualRule(data, tableLength, offset + 2, 0, false, format3Coverage, NULL);
		return _subtable;
	}
FAIL:
	fprintf(stderr, "Unsupported format %d.\n", format);
	DELETE(otl_delete_chaining, _subtable);
	return NULL;
}

otl_ChainingRule *GeneralReadChainingRule(font_file_pointer data, uint32_t tableLength, uint32_t offset,
                                          uint16_t startGID, bool minusOne, CoverageReaderHandler fn, void *userdata) {
	otl_ChainingRule *rule;
	NEW(rule);
	rule->match = NULL;
	rule->apply = NULL;

	uint16_t minusOneQ = (minusOne ? 1 : 0);

	checkLength(offset + 8);
	tableid_t nBack = read_16u(data + offset);
	checkLength(offset + 2 + 2 * nBack + 2);
	tableid_t nInput = read_16u(data + offset + 2 + 2 * nBack);
	checkLength(offset + 4 + 2 * (nBack + nInput - minusOneQ) + 2);
	tableid_t nLookaround = read_16u(data + offset + 4 + 2 * (nBack + nInput - minusOneQ));
	checkLength(offset + 6 + 2 * (nBack + nInput - minusOneQ + nLookaround) + 2);
	tableid_t nApply = read_16u(data + offset + 6 + 2 * (nBack + nInput - minusOneQ + nLookaround));
	checkLength(offset + 8 + 2 * (nBack + nInput - minusOneQ + nLookaround) + nApply * 4);

	rule->matchCount = nBack + nInput + nLookaround;
	rule->inputBegins = nBack;
	rule->inputEnds = nBack + nInput;

	NEW_N(rule->match, rule->matchCount);
	tableid_t jj = 0;
	for (tableid_t j = 0; j < nBack; j++) {
		uint32_t gid = read_16u(data + offset + 2 + j * 2);
		rule->match[jj++] = fn(data, tableLength, gid, offset, 1, userdata);
	}
	if (minusOne) { rule->match[jj++] = fn(data, tableLength, startGID, offset, 2, userdata); }
	for (tableid_t j = 0; j < nInput - minusOneQ; j++) {
		uint32_t gid = read_16u(data + offset + 4 + 2 * rule->inputBegins + j * 2);
		rule->match[jj++] = fn(data, tableLength, gid, offset, 2, userdata);
	}
	for (tableid_t j = 0; j < nLookaround; j++) {
		uint32_t gid = read_16u(data + offset + 6 + 2 * (rule->inputEnds - minusOneQ) + j * 2);
		rule->match[jj++] = fn(data, tableLength, gid, offset, 3, userdata);
	}
	rule->applyCount = nApply;
	NEW_N(rule->apply, rule->applyCount);
	for (tableid_t j = 0; j < nApply; j++) {
		rule->apply[j].index =
		    rule->inputBegins + read_16u(data + offset + 8 + 2 * (rule->matchCount - minusOneQ) + j * 4);
		rule->apply[j].lookup =
		    handle_fromIndex(read_16u(data + offset + 8 + 2 * (rule->matchCount - minusOneQ) + j * 4 + 2));
	}
	reverseBacktracks(rule);
	return rule;

FAIL:
	DELETE(deleteRule, rule);
	return NULL;
}

otl_Subtable *otl_read_chaining(const font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	uint16_t format = 0;
	otl_Subtable *_subtable;
	NEW(_subtable);
	subtable_chaining *subtable = &(_subtable->chaining);
	subtable->rulesCount = 0;
	subtable->classified = false;
	subtable->bc = NULL;
	subtable->ic = NULL;
	subtable->fc = NULL;
	subtable->rules = NULL;

	checkLength(offset + 2);
	format = read_16u(data + offset);
	if (format == 1) {
		// Contextual Substitution Subtable, Simple.
		checkLength(offset + 6);

		uint16_t covOffset = offset + read_16u(data + offset + 2);
		otl_Coverage *firstCoverage = otl_read_Coverage(data, tableLength, covOffset);

		tableid_t chainSubRuleSetCount = read_16u(data + offset + 4);
		if (chainSubRuleSetCount != firstCoverage->numGlyphs) goto FAIL;
		checkLength(offset + 6 + 2 * chainSubRuleSetCount);

		tableid_t totalRules = 0;
		for (tableid_t j = 0; j < chainSubRuleSetCount; j++) {
			uint32_t srsOffset = offset + read_16u(data + offset + 6 + j * 2);
			checkLength(srsOffset + 2);
			totalRules += read_16u(data + srsOffset);
			checkLength(srsOffset + 2 + 2 * read_16u(data + srsOffset));
		}
		subtable->rulesCount = totalRules;
		NEW_N(subtable->rules, totalRules);

		tableid_t jj = 0;
		for (tableid_t j = 0; j < chainSubRuleSetCount; j++) {
			uint32_t srsOffset = offset + read_16u(data + offset + 6 + j * 2);
			tableid_t srsCount = read_16u(data + srsOffset);
			for (tableid_t k = 0; k < srsCount; k++) {
				uint32_t srOffset = srsOffset + read_16u(data + srsOffset + 2 + k * 2);
				subtable->rules[jj] = GeneralReadChainingRule(
				    data, tableLength, srOffset, firstCoverage->glyphs[j].index, true, singleCoverage, NULL);
				jj += 1;
			}
		}

		otl_delete_Coverage(firstCoverage);
		return _subtable;
	} else if (format == 2) {
		// Chaining Contextual Substitution Subtable, Class based.
		checkLength(offset + 12);

		classdefs *cds;
		NEW(cds);
		cds->bc = otl_read_ClassDef(data, tableLength, offset + read_16u(data + offset + 4));
		cds->ic = otl_read_ClassDef(data, tableLength, offset + read_16u(data + offset + 6));
		cds->fc = otl_read_ClassDef(data, tableLength, offset + read_16u(data + offset + 8));

		tableid_t chainSubClassSetCnt = read_16u(data + offset + 10);
		checkLength(offset + 12 + 2 * chainSubClassSetCnt);

		tableid_t totalRules = 0;
		for (tableid_t j = 0; j < chainSubClassSetCnt; j++) {
			uint32_t srcOffset = read_16u(data + offset + 12 + j * 2);
			if (srcOffset) { totalRules += read_16u(data + offset + srcOffset); }
		}
		subtable->rulesCount = totalRules;
		NEW_N(subtable->rules, totalRules);

		tableid_t jj = 0;
		for (tableid_t j = 0; j < chainSubClassSetCnt; j++) {
			uint32_t srcOffset = read_16u(data + offset + 12 + j * 2);
			if (srcOffset) {
				tableid_t srsCount = read_16u(data + offset + srcOffset);
				for (tableid_t k = 0; k < srsCount; k++) {
					uint32_t dsrOffset = read_16u(data + offset + srcOffset + 2 + k * 2);
					uint32_t srOffset = offset + srcOffset + dsrOffset;
					subtable->rules[jj] =
					    GeneralReadChainingRule(data, tableLength, srOffset, j, true, classCoverage, cds);
					jj += 1;
				}
			}
		}

		if (cds) {
			if (cds->bc) otl_delete_ClassDef(cds->bc);
			if (cds->ic) otl_delete_ClassDef(cds->ic);
			if (cds->fc) otl_delete_ClassDef(cds->fc);
		}
		return _subtable;
	} else if (format == 3) {
		// Chaining Contextual Substitution Subtable, Coverage based.
		// This table has exactly one rule within it, and i love it.
		subtable->rulesCount = 1;
		NEW_N(subtable->rules, 1);
		subtable->rules[0] = GeneralReadChainingRule(data, tableLength, offset + 2, 0, false, format3Coverage, NULL);
		return _subtable;
	}
FAIL:
	fprintf(stderr, "Unsupported format %d.\n", format);
	DELETE(otl_delete_chaining, _subtable);
	return NULL;
}

json_value *otl_dump_chaining(const otl_Subtable *_subtable) {
	const subtable_chaining *subtable = &(_subtable->chaining);
	otl_ChainingRule *rule = subtable->rules[0];
	json_value *_st = json_object_new(4);

	json_value *_match = json_array_new(rule->matchCount);
	for (tableid_t j = 0; j < rule->matchCount; j++) {
		json_array_push(_match, otl_dump_Coverage(rule->match[j]));
	}
	json_object_push(_st, "match", _match);

	json_value *_apply = json_array_new(rule->applyCount);
	for (tableid_t j = 0; j < rule->applyCount; j++) {
		json_value *_application = json_object_new(2);
		json_object_push(_application, "at", json_integer_new(rule->apply[j].index));
		json_object_push(_application, "lookup", json_string_new(rule->apply[j].lookup.name));
		json_array_push(_apply, _application);
	}
	json_object_push(_st, "apply", preserialize(_apply));

	json_object_push(_st, "inputBegins", json_integer_new(rule->inputBegins));
	json_object_push(_st, "inputEnds", json_integer_new(rule->inputEnds));
	return _st;
}

otl_Subtable *otl_parse_chaining(const json_value *_subtable) {
	json_value *_match = json_obj_get_type(_subtable, "match", json_array);
	json_value *_apply = json_obj_get_type(_subtable, "apply", json_array);
	if (!_match || !_apply) return NULL;

	otl_Subtable *_st;
	NEW(_st);
	subtable_chaining *subtable = &(_st->chaining);
	subtable->rulesCount = 1;
	subtable->classified = false;
	subtable->bc = NULL;
	subtable->ic = NULL;
	subtable->fc = NULL;

	NEW(subtable->rules);
	NEW(subtable->rules[0]);
	otl_ChainingRule *rule = subtable->rules[0];

	rule->matchCount = _match->u.array.length;
	NEW_N(rule->match, rule->matchCount);
	rule->applyCount = _apply->u.array.length;
	NEW_N(rule->apply, rule->applyCount);

	rule->inputBegins = json_obj_getnum_fallback(_subtable, "inputBegins", 0);
	rule->inputEnds = json_obj_getnum_fallback(_subtable, "inputEnds", rule->matchCount);

	for (tableid_t j = 0; j < rule->matchCount; j++) {
		rule->match[j] = otl_parse_Coverage(_match->u.array.values[j]);
	}
	for (tableid_t j = 0; j < rule->applyCount; j++) {
		rule->apply[j].index = 0;
		rule->apply[j].lookup = handle_new();
		json_value *_application = _apply->u.array.values[j];
		if (_application->type == json_object) {
			json_value *_ln = json_obj_get_type(_application, "lookup", json_string);
			if (_ln) {
				rule->apply[j].lookup = handle_fromName(sdsnewlen(_ln->u.string.ptr, _ln->u.string.length));
				rule->apply[j].index = json_obj_getnum(_application, "at");
			}
		}
	}
	return _st;
}

caryll_buffer *caryll_build_chaining_coverage(const otl_Subtable *_subtable) {
	const subtable_chaining *subtable = &(_subtable->chaining);
	otl_ChainingRule *rule = subtable->rules[0];
	tableid_t nBacktrack = rule->inputBegins;
	tableid_t nInput = rule->inputEnds - rule->inputBegins;
	tableid_t nLookahead = rule->matchCount - rule->inputEnds;
	tableid_t nSubst = rule->applyCount;
	reverseBacktracks(rule);

	bk_Block *root = bk_new_Block(b16, 3, // format
	                              bkover);

	bk_push(root, b16, nBacktrack, bkover);
	for (tableid_t j = 0; j < rule->inputBegins; j++) {
		bk_push(root, p16, bk_newBlockFromBuffer(otl_build_Coverage(rule->match[j])), bkover);
	}
	bk_push(root, b16, nInput, bkover);
	for (tableid_t j = rule->inputBegins; j < rule->inputEnds; j++) {
		bk_push(root, p16, bk_newBlockFromBuffer(otl_build_Coverage(rule->match[j])), bkover);
	}
	bk_push(root, b16, nLookahead, bkover);
	for (tableid_t j = rule->inputEnds; j < rule->matchCount; j++) {
		bk_push(root, p16, bk_newBlockFromBuffer(otl_build_Coverage(rule->match[j])), bkover);
	}
	bk_push(root, b16, rule->applyCount, bkover);
	for (tableid_t j = 0; j < nSubst; j++) {
		bk_push(root, b16, rule->apply[j].index - nBacktrack, // position
		        b16, rule->apply[j].lookup.index,             // lookup
		        bkover);
	}

	return bk_build_Block(root);
}

caryll_buffer *caryll_build_chaining_classes(const otl_Subtable *_subtable) {
	const subtable_chaining *subtable = &(_subtable->chaining);

	otl_Coverage *coverage;
	NEW(coverage);
	coverage->numGlyphs = subtable->ic->numGlyphs;
	coverage->glyphs = subtable->ic->glyphs;

	bk_Block *root = bk_new_Block(b16, 2,                                                       // format
	                              p16, bk_newBlockFromBuffer(otl_build_Coverage(coverage)),     // coverage
	                              p16, bk_newBlockFromBuffer(otl_build_ClassDef(subtable->bc)), // BacktrackClassDef
	                              p16, bk_newBlockFromBuffer(otl_build_ClassDef(subtable->ic)), // InputClassDef
	                              p16, bk_newBlockFromBuffer(otl_build_ClassDef(subtable->fc)), // LookaheadClassDef
	                              b16, subtable->ic->maxclass + 1,                              // ChainSubClassSetCnt
	                              bkover);

	glyphclass_t *rcpg;
	NEW_N(rcpg, subtable->ic->maxclass + 1);
	for (glyphclass_t j = 0; j <= subtable->ic->maxclass; j++) {
		rcpg[j] = 0;
	}
	for (tableid_t j = 0; j < subtable->rulesCount; j++) {
		tableid_t ib = subtable->rules[j]->inputBegins;
		tableid_t startClass = subtable->rules[j]->match[ib]->glyphs[0].index;
		if (startClass <= subtable->ic->maxclass) rcpg[startClass] += 1;
	}

	for (glyphclass_t j = 0; j <= subtable->ic->maxclass; j++) {
		if (rcpg[j]) {
			bk_Block *cset = bk_new_Block(b16, rcpg[j], // ChainSubClassRuleCnt
			                              bkover);
			for (tableid_t k = 0; k < subtable->rulesCount; k++) {
				otl_ChainingRule *rule = subtable->rules[k];
				glyphclass_t startClass = rule->match[rule->inputBegins]->glyphs[0].index;
				if (startClass != j) { continue; }
				reverseBacktracks(rule);
				tableid_t nBacktrack = rule->inputBegins;
				tableid_t nInput = rule->inputEnds - rule->inputBegins;
				tableid_t nLookahead = rule->matchCount - rule->inputEnds;
				tableid_t nSubst = rule->applyCount;
				bk_Block *r = bk_new_Block(bkover);
				bk_push(r, b16, nBacktrack, bkover);
				for (tableid_t m = 0; m < rule->inputBegins; m++) {
					bk_push(r, b16, rule->match[m]->glyphs[0].index, bkover);
				}
				bk_push(r, b16, nInput, bkover);
				for (tableid_t m = rule->inputBegins + 1; m < rule->inputEnds; m++) {
					bk_push(r, b16, rule->match[m]->glyphs[0].index, bkover);
				}
				bk_push(r, b16, nLookahead, bkover);
				for (tableid_t m = rule->inputEnds; m < rule->matchCount; m++) {
					bk_push(r, b16, rule->match[m]->glyphs[0].index, bkover);
				}
				bk_push(r, b16, nSubst, bkover);
				for (tableid_t m = 0; m < nSubst; m++) {
					bk_push(r, b16, rule->apply[m].index - nBacktrack, // position
					        b16, rule->apply[m].lookup.index,          // lookup index
					        bkover);
				}
				bk_push(cset, p16, r, bkover);
			}
			bk_push(root, p16, cset, bkover);
		} else {
			bk_push(root, p16, NULL, bkover);
		}
	}

	free(coverage);
	free(rcpg);
	return bk_build_Block(root);
}

caryll_buffer *caryll_build_chaining(const otl_Subtable *_subtable) {
	if (_subtable->chaining.classified) {
		return caryll_build_chaining_classes(_subtable);
	} else {
		return caryll_build_chaining_coverage(_subtable);
	}
}
