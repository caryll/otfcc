#include "gpos-cursive.h"
#include "gpos-common.h"

static void deleteGposCursiveEntry(otl_GposCursiveEntry *entry) {
	Handle.dispose(&entry->target);
}

static const caryll_VectorEntryTypeInfo(otl_GposCursiveEntry) gss_typeinfo = {
    .ctor = NULL, .copyctor = NULL, .dtor = deleteGposCursiveEntry};

void otl_delete_gpos_cursive(otl_Subtable *_subtable) {
	if (!_subtable) return;
	subtable_gpos_cursive *subtable = &(_subtable->gpos_cursive);
	caryll_vecDelete(subtable);
}

subtable_gpos_cursive *otl_new_gpos_cursive() {
	subtable_gpos_cursive *subtable;
	caryll_vecNew(subtable, gss_typeinfo);
	return subtable;
}

otl_Subtable *otl_read_gpos_cursive(const font_file_pointer data, uint32_t tableLength, uint32_t offset,
                                    const otfcc_Options *options) {
	subtable_gpos_cursive *subtable = otl_new_gpos_cursive();
	otl_Coverage *targets = NULL;

	checkLength(offset + 6);

	targets = Coverage.read(data, tableLength, offset + read_16u(data + offset + 2));
	if (!targets || targets->numGlyphs == 0) goto FAIL;

	glyphid_t valueCount = read_16u(data + offset + 4);
	checkLength(offset + 6 + 4 * valueCount);
	if (valueCount != targets->numGlyphs) goto FAIL;

	for (glyphid_t j = 0; j < valueCount; j++) {
		uint16_t enterOffset = read_16u(data + offset + 6 + 4 * j);
		uint16_t exitOffset = read_16u(data + offset + 6 + 4 * j + 2);
		otl_Anchor enter = otl_anchor_absent();
		otl_Anchor exit = otl_anchor_absent();
		if (enterOffset) { enter = otl_read_anchor(data, tableLength, offset + enterOffset); }
		if (exitOffset) { exit = otl_read_anchor(data, tableLength, offset + exitOffset); }
		caryll_vecPush(subtable, ((otl_GposCursiveEntry){
		                             .target = Handle.copy(targets->glyphs[j]), .enter = enter, .exit = exit}));
	}
	if (targets) Coverage.dispose(targets);
	return (otl_Subtable *)subtable;
FAIL:
	if (targets) Coverage.dispose(targets);
	otl_delete_gpos_cursive((otl_Subtable *)subtable);
	return NULL;
}

json_value *otl_gpos_dump_cursive(const otl_Subtable *_subtable) {
	const subtable_gpos_cursive *subtable = &(_subtable->gpos_cursive);
	json_value *st = json_object_new(subtable->length);
	for (glyphid_t j = 0; j < subtable->length; j++) {
		json_value *rec = json_object_new(2);
		json_object_push(rec, "enter", otl_dump_anchor(subtable->data[j].enter));
		json_object_push(rec, "exit", otl_dump_anchor(subtable->data[j].exit));
		json_object_push(st, subtable->data[j].target.name, preserialize(rec));
	}
	return st;
}

otl_Subtable *otl_gpos_parse_cursive(const json_value *_subtable, const otfcc_Options *options) {
	subtable_gpos_cursive *subtable = otl_new_gpos_cursive();
	for (glyphid_t j = 0; j < _subtable->u.object.length; j++) {
		if (_subtable->u.object.values[j].value && _subtable->u.object.values[j].value->type == json_object) {
			sds gname = sdsnewlen(_subtable->u.object.values[j].name, _subtable->u.object.values[j].name_length);
			caryll_vecPush(subtable,
			               ((otl_GposCursiveEntry){
			                   .target = Handle.fromName(gname),
			                   .enter = otl_parse_anchor(json_obj_get(_subtable->u.object.values[j].value, "enter")),
			                   .exit = otl_parse_anchor(json_obj_get(_subtable->u.object.values[j].value, "exit")),
			               }));
		}
	}
	return (otl_Subtable *)subtable;
}

caryll_Buffer *otfcc_build_gpos_cursive(const otl_Subtable *_subtable) {
	const subtable_gpos_cursive *subtable = &(_subtable->gpos_cursive);
	otl_Coverage *cov = Coverage.create();
	for (glyphid_t j = 0; j < subtable->length; j++) {
		Coverage.push(cov, Handle.copy(subtable->data[j].target));
	}

	bk_Block *root = bk_new_Block(b16, 1,                                          // format
	                              p16, bk_newBlockFromBuffer(Coverage.build(cov)), // Coverage
	                              b16, subtable->length,                           // EntryExitCount
	                              bkover);
	for (glyphid_t j = 0; j < subtable->length; j++) {
		bk_push(root,                                       // EntryExitRecord[.]
		        p16, bkFromAnchor(subtable->data[j].enter), // enter
		        p16, bkFromAnchor(subtable->data[j].exit),  // exit
		        bkover);
	}
	Coverage.dispose(cov);

	return bk_build_Block(root);
}
