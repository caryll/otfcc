#include "gpos-mark-to-ligature.h"
#include "gpos-common.h"

void delete_lig_attachment(mark_to_ligature_base *att) {
	if (!att) return;
	if (att->anchors) {
		for (uint16_t k = 0; k < att->componentCount; k++)
			free(att->anchors[k]);
		free(att->anchors);
	}
	free(att);
}

static void delete_mtl_subtable(otl_subtable *_subtable) {
	if (_subtable) {
		subtable_gpos_mark_to_ligature *subtable = &(_subtable->gpos_mark_to_ligature);
		if (subtable->marks) { caryll_delete_coverage(subtable->marks); }
		if (subtable->markArray) { otl_delete_mark_array(subtable->markArray); }
		if (subtable->bases) {
			if (subtable->ligArray) {
				for (uint16_t j = 0; j < subtable->bases->numGlyphs; j++) {
					delete_lig_attachment(subtable->ligArray[j]);
				}
				free(subtable->ligArray);
			}
			caryll_delete_coverage(subtable->bases);
		}
		free(_subtable);
	}
}

void caryll_delete_gpos_mark_to_ligature(otl_lookup *lookup) {
	if (lookup) {
		if (lookup->subtables) {
			for (uint16_t j = 0; j < lookup->subtableCount; j++) {
				delete_mtl_subtable(lookup->subtables[j]);
			}
			free(lookup->subtables);
		}
		free(lookup);
	}
}

otl_subtable *caryll_read_gpos_mark_to_ligature(font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	otl_subtable *_subtable;
	NEW(_subtable);
	subtable_gpos_mark_to_ligature *subtable = &(_subtable->gpos_mark_to_ligature);
	if (tableLength < offset + 12) goto FAIL;
	subtable->marks = caryll_read_coverage(data, tableLength, offset + read_16u(data + offset + 2));
	subtable->bases = caryll_read_coverage(data, tableLength, offset + read_16u(data + offset + 4));
	if (!subtable->marks || subtable->marks->numGlyphs == 0 || !subtable->bases || subtable->bases->numGlyphs == 0)
		goto FAIL;
	subtable->classCount = read_16u(data + offset + 6);
	uint32_t markArrayOffset = offset + read_16u(data + offset + 8);
	subtable->markArray = otl_read_mark_array(data, tableLength, markArrayOffset);
	if (!subtable->markArray || subtable->markArray->markCount != subtable->marks->numGlyphs) goto FAIL;

	uint32_t ligArrayOffset = offset + read_16u(data + offset + 10);
	checkLength(ligArrayOffset + 2 + 2 * subtable->bases->numGlyphs);
	if (read_16u(data + ligArrayOffset) != subtable->bases->numGlyphs) goto FAIL;
	NEW_N(subtable->ligArray, subtable->bases->numGlyphs);
	for (uint16_t j = 0; j < subtable->bases->numGlyphs; j++) {
		subtable->ligArray[j] = NULL;
	}
	for (uint16_t j = 0; j < subtable->bases->numGlyphs; j++) {
		uint32_t ligAttachOffset = ligArrayOffset + read_16u(data + ligArrayOffset + 2 + j * 2);
		NEW(subtable->ligArray[j]);
		subtable->ligArray[j]->anchors = NULL;
		checkLength(ligAttachOffset + 2);
		subtable->ligArray[j]->componentCount = read_16u(data + ligAttachOffset);

		checkLength(ligAttachOffset + 2 + 2 * subtable->ligArray[j]->componentCount * subtable->classCount);
		NEW_N(subtable->ligArray[j]->anchors, subtable->ligArray[j]->componentCount);

		uint32_t _offset = ligAttachOffset + 2;
		for (uint16_t k = 0; k < subtable->ligArray[j]->componentCount; k++) {
			NEW_N(subtable->ligArray[j]->anchors[k], subtable->classCount);
			for (uint16_t m = 0; m < subtable->classCount; m++) {
				uint32_t anchorOffset = read_16u(data + _offset);
				if (anchorOffset) {
					subtable->ligArray[j]->anchors[k][m] =
					    otl_read_anchor(data, tableLength, ligAttachOffset + anchorOffset);
				} else {
					subtable->ligArray[j]->anchors[k][m].present = false;
					subtable->ligArray[j]->anchors[k][m].x = 0;
					subtable->ligArray[j]->anchors[k][m].y = 0;
				}
				_offset += 2;
			}
		}
	}
	goto OK;
FAIL:
	DELETE(delete_mtl_subtable, _subtable);
OK:
	return _subtable;
}

