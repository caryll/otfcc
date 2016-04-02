#include "otl-gpos-mark-to-single.h"
#include "otl-gpos-common.h"

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
		if (lookup->subtables)
			for (uint16_t j = 0; j < lookup->subtableCount; j++)
				delete_mtb_subtable(lookup->subtables[j]);
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
	uint16_t markArrayOffset = subtableOffset + read_16u(data + subtableOffset + 8);
	subtable->markArray = otl_read_mark_array(data, tableLength, markArrayOffset);
	if (!subtable->markArray || subtable->markArray->markCount != subtable->marks->numGlyphs) goto FAIL;

	uint16_t baseArrayOffset = subtableOffset + read_16u(data + subtableOffset + 10);
	checkLength(baseArrayOffset + 2 + 2 * subtable->bases->numGlyphs * subtable->classCount);
	if (read_16u(data + baseArrayOffset) != subtable->bases->numGlyphs) goto FAIL;
	NEW_N(subtable->baseArray, subtable->bases->numGlyphs);
	uint16_t _offset = baseArrayOffset + 2;
	for (uint16_t j = 0; j < subtable->bases->numGlyphs; j++) {
		NEW_N(subtable->baseArray[j], subtable->classCount);
		for (uint16_t k = 0; k < subtable->classCount; k++) {
			if (read_16u(data + _offset)) {
				subtable->baseArray[j][k] =
				    otl_read_anchor(data, tableLength, baseArrayOffset + read_16u(data + _offset));
			} else {
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

void caryll_gpos_mark_to_single_to_json(otl_lookup *lookup, json_value *dump) {
	json_object_push(dump, "type", json_string_new(lookup->type == otl_type_gpos_mark_to_base ? "gpos_mark_to_base"
	                                                                                            : "gpos_mark_to_mark"));
	json_value *_subtables = json_array_new(lookup->subtableCount);
	for (uint16_t j = 0; j < lookup->subtableCount; j++)
		if (lookup->subtables[j]) {
			subtable_gpos_mark_to_single *subtable = &(lookup->subtables[j]->gpos_mark_to_single);
			json_value *_subtable = json_object_new(3);
			json_value *_marks = json_object_new(subtable->marks->numGlyphs);
			json_value *_bases = json_object_new(subtable->bases->numGlyphs);
			for (uint16_t j = 0; j < subtable->marks->numGlyphs; j++) {
				json_value *_mark = json_object_new(3);
				json_object_push(_mark, "class", json_integer_new(subtable->markArray->records[j].markClass));
				json_object_push(_mark, "x", json_integer_new(subtable->markArray->records[j].anchor.x));
				json_object_push(_mark, "y", json_integer_new(subtable->markArray->records[j].anchor.y));
				json_object_push(_marks, subtable->marks->glyphs[j].name, preserialize(_mark));
			}
			for (uint16_t j = 0; j < subtable->bases->numGlyphs; j++) {
				json_value *_base = json_array_new(subtable->classCount);
				for (uint16_t k = 0; k < subtable->classCount; k++) {
					json_value *_anchor = json_object_new(2);
					json_object_push(_anchor, "x", json_integer_new(subtable->baseArray[j][k].x));
					json_object_push(_anchor, "y", json_integer_new(subtable->baseArray[j][k].y));
					json_array_push(_base, _anchor);
				}
				json_object_push(_bases, subtable->bases->glyphs[j].name, preserialize(_base));
			}
			json_object_push(_subtable, "classCount", json_integer_new(subtable->classCount));
			json_object_push(_subtable, "marks", _marks);
			json_object_push(_subtable, "bases", _bases);
			json_array_push(_subtables, _subtable);
		}
	json_object_push(dump, "subtables", _subtables);
}

static void parseMarks(json_value *_marks, subtable_gpos_mark_to_single *subtable, uint16_t classCount) {
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
		if (anchorRecord && anchorRecord->type == json_object) {
			subtable->markArray->records[j].markClass = json_obj_getint(anchorRecord, "class");
			subtable->markArray->records[j].anchor.x = json_obj_getnum(anchorRecord, "x");
			subtable->markArray->records[j].anchor.y = json_obj_getnum(anchorRecord, "y");
		} else {
			subtable->markArray->records[j].markClass = 0;
			subtable->markArray->records[j].anchor.x = 0;
			subtable->markArray->records[j].anchor.y = 0;
		}
	}
}
static void parseBases(json_value *_bases, subtable_gpos_mark_to_single *subtable, uint16_t classCount) {
	NEW(subtable->bases);
	subtable->bases->numGlyphs = _bases->u.object.length;
	NEW_N(subtable->bases->glyphs, subtable->bases->numGlyphs);
	NEW_N(subtable->baseArray, _bases->u.object.length);
	for (uint16_t j = 0; j < _bases->u.object.length; j++) {
		char *gname = _bases->u.object.values[j].name;
		subtable->bases->glyphs[j].name = sdsnewlen(gname, _bases->u.object.values[j].name_length);
		NEW_N(subtable->baseArray[j], classCount);
		for (uint16_t k = 0; k < classCount; k++) {
			subtable->baseArray[j][k].x = 0;
			subtable->baseArray[j][k].y = 0;
		}
		json_value *baseRecord = _bases->u.object.values[j].value;
		if (baseRecord && baseRecord->type == json_array) {
			for (uint16_t k = 0; k < classCount && k < baseRecord->u.array.length; k++) {
				json_value *anchor = baseRecord->u.array.values[k];
				if (anchor->type == json_object) {
					subtable->baseArray[j][k].x = json_obj_getnum(anchor, "x");
					subtable->baseArray[j][k].y = json_obj_getnum(anchor, "y");
				}
			}
		}
	}
}
otl_lookup *caryll_gpos_mark_to_single_from_json(json_value *_lookup, char *_type) {
	otl_lookup *lookup = NULL;
	json_value *_subtables = json_obj_get_type(_lookup, "subtables", json_array);
	if (!_subtables) goto FAIL;

	NEW(lookup);
	lookup->type = strcmp(_type, "gpos_mark_to_mark") == 0 ? otl_type_gpos_mark_to_mark : otl_type_gpos_mark_to_base;
	lookup->subtableCount = _subtables->u.array.length;
	NEW_N(lookup->subtables, lookup->subtableCount);

	uint16_t jj = 0;
	for (uint16_t j = 0; j < lookup->subtableCount; j++) {
		json_value *_subtable = _subtables->u.array.values[j];
		if (_subtable && _subtable->type == json_object) {
			json_value *_marks = json_obj_get_type(_subtable, "marks", json_object);
			json_value *_bases = json_obj_get_type(_subtable, "bases", json_object);
			uint16_t classCount = json_obj_getint(_subtable, "classCount");
			if (_marks && _bases && classCount) {
				otl_subtable *st;
				NEW(st);
				st->gpos_mark_to_single.classCount = classCount;
				parseMarks(_marks, &(st->gpos_mark_to_single), classCount);
				parseBases(_bases, &(st->gpos_mark_to_single), classCount);
				lookup->subtables[jj] = st;
				jj += 1;
			}
		}
	}
	lookup->subtableCount = jj;
	return lookup;

FAIL:
	DELETE(caryll_delete_gpos_mark_to_single, lookup);
	return NULL;
}

typedef struct {
	int position;
	int x;
	int y;
	uint16_t index;
	UT_hash_handle hh;
} anchor_aggeration_hash;
static INLINE int getPositon(otl_anchor anchor) { return ((uint16_t)anchor.x) << 16 | ((uint16_t)anchor.y); }
static INLINE int byAnchorIndex(anchor_aggeration_hash *a, anchor_aggeration_hash *b) { return a->index - b->index; }
caryll_buffer *caryll_write_gpos_mark_to_single(otl_subtable *_subtable) {
	caryll_buffer *buf = bufnew();
	subtable_gpos_mark_to_single *subtable = &(_subtable->gpos_mark_to_single);
	// we will aggerate these anchors

	anchor_aggeration_hash *agh = NULL, *s, *tmp;
	for (uint16_t j = 0; j < subtable->marks->numGlyphs; j++) {
		int position = getPositon(subtable->markArray->records[j].anchor);
		HASH_FIND_INT(agh, &position, s);
		if (!s) {
			NEW(s);
			s->position = position;
			s->x = subtable->markArray->records[j].anchor.x;
			s->y = subtable->markArray->records[j].anchor.y;
			s->index = HASH_COUNT(agh);
			HASH_ADD_INT(agh, position, s);
		}
	}
	for (uint16_t j = 0; j < subtable->bases->numGlyphs; j++) {
		for (uint16_t k = 0; k < subtable->classCount; k++) {
			int position = getPositon(subtable->baseArray[j][k]);
			HASH_FIND_INT(agh, &position, s);
			if (!s) {
				NEW(s);
				s->position = position;
				s->x = subtable->baseArray[j][k].x;
				s->y = subtable->baseArray[j][k].y;
				s->index = HASH_COUNT(agh);
				HASH_ADD_INT(agh, position, s);
			}
		}
	}
	HASH_SORT(agh, byAnchorIndex);

	bufwrite16b(buf, 1);
	size_t offset = 12;
	size_t cp = buf->cursor;
	bufpingpong16b(buf, caryll_write_coverage(subtable->marks), &offset, &cp);
	bufpingpong16b(buf, caryll_write_coverage(subtable->bases), &offset, &cp);
	bufwrite16b(buf, subtable->classCount);
	// for now, we will write markArray and baseArray.
	// since these tables' length are fixed
	// dealing with them will be much easier.
	size_t markArraySize = 2 + 4 * subtable->marks->numGlyphs;
	size_t baseArraySize = 2 + 2 * subtable->bases->numGlyphs * subtable->classCount;
	bufwrite16b(buf, offset);
	bufwrite16b(buf, offset + markArraySize);
	// Write the markArray
	bufseek(buf, offset);
	size_t markArrayStart = buf->cursor;
	bufwrite16b(buf, subtable->marks->numGlyphs);
	for (uint16_t j = 0; j < subtable->marks->numGlyphs; j++) {
		bufwrite16b(buf, subtable->markArray->records[j].markClass);
		anchor_aggeration_hash *s;
		int position = getPositon(subtable->markArray->records[j].anchor);
		HASH_FIND_INT(agh, &position, s);
		if (s) {
			bufwrite16b(buf, offset + s->index * 6 + markArraySize + baseArraySize - markArrayStart);
		} else {
			bufwrite16b(buf, 0);
		}
	}

	size_t baseArrayStart = buf->cursor;
	bufwrite16b(buf, subtable->bases->numGlyphs);
	for (uint16_t j = 0; j < subtable->bases->numGlyphs; j++) {
		for (uint16_t k = 0; k < subtable->classCount; k++) {
			anchor_aggeration_hash *s;
			int position = getPositon(subtable->baseArray[j][k]);
			HASH_FIND_INT(agh, &position, s);
			if (s) {
				bufwrite16b(buf, offset + s->index * 6 + markArraySize + baseArraySize - baseArrayStart);
			} else {
				bufwrite16b(buf, 0);
			}
		}
	}

	HASH_ITER(hh, agh, s, tmp) {
		bufwrite16b(buf, 1);
		bufwrite16b(buf, s->x);
		bufwrite16b(buf, s->y);
		HASH_DEL(agh, s);
		free(s);
	}

	return buf;
}
