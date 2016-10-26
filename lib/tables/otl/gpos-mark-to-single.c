#include "gpos-mark-to-single.h"
#include "gpos-common.h"

void otl_delete_gpos_markToSingle(otl_Subtable *_subtable) {
	if (_subtable) {
		subtable_gpos_markToSingle *subtable = &(_subtable->gpos_markToSingle);
		if (subtable->marks) { otl_delete_Coverage(subtable->marks); }
		if (subtable->markArray) { otl_delete_mark_array(subtable->markArray); }
		if (subtable->bases) {
			if (subtable->baseArray) {
				for (glyphid_t j = 0; j < subtable->bases->numGlyphs; j++) {
					if (subtable->baseArray[j]) free(subtable->baseArray[j]);
				}
				free(subtable->baseArray);
			}
			otl_delete_Coverage(subtable->bases);
		}
		free(_subtable);
	}
}

otl_Subtable *otl_read_gpos_markToSingle(const font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset) {
	otl_Subtable *_subtable;
	NEW(_subtable);
	subtable_gpos_markToSingle *subtable = &(_subtable->gpos_markToSingle);
	if (tableLength < subtableOffset + 12) goto FAIL;
	subtable->marks = otl_read_Coverage(data, tableLength, subtableOffset + read_16u(data + subtableOffset + 2));
	subtable->bases = otl_read_Coverage(data, tableLength, subtableOffset + read_16u(data + subtableOffset + 4));
	if (!subtable->marks || subtable->marks->numGlyphs == 0 || !subtable->bases || subtable->bases->numGlyphs == 0)
		goto FAIL;
	subtable->classCount = read_16u(data + subtableOffset + 6);
	uint32_t markArrayOffset = subtableOffset + read_16u(data + subtableOffset + 8);
	subtable->markArray = otl_read_mark_array(data, tableLength, markArrayOffset);
	if (!subtable->markArray || subtable->markArray->markCount != subtable->marks->numGlyphs) goto FAIL;

	uint32_t baseArrayOffset = subtableOffset + read_16u(data + subtableOffset + 10);
	checkLength(baseArrayOffset + 2 + 2 * subtable->bases->numGlyphs * subtable->classCount);
	if (read_16u(data + baseArrayOffset) != subtable->bases->numGlyphs) goto FAIL;
	NEW_N(subtable->baseArray, subtable->bases->numGlyphs);
	uint32_t _offset = baseArrayOffset + 2;
	for (glyphid_t j = 0; j < subtable->bases->numGlyphs; j++) {
		NEW_N(subtable->baseArray[j], subtable->classCount);
		for (glyphclass_t k = 0; k < subtable->classCount; k++) {
			if (read_16u(data + _offset)) {
				subtable->baseArray[j][k] =
				    otl_read_anchor(data, tableLength, baseArrayOffset + read_16u(data + _offset));
			} else {
				subtable->baseArray[j][k].present = false;
				subtable->baseArray[j][k].x = 0;
				subtable->baseArray[j][k].y = 0;
			}
			_offset += 2;
		}
	}
	goto OK;
FAIL:
	DELETE(otl_delete_gpos_markToSingle, _subtable);
OK:
	return _subtable;
}

