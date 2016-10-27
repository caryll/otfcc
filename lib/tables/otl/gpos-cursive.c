#include "gpos-cursive.h"
#include "gpos-common.h"
void otl_delete_gpos_cursive(otl_Subtable *subtable) {
	if (subtable) {
		otl_delete_Coverage(subtable->gpos_cursive.coverage);
		free(subtable->gpos_cursive.enter);
		free(subtable->gpos_cursive.exit);
		free(subtable);
	}
}

otl_Subtable *otl_read_gpos_cursive(const font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	otl_Subtable *_subtable;
	NEW(_subtable);
	subtable_gpos_cursive *subtable = &(_subtable->gpos_cursive);
	subtable->coverage = NULL;
	subtable->enter = NULL;
	subtable->exit = NULL;
	checkLength(offset + 6);

	subtable->coverage = otl_read_Coverage(data, tableLength, offset + read_16u(data + offset + 2));
	if (!subtable->coverage || subtable->coverage->numGlyphs == 0) goto FAIL;
	NEW_N(subtable->enter, subtable->coverage->numGlyphs);
	NEW_N(subtable->exit, subtable->coverage->numGlyphs);

	glyphid_t valueCount = read_16u(data + offset + 4);
	checkLength(offset + 6 + 4 * valueCount);
	if (valueCount != subtable->coverage->numGlyphs) goto FAIL;

	for (glyphid_t j = 0; j < subtable->coverage->numGlyphs; j++) {
		uint16_t enterOffset = read_16u(data + offset + 6 + 4 * j);
		uint16_t exitOffset = read_16u(data + offset + 6 + 4 * j + 2);
		subtable->enter[j] = otl_anchor_absent();
		subtable->exit[j] = otl_anchor_absent();
		if (enterOffset) { subtable->enter[j] = otl_read_anchor(data, tableLength, offset + enterOffset); }
		if (exitOffset) { subtable->exit[j] = otl_read_anchor(data, tableLength, offset + exitOffset); }
	}
	goto OK;
FAIL:
	if (subtable->coverage) otl_delete_Coverage(subtable->coverage);
	if (subtable->enter) free(subtable->enter);
	if (subtable->exit) free(subtable->exit);
	_subtable = NULL;
OK:
	return _subtable;
}

json_value *otl_gpos_dump_cursive(const otl_Subtable *_subtable) {
	const subtable_gpos_cursive *subtable = &(_subtable->gpos_cursive);
	json_value *st = json_object_new(subtable->coverage->numGlyphs);
	for (glyphid_t j = 0; j < subtable->coverage->numGlyphs; j++) {
		json_value *rec = json_object_new(2);
		json_object_push(rec, "enter", otl_dump_anchor(subtable->enter[j]));
		json_object_push(rec, "exit", otl_dump_anchor(subtable->exit[j]));
		json_object_push(st, subtable->coverage->glyphs[j].name, preserialize(rec));
	}
	return st;
}

otl_Subtable *otl_gpos_parse_cursive(const json_value *_subtable, const otfcc_Options *options) {
	otl_Subtable *_st;
	NEW(_st);
	subtable_gpos_cursive *subtable = &(_st->gpos_cursive);
	NEW(subtable->coverage);
	NEW_N(subtable->coverage->glyphs, _subtable->u.object.length);
	NEW_N(subtable->enter, _subtable->u.object.length);
	NEW_N(subtable->exit, _subtable->u.object.length);
	glyphid_t jj = 0;
	for (glyphid_t j = 0; j < _subtable->u.object.length; j++) {
		if (_subtable->u.object.values[j].value && _subtable->u.object.values[j].value->type == json_object) {
			sds gname = sdsnewlen(_subtable->u.object.values[j].name, _subtable->u.object.values[j].name_length);
			subtable->coverage->glyphs[jj] = handle_fromName(gname);
			subtable->enter[jj] = otl_parse_anchor(json_obj_get(_subtable->u.object.values[j].value, "enter"));
			subtable->exit[jj] = otl_parse_anchor(json_obj_get(_subtable->u.object.values[j].value, "exit"));
			jj++;
		}
	}
	subtable->coverage->numGlyphs = jj;
	return _st;
}

caryll_Buffer *caryll_build_gpos_cursive(const otl_Subtable *_subtable) {
	const subtable_gpos_cursive *subtable = &(_subtable->gpos_cursive);

	bk_Block *root = bk_new_Block(b16, 1,                                                             // format
	                              p16, bk_newBlockFromBuffer(otl_build_Coverage(subtable->coverage)), // Coverage
	                              b16, subtable->coverage->numGlyphs,                                 // EntryExitCount
	                              bkover);
	for (glyphid_t j = 0; j < subtable->coverage->numGlyphs; j++) {
		bk_push(root,                                  // EntryExitRecord[.]
		        p16, bkFromAnchor(subtable->enter[j]), // enter
		        p16, bkFromAnchor(subtable->exit[j]),  // exit
		        bkover);
	}

	return bk_build_Block(root);
}