json_value *caryll_gpos_mark_to_ligature_to_json(otl_subtable *st) {
	subtable_gpos_mark_to_ligature *subtable = &(st->gpos_mark_to_ligature);
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
		mark_to_ligature_base *base = subtable->ligArray[j];
		json_value *_base = json_array_new(base->componentCount);
		for (uint16_t k = 0; k < base->componentCount; k++) {
			json_value *_bk = json_object_new(subtable->classCount);
			for (uint16_t m = 0; m < subtable->classCount; m++) {
				if (base->anchors[k][m].present) {
					json_value *_anchor = json_object_new(2);
					json_object_push(_anchor, "x", json_integer_new(base->anchors[k][m].x));
					json_object_push(_anchor, "y", json_integer_new(base->anchors[k][m].y));
					sds markClassName = sdscatfmt(sdsempty(), "ac_%i", m);
					json_object_push_length(_bk, (uint32_t)sdslen(markClassName), markClassName, _anchor);
					sdsfree(markClassName);
				}
			}
			json_array_push(_base, _bk);
		}
		json_object_push(_bases, subtable->bases->glyphs[j].name, preserialize(_base));
	}
	json_object_push(_subtable, "classCount", json_integer_new(subtable->classCount));
	json_object_push(_subtable, "marks", _marks);
	json_object_push(_subtable, "bases", _bases);
	return _subtable;
}