json_value *otl_gpos_dump_markToSingle(const otl_Subtable *st) {
	const subtable_gpos_markToSingle *subtable = &(st->gpos_markToSingle);
	json_value *_subtable = json_object_new(3);
	json_value *_marks = json_object_new(subtable->marks->numGlyphs);
	json_value *_bases = json_object_new(subtable->bases->numGlyphs);
	for (glyphid_t j = 0; j < subtable->marks->numGlyphs; j++) {
		json_value *_mark = json_object_new(3);
		sds markClassName = sdscatfmt(sdsempty(), "anchor%i", subtable->markArray->records[j].markClass);
		json_object_push(_mark, "class", json_string_new_length((uint32_t)sdslen(markClassName), markClassName));
		sdsfree(markClassName);
		json_object_push(_mark, "x", json_integer_new(subtable->markArray->records[j].anchor.x));
		json_object_push(_mark, "y", json_integer_new(subtable->markArray->records[j].anchor.y));
		json_object_push(_marks, subtable->marks->glyphs[j].name, preserialize(_mark));
	}
	for (glyphid_t j = 0; j < subtable->bases->numGlyphs; j++) {
		json_value *_base = json_object_new(subtable->classCount);
		for (glyphclass_t k = 0; k < subtable->classCount; k++) {
			if (subtable->baseArray[j][k].present) {
				json_value *_anchor = json_object_new(2);
				json_object_push(_anchor, "x", json_integer_new(subtable->baseArray[j][k].x));
				json_object_push(_anchor, "y", json_integer_new(subtable->baseArray[j][k].y));
				sds markClassName = sdscatfmt(sdsempty(), "anchor%i", k);
				json_object_push_length(_base, (uint32_t)sdslen(markClassName), markClassName, _anchor);
				sdsfree(markClassName);
			}
		}
		json_object_push(_bases, subtable->bases->glyphs[j].name, preserialize(_base));
	}
	json_object_push(_subtable, "marks", _marks);
	json_object_push(_subtable, "bases", _bases);
	return _subtable;
}

typedef struct {
	sds className;
	glyphclass_t classID;
	UT_hash_handle hh;
} classname_hash;
static int compare_classHash(classname_hash *a, classname_hash *b) {
	return strcmp(a->className, b->className);
}
static void parseMarks(json_value *_marks, subtable_gpos_markToSingle *subtable, classname_hash **h) {
	NEW(subtable->marks);
	subtable->marks->numGlyphs = _marks->u.object.length;
	NEW_N(subtable->marks->glyphs, subtable->marks->numGlyphs);
	NEW(subtable->markArray);
	subtable->markArray->markCount = _marks->u.object.length;
	NEW_N(subtable->markArray->records, subtable->markArray->markCount);
	for (glyphid_t j = 0; j < _marks->u.object.length; j++) {
		char *gname = _marks->u.object.values[j].name;
		json_value *anchorRecord = _marks->u.object.values[j].value;
		subtable->marks->glyphs[j] = handle_fromName(sdsnewlen(gname, _marks->u.object.values[j].name_length));

		subtable->markArray->records[j].markClass = 0;
		subtable->markArray->records[j].anchor = otl_anchor_absent();

		if (!anchorRecord || anchorRecord->type != json_object) continue;
		json_value *_className = json_obj_get_type(anchorRecord, "class", json_string);
		if (!_className) continue;

		sds className = sdsnewlen(_className->u.string.ptr, _className->u.string.length);
		classname_hash *s;
		HASH_FIND_STR(*h, className, s);
		if (!s) {
			NEW(s);
			s->className = className;
			s->classID = HASH_COUNT(*h);
			HASH_ADD_STR(*h, className, s);
		} else {
			sdsfree(className);
		}
		subtable->markArray->records[j].markClass = s->classID;
		subtable->markArray->records[j].anchor.present = true;
		subtable->markArray->records[j].anchor.x = json_obj_getnum(anchorRecord, "x");
		subtable->markArray->records[j].anchor.y = json_obj_getnum(anchorRecord, "y");
	}
	HASH_SORT(*h, compare_classHash);
	glyphid_t jAnchorIndex = 0;
	classname_hash *s;
	foreach_hash(s, *h) {
		s->classID = jAnchorIndex;
		jAnchorIndex++;
	}
	for (glyphid_t j = 0; j < _marks->u.object.length; j++) {
		if (!subtable->markArray->records[j].anchor.present) continue;
		json_value *anchorRecord = _marks->u.object.values[j].value;
		json_value *_className = json_obj_get_type(anchorRecord, "class", json_string);
		sds className = sdsnewlen(_className->u.string.ptr, _className->u.string.length);
		classname_hash *s;
		HASH_FIND_STR(*h, className, s);
		if (s) {
			subtable->markArray->records[j].markClass = s->classID;
		} else {
			subtable->markArray->records[j].markClass = 0;
		}
		sdsfree(className);
	}
}
static void parseBases(json_value *_bases, subtable_gpos_markToSingle *subtable, classname_hash **h) {
	glyphclass_t classCount = HASH_COUNT(*h);
	NEW(subtable->bases);
	subtable->bases->numGlyphs = _bases->u.object.length;
	NEW_N(subtable->bases->glyphs, subtable->bases->numGlyphs);
	NEW_N(subtable->baseArray, _bases->u.object.length);
	for (glyphid_t j = 0; j < _bases->u.object.length; j++) {
		char *gname = _bases->u.object.values[j].name;
		subtable->bases->glyphs[j] = handle_fromName(sdsnewlen(gname, _bases->u.object.values[j].name_length));
		NEW_N(subtable->baseArray[j], classCount);
		for (glyphclass_t k = 0; k < classCount; k++) {
			subtable->baseArray[j][k] = otl_anchor_absent();
		}
		json_value *baseRecord = _bases->u.object.values[j].value;
		if (!baseRecord || baseRecord->type != json_object) continue;

		for (glyphclass_t k = 0; k < baseRecord->u.object.length; k++) {
			sds className = sdsnewlen(baseRecord->u.object.values[k].name, baseRecord->u.object.values[k].name_length);
			classname_hash *s;
			HASH_FIND_STR(*h, className, s);
			if (!s) {
				fprintf(stderr, "[OTFCC-fea] Invalid anchor class name <%s> "
				                "for /%s. This base anchor is ignored.\n",
				        className, gname);
				goto NEXT;
			}
			subtable->baseArray[j][s->classID] = otl_parse_anchor(baseRecord->u.object.values[k].value);
		NEXT:
			sdsfree(className);
		}
	}
}
otl_Subtable *otl_gpos_parse_markToSingle(const json_value *_subtable) {
	json_value *_marks = json_obj_get_type(_subtable, "marks", json_object);
	json_value *_bases = json_obj_get_type(_subtable, "bases", json_object);
	if (!_marks || !_bases) return NULL;
	otl_Subtable *st;
	NEW(st);
	classname_hash *h = NULL;
	parseMarks(_marks, &(st->gpos_markToSingle), &h);
	st->gpos_markToSingle.classCount = HASH_COUNT(h);
	parseBases(_bases, &(st->gpos_markToSingle), &h);

	classname_hash *s, *tmp;
	HASH_ITER(hh, h, s, tmp) {
		HASH_DEL(h, s);
		sdsfree(s->className);
		free(s);
	}

	return st;
}

