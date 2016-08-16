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
	subtable_gpos_cursive *subtable = &(_subtable->gpos_cursive);

	caryll_bkblock *root =
	    new_bkblock(b16, 1,                                                                  // format
	                p16, new_bkblock_from_buffer(caryll_write_coverage(subtable->coverage)), // Coverage
	                b16, subtable->coverage->numGlyphs,                                      // EntryExitCount
	                bkover);
	for (uint16_t j = 0; j < subtable->coverage->numGlyphs; j++) {
		bkblock_push(root,                                  // EntryExitRecord[.]
		             p16, bkFromAnchor(subtable->enter[j]), // enter
		             p16, bkFromAnchor(subtable->exit[j]),  // exit
		             bkover);
	}

	caryll_bkgraph *f = caryll_bkgraph_from_block(root);
	caryll_minimize_bkgraph(f);
	caryll_buffer *buf = caryll_write_bkgraph(f);
	caryll_delete_bkgraph(f);

	return buf;
}
