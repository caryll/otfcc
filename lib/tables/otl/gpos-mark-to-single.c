#include "gpos-mark-to-single.h"
#include "gpos-common.h"

static void delete_mtb_subtable(otl_subtable *_subtable) {
	if (_subtable) {
		subtable_gpos_mark_to_single *subtable = &(_subtable->gpos_mark_to_single);
		if (subtable->marks) { caryll_delete_coverage(subtable->marks); }
		if (subtable->markArray) { otl_delete_mark_array(subtable->markArray); }
		if (subtable->bases) {
			if (subtable->baseArray) {
				for (uint16_t j = 0; j < subtable->bases->numGlyphs; j++) {
					if (subtable->baseArray[j]) free(subtable->baseArray[j]);
				}
				free(subtable->baseArray);
			}
			caryll_delete_coverage(subtable->bases);
		}
		free(_subtable);
	}
}

void caryll_delete_gpos_mark_to_single(otl_lookup *lookup) {
	if (lookup) {
		if (lookup->subtables) {
			for (uint16_t j = 0; j < lookup->subtableCount; j++)
				delete_mtb_subtable(lookup->subtables[j]);
			free(lookup->subtables);
		}
		free(lookup);
	}
}

otl_subtable *caryll_read_gpos_mark_to_single(font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset) {
	otl_subtable *_subtable;
	NEW(_subtable);
	subtable_gpos_mark_to_single *subtable = &(_subtable->gpos_mark_to_single);
	if (tableLength < subtableOffset + 12) goto FAIL;
	subtable->marks = caryll_read_coverage(data, tableLength, subtableOffset + read_16u(data + subtableOffset + 2));
	subtable->bases = caryll_read_coverage(data, tableLength, subtableOffset + read_16u(data + subtableOffset + 4));
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
	for (uint16_t j = 0; j < subtable->bases->numGlyphs; j++) {
		NEW_N(subtable->baseArray[j], subtable->classCount);
		for (uint16_t k = 0; k < subtable->classCount; k++) {
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
	DELETE(delete_mtb_subtable, _subtable);
OK:
	return _subtable;
}

json_value *caryll_gpos_mark_to_single_to_json(otl_subtable *st) {
	subtable_gpos_mark_to_single *subtable = &(st->gpos_mark_to_single);
	json_value *_subtable = json_object_new(3);
	json_value *_marks = json_object_new(subtable->marks->numGlyphs);
	json_value *_bases = json_object_new(subtable->bases->numGlyphs);
	for (uint16_t j = 0; j < subtable->marks->numGlyphs; j++) {
		json_value *_mark = json_object_new(3);
		sds markClassName = sdscatfmt(sdsempty(), "ac_%i", subtable->markArray->records[j].markClass);
		json_object_push(_mark, "class", json_string_new_length((uint32_t)sdslen(markClassName), markClassName));
		sdsfree(markClassName);
		json_object_push(_mark, "x", json_integer_new(subtable->markArray->records[j].anchor.x));
		json_object_push(_mark, "y", json_integer_new(subtable->markArray->records[j].anchor.y));
		json_object_push(_marks, subtable->marks->glyphs[j].name, preserialize(_mark));
	}
	for (uint16_t j = 0; j < subtable->bases->numGlyphs; j++) {
		json_value *_base = json_object_new(subtable->classCount);
		for (uint16_t k = 0; k < subtable->classCount; k++) {
			if (subtable->baseArray[j][k].present) {
				json_value *_anchor = json_object_new(2);
				json_object_push(_anchor, "x", json_integer_new(subtable->baseArray[j][k].x));
				json_object_push(_anchor, "y", json_integer_new(subtable->baseArray[j][k].y));
				sds markClassName = sdscatfmt(sdsempty(), "ac_%i", k);
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
	uint16_t classID;
	UT_hash_handle hh;
} classname_hash;
static void parseMarks(json_value *_marks, subtable_gpos_mark_to_single *subtable, classname_hash **h) {
	NEW(subtable->marks);
	subtable->marks->numGlyphs = _marks->u.object.length;
	NEW_N(subtable->marks->glyphs, subtable->marks->numGlyphs);
	NEW(subtable->markArray);
	subtable->markArray->markCount = _marks->u.object.length;
	NEW_N(subtable->markArray->records, subtable->markArray->markCount);
	for (uint16_t j = 0; j < _marks->u.object.length; j++) {
		char *gname = _marks->u.object.values[j].name;
		json_value *anchorRecord = _marks->u.object.values[j].value;
		subtable->marks->glyphs[j].name = sdsnewlen(gname, _marks->u.object.values[j].name_length);

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
}
static void parseBases(json_value *_bases, subtable_gpos_mark_to_single *subtable, classname_hash **h) {
	uint16_t classCount = HASH_COUNT(*h);
	NEW(subtable->bases);
	subtable->bases->numGlyphs = _bases->u.object.length;
	NEW_N(subtable->bases->glyphs, subtable->bases->numGlyphs);
	NEW_N(subtable->baseArray, _bases->u.object.length);
	for (uint16_t j = 0; j < _bases->u.object.length; j++) {
		char *gname = _bases->u.object.values[j].name;
		subtable->bases->glyphs[j].name = sdsnewlen(gname, _bases->u.object.values[j].name_length);
		NEW_N(subtable->baseArray[j], classCount);
		for (uint16_t k = 0; k < classCount; k++) {
			subtable->baseArray[j][k] = otl_anchor_absent();
		}
		json_value *baseRecord = _bases->u.object.values[j].value;
		if (!baseRecord || baseRecord->type != json_object) continue;

		for (uint16_t k = 0; k < baseRecord->u.object.length; k++) {
			sds className = sdsnewlen(baseRecord->u.object.values[k].name, baseRecord->u.object.values[k].name_length);
			classname_hash *s;
			HASH_FIND_STR(*h, className, s);
			if (!s) {
				fprintf(stderr, "[OTFCC-fea] Invalid anchor class name <%s> "
				                "for /%s. This base anchor is ignored.\n",
				        className, gname);
				goto NEXT;
			}
			subtable->baseArray[j][s->classID] = otl_anchor_from_json(baseRecord->u.object.values[k].value);
		NEXT:
			sdsfree(className);
		}
	}
}
otl_subtable *caryll_gpos_mark_to_single_from_json(json_value *_subtable) {
	json_value *_marks = json_obj_get_type(_subtable, "marks", json_object);
	json_value *_bases = json_obj_get_type(_subtable, "bases", json_object);
	if (!_marks || !_bases) return NULL;
	otl_subtable *st;
	NEW(st);
	classname_hash *h = NULL;
	parseMarks(_marks, &(st->gpos_mark_to_single), &h);
	st->gpos_mark_to_single.classCount = HASH_COUNT(h);
	parseBases(_bases, &(st->gpos_mark_to_single), &h);

	classname_hash *s, *tmp;
	HASH_ITER(hh, h, s, tmp) {
		HASH_DEL(h, s);
		sdsfree(s->className);
		free(s);
	}

	return st;
}

caryll_buffer *caryll_write_gpos_mark_to_single(otl_subtable *_subtable) {

	/* structure :
	| format           | 0
	| markCoverage *   |
	| baseCoverage *   |
	| classCount       |
	| markArray    *   |
	| baseArray    *   | 10
	| markCoverage     | 12
	| baseCoverage     | 12 + a
	| markArray        | 12 + a + b
	| baseArray        | 12 + a + b + c
	| anchor directory | 12 + a + b + c + d
	*/

	caryll_buffer *buf = bufnew();
	subtable_gpos_mark_to_single *subtable = &(_subtable->gpos_mark_to_single);

	// we will aggerate these anchors to reduce subtable size as more as
	// possible
	anchor_aggeration_hash *agh = NULL, *s, *tmp;
	for (uint16_t j = 0; j < subtable->marks->numGlyphs; j++) {
		ANCHOR_AGGERATOR_PUSH(agh, subtable->markArray->records[j].anchor);
	}
	for (uint16_t j = 0; j < subtable->bases->numGlyphs; j++) {
		for (uint16_t k = 0; k < subtable->classCount; k++) {
			ANCHOR_AGGERATOR_PUSH(agh, subtable->baseArray[j][k]);
		}
	}
	HASH_SORT(agh, byAnchorIndex);

	bufwrite16b(buf, 1);
	size_t covOffset = 12;
	size_t cp = buf->cursor;
	bufpingpong16b(buf, caryll_write_coverage(subtable->marks), &covOffset, &cp);
	bufpingpong16b(buf, caryll_write_coverage(subtable->bases), &covOffset, &cp);
	bufwrite16b(buf, subtable->classCount);
	// for now, we will write markArray and baseArray.
	// since these tables' length are fixed
	// dealing with them will be much easier.
	size_t markArraySize = 2 + 4 * subtable->marks->numGlyphs;
	size_t baseArraySize = 2 + 2 * subtable->bases->numGlyphs * subtable->classCount;
	size_t markArrayOffset = covOffset;
	size_t baseArrayOffset = covOffset + markArraySize;
	size_t anchorDirectoryOffset = covOffset + markArraySize + baseArraySize;
	bufwrite16b(buf, markArrayOffset);
	bufwrite16b(buf, baseArrayOffset);

	// Write the markArray
	bufseek(buf, markArrayOffset);
	bufwrite16b(buf, subtable->marks->numGlyphs);
	for (uint16_t j = 0; j < subtable->marks->numGlyphs; j++) {
		bufwrite16b(buf, subtable->markArray->records[j].markClass);
		anchor_aggeration_hash *s;
		int position = getPositon(subtable->markArray->records[j].anchor);
		HASH_FIND_INT(agh, &position, s);
		if (s) {
			bufwrite16b(buf, anchorDirectoryOffset + s->index * 6 - markArrayOffset);
		} else {
			bufwrite16b(buf, 0);
		}
	}

	bufseek(buf, baseArrayOffset);
	bufwrite16b(buf, subtable->bases->numGlyphs);
	for (uint16_t j = 0; j < subtable->bases->numGlyphs; j++) {
		for (uint16_t k = 0; k < subtable->classCount; k++) {
			if (subtable->baseArray[j][k].present) {
				anchor_aggeration_hash *s;
				int position = getPositon(subtable->baseArray[j][k]);
				HASH_FIND_INT(agh, &position, s);
				if (s) {
					bufwrite16b(buf, anchorDirectoryOffset + s->index * 6 - baseArrayOffset);
				} else {
					bufwrite16b(buf, 0);
				}
			} else {
				bufwrite16b(buf, 0);
			}
		}
	}

	bufseek(buf, anchorDirectoryOffset);

	HASH_ITER(hh, agh, s, tmp) {
		bufwrite16b(buf, 1);
		bufwrite16b(buf, s->x);
		bufwrite16b(buf, s->y);
		HASH_DEL(agh, s);
		free(s);
	}

	return buf;
}