caryll_Buffer *caryll_build_gpos_markToSingle(const otl_Subtable *_subtable) {
	const subtable_gpos_markToSingle *subtable = &(_subtable->gpos_markToSingle);

	bk_Block *root = bk_new_Block(b16, 1,                                                          // format
	                              p16, bk_newBlockFromBuffer(otl_build_Coverage(subtable->marks)), // markCoverage
	                              p16, bk_newBlockFromBuffer(otl_build_Coverage(subtable->bases)), // baseCoverage
	                              b16, subtable->classCount,                                       // classCont
	                              bkover);

	bk_Block *markArray = bk_new_Block(b16, subtable->marks->numGlyphs, // markCount
	                                   bkover);
	for (glyphid_t j = 0; j < subtable->marks->numGlyphs; j++) {
		bk_push(markArray,                                                 // markArray item
		        b16, subtable->markArray->records[j].markClass,            // markClass
		        p16, bkFromAnchor(subtable->markArray->records[j].anchor), // Anchor
		        bkover);
	}

	bk_Block *baseArray = bk_new_Block(b16, subtable->bases->numGlyphs, // baseCount
	                                   bkover);
	for (glyphid_t j = 0; j < subtable->bases->numGlyphs; j++) {
		for (glyphclass_t k = 0; k < subtable->classCount; k++) {
			bk_push(baseArray, p16, bkFromAnchor(subtable->baseArray[j][k]), bkover);
		}
	}

	bk_push(root, p16, markArray, p16, baseArray, bkover);

	return bk_build_Block(root);
}
