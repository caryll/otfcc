#include "gpos-cursive.h"
#include "gpos-common.h"
void caryll_delete_gpos_cursive(otl_lookup *lookup) {
	if (lookup) {
		if (lookup->subtables) {
			for (uint16_t j = 0; j < lookup->subtableCount; j++)
				if (lookup->subtables[j]) {
					caryll_delete_coverage(lookup->subtables[j]->gpos_cursive.coverage);
					free(lookup->subtables[j]->gpos_cursive.enter);
					free(lookup->subtables[j]->gpos_cursive.exit);
					free(lookup->subtables[j]);
				}
			free(lookup->subtables);
		}
		FREE(lookup);
	}
}
otl_subtable *caryll_read_gpos_cursive(font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	otl_subtable *_subtable;
	NEW(_subtable);
	subtable_gpos_cursive *subtable = &(_subtable->gpos_cursive);
	subtable->coverage = NULL;
	subtable->enter = NULL;
	subtable->exit = NULL;
	checkLength(offset + 6);

	subtable->coverage = caryll_read_coverage(data, tableLength, offset + read_16u(data + offset + 2));
	if (!subtable->coverage || subtable->coverage->numGlyphs == 0) goto FAIL;
	NEW_N(subtable->enter, subtable->coverage->numGlyphs);
	NEW_N(subtable->exit, subtable->coverage->numGlyphs);

	uint16_t valueCount = read_16u(data + offset + 4);
	checkLength(offset + 6 + 4 * valueCount);
	if (valueCount != subtable->coverage->numGlyphs) goto FAIL;

	for (uint16_t j = 0; j < subtable->coverage->numGlyphs; j++) {
		uint16_t enterOffset = read_16u(data + offset + 6 + 4 * j);
		uint16_t exitOffset = read_16u(data + offset + 6 + 4 * j + 2);
		subtable->enter[j] = otl_anchor_absent();
		subtable->exit[j] = otl_anchor_absent();
		if (enterOffset) { subtable->enter[j] = otl_read_anchor(data, tableLength, offset + enterOffset); }
		if (exitOffset) { subtable->exit[j] = otl_read_anchor(data, tableLength, offset + exitOffset); }
	}
	goto OK;
FAIL:
	if (subtable->coverage) caryll_delete_coverage(subtable->coverage);
	if (subtable->enter) free(subtable->enter);
	if (subtable->exit) free(subtable->exit);
	_subtable = NULL;
OK:
	return _subtable;
}

json_value *caryll_gpos_cursive_to_json(otl_subtable *_subtable) {
	subtable_gpos_cursive *subtable = &(_subtable->gpos_cursive);
	json_value *st = json_object_new(subtable->coverage->numGlyphs);
	for (uint16_t j = 0; j < subtable->coverage->numGlyphs; j++) {
		json_value *rec = json_object_new(2);
		json_object_push(rec, "enter", otl_anchor_to_json(subtable->enter[j]));
		json_object_push(rec, "exit", otl_anchor_to_json(subtable->exit[j]));
		json_object_push(st, subtable->coverage->glyphs[j].name, preserialize(rec));
	}
	return st;
}

otl_subtable *caryll_gpos_cursive_from_json(json_value *_subtable) {
	otl_subtable *_st;
	NEW(_st);
	subtable_gpos_cursive *subtable = &(_st->gpos_cursive);
	NEW(subtable->coverage);
	NEW_N(subtable->coverage->glyphs, _subtable->u.object.length);
	NEW_N(subtable->enter, _subtable->u.object.length);
	NEW_N(subtable->exit, _subtable->u.object.length);
	uint16_t jj = 0;
	for (uint16_t j = 0; j < _subtable->u.object.length; j++) {
		if (_subtable->u.object.values[j].value && _subtable->u.object.values[j].value->type == json_object) {
			sds gname = sdsnewlen(_subtable->u.object.values[j].name, _subtable->u.object.values[j].name_length);
			subtable->coverage->glyphs[jj].name = gname;
			subtable->enter[jj] = otl_anchor_from_json(json_obj_get(_subtable->u.object.values[j].value, "enter"));
			subtable->exit[jj] = otl_anchor_from_json(json_obj_get(_subtable->u.object.values[j].value, "exit"));
			jj++;
		}
	}
	subtable->coverage->numGlyphs = jj;
	return _st;
}

caryll_buffer *caryll_write_gpos_cursive(otl_subtable *_subtable) {
	anchor_aggeration_hash *agh = NULL, *s, *tmp;
	subtable_gpos_cursive *subtable = &(_subtable->gpos_cursive);
	for (uint16_t j = 0; j < subtable->coverage->numGlyphs; j++) {
		ANCHOR_AGGERATOR_PUSH(agh, subtable->enter[j]);
		ANCHOR_AGGERATOR_PUSH(agh, subtable->exit[j]);
	}
	HASH_SORT(agh, byAnchorIndex);
	caryll_buffer *buf = bufnew();
	bufwrite16b(buf, 1);
	size_t covOffset = 6 + 4 * subtable->coverage->numGlyphs;
	size_t cp = buf->cursor;
	bufpingpong16b(buf, caryll_write_coverage(subtable->coverage), &covOffset, &cp);
	bufwrite16b(buf, subtable->coverage->numGlyphs);
	for (uint16_t j = 0; j < subtable->coverage->numGlyphs; j++) {
		if (subtable->enter[j].present) {
			anchor_aggeration_hash *s;
			int position = getPositon(subtable->enter[j]);
			HASH_FIND_INT(agh, &position, s);
			if (s) {
				bufwrite16b(buf, covOffset + s->index * 6);
			} else {
				bufwrite16b(buf, 0);
			}
		} else {
			bufwrite16b(buf, 0);
		}
		if (subtable->exit[j].present) {
			anchor_aggeration_hash *s;
			int position = getPositon(subtable->exit[j]);
			HASH_FIND_INT(agh, &position, s);
			if (s) {
				bufwrite16b(buf, covOffset + s->index * 6);
			} else {
				bufwrite16b(buf, 0);
			}
		} else {
			bufwrite16b(buf, 0);
		}
	}
	bufseek(buf, covOffset);
	HASH_ITER(hh, agh, s, tmp) {
		bufwrite16b(buf, 1);
		bufwrite16b(buf, s->x);
		bufwrite16b(buf, s->y);
		HASH_DEL(agh, s);
		free(s);
	}
	return buf;
}