typedef struct {
	sds className;
	uint16_t classID;
	UT_hash_handle hh;
} classname_hash;
static void parseMarks(json_value *_marks, subtable_gpos_mark_to_ligature *subtable, classname_hash **h) {
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
static void parseBases(json_value *_bases, subtable_gpos_mark_to_ligature *subtable, classname_hash **h) {
	uint16_t classCount = HASH_COUNT(*h);
	NEW(subtable->bases);
	subtable->bases->numGlyphs = _bases->u.object.length;
	NEW_N(subtable->bases->glyphs, subtable->bases->numGlyphs);
	NEW_N(subtable->ligArray, _bases->u.object.length);
	for (uint16_t j = 0; j < _bases->u.object.length; j++) {
		subtable->bases->glyphs[j].name =
		    sdsnewlen(_bases->u.object.values[j].name, _bases->u.object.values[j].name_length);
		NEW(subtable->ligArray[j]);
		subtable->ligArray[j]->componentCount = 0;
		subtable->ligArray[j]->anchors = NULL;

		json_value *baseRecord = _bases->u.object.values[j].value;
		if (!baseRecord || baseRecord->type != json_array) continue;
		subtable->ligArray[j]->componentCount = baseRecord->u.array.length;

		NEW_N(subtable->ligArray[j]->anchors, subtable->ligArray[j]->componentCount);

		for (uint16_t k = 0; k < subtable->ligArray[j]->componentCount; k++) {
			json_value *_componentRecord = baseRecord->u.array.values[k];
			NEW_N(subtable->ligArray[j]->anchors[k], classCount);
			for (uint16_t m = 0; m < classCount; m++) {
				subtable->ligArray[j]->anchors[k][m] = otl_anchor_absent();
			}
			if (!_componentRecord || _componentRecord->type != json_object) { continue; }
			for (uint16_t m = 0; m < _componentRecord->u.object.length; m++) {
				sds className = sdsnewlen(_componentRecord->u.object.values[m].name,
				                          _componentRecord->u.object.values[m].name_length);
				classname_hash *s;
				HASH_FIND_STR(*h, className, s);
				if (!s) goto NEXT;
				subtable->ligArray[j]->anchors[k][s->classID] =
				    otl_anchor_from_json(_componentRecord->u.object.values[m].value);

			NEXT:
				sdsfree(className);
			}
		}
	}
}
otl_subtable *caryll_gpos_mark_to_ligature_from_json(json_value *_subtable) {
	json_value *_marks = json_obj_get_type(_subtable, "marks", json_object);
	json_value *_bases = json_obj_get_type(_subtable, "bases", json_object);
	if (!_marks || !_bases) return NULL;
	otl_subtable *st;
	NEW(st);
	classname_hash *h = NULL;
	parseMarks(_marks, &(st->gpos_mark_to_ligature), &h);
	st->gpos_mark_to_ligature.classCount = HASH_COUNT(h);
	parseBases(_bases, &(st->gpos_mark_to_ligature), &h);

	classname_hash *s, *tmp;
	HASH_ITER(hh, h, s, tmp) {
		HASH_DEL(h, s);
		sdsfree(s->className);
		free(s);
	}

	return st;
}

caryll_buffer *caryll_write_gpos_mark_to_ligature(otl_subtable *_subtable) {
	caryll_buffer *buf = bufnew();
	subtable_gpos_mark_to_ligature *subtable = &(_subtable->gpos_mark_to_ligature);

	// we will aggerate these anchors to reduce subtable size as more as
	// possible
	anchor_aggeration_hash *agh = NULL, *s, *tmp;
	for (uint16_t j = 0; j < subtable->marks->numGlyphs; j++) {
		ANCHOR_AGGERATOR_PUSH(agh, subtable->markArray->records[j].anchor);
	}
	for (uint16_t j = 0; j < subtable->bases->numGlyphs; j++) {
		for (uint16_t k = 0; k < subtable->ligArray[j]->componentCount; k++) {
			for (uint16_t m = 0; m < subtable->classCount; m++) {
				ANCHOR_AGGERATOR_PUSH(agh, subtable->ligArray[j]->anchors[k][m]);
			}
		}
	}
	HASH_SORT(agh, byAnchorIndex);

	/* structure :
	| format           | 0
	| markCoverage *   |
	| baseCoverage *   |
	| classCount       |
	| markArray    *   |
	| ligArray     *   | 10
	| markCoverage     | 12
	| baseCoverage     | 12 + a
	| markArray        | 12 + a + b
	| ligArray         | 12 + a + b + c
	| anchor directory | 12 + a + b + c + d
	*/

	bufwrite16b(buf, 1);
	size_t covOffset = 12;
	size_t cp = buf->cursor;
	bufpingpong16b(buf, caryll_write_coverage(subtable->marks), &covOffset, &cp);
	bufpingpong16b(buf, caryll_write_coverage(subtable->bases), &covOffset, &cp);
	bufwrite16b(buf, subtable->classCount);

	size_t markArraySize = 2 + 4 * subtable->marks->numGlyphs;
	size_t ligArrayIndexSize = 2 + 2 * subtable->bases->numGlyphs;
	size_t ligArraySize = ligArrayIndexSize;
	for (uint16_t j = 0; j < subtable->bases->numGlyphs; j++) {
		ligArraySize += 2 + 2 * subtable->ligArray[j]->componentCount * subtable->classCount;
	}
	size_t markArrayOffset = covOffset;
	size_t ligArrayOffset = covOffset + markArraySize;
	size_t anchorDirectoryOffset = covOffset + markArraySize + ligArraySize;

	/* Write the markArray
	| format           | 0
	| markCoverage *   |
	| baseCoverage *   |
	| classCount       |
	| markArray    *   | <-- cursor is here
	| ligArray     *   | 10
	| markCoverage     | 12
	| baseCoverage     | 12 + a
	|                  | <-- offset is here
	*/
	bufwrite16b(buf, markArrayOffset);
	bufwrite16b(buf, ligArrayOffset);

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

	/* Write the ligArray
	| format           | 0
	| markCoverage *   |
	| baseCoverage *   |
	| classCount       |
	| markArray    *   |
	| ligArray     *   | 10
	| markCoverage     | 12
	| baseCoverage     | 12 + a
	| markArray        | 12 + a + b <-- offset
	| ligArray         | <-- cursor is here
	*/

	bufseek(buf, ligArrayOffset);
	bufwrite16b(buf, subtable->bases->numGlyphs);
	size_t ligAttachOffset = ligArrayOffset + ligArrayIndexSize;
	size_t cpls = buf->cursor;
	for (uint16_t j = 0; j < subtable->bases->numGlyphs; j++) {
		bufping16bd(buf, &ligAttachOffset, &ligArrayOffset, &cpls);
		size_t ligStart = buf->cursor;
		bufwrite16b(buf, subtable->ligArray[j]->componentCount);
		for (uint16_t k = 0; k < subtable->ligArray[j]->componentCount; k++) {
			for (uint16_t m = 0; m < subtable->classCount; m++) {
				if (subtable->ligArray[j]->anchors[k][m].present) {
					anchor_aggeration_hash *s;
					int position = getPositon(subtable->ligArray[j]->anchors[k][m]);
					HASH_FIND_INT(agh, &position, s);
					if (s) {
						bufwrite16b(buf, anchorDirectoryOffset + s->index * 6 - ligStart);
					} else {
						bufwrite16b(buf, 0);
					}
				} else {
					bufwrite16b(buf, 0);
				}
			}
		}
		bufpong(buf, &ligAttachOffset, &cpls);
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
