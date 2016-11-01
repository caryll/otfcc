#include "gsub-multi.h"

static void deleteGsubMultiEntry(otl_GsubMultiEntry *entry) {
	Handle.dispose(&entry->from);
	DELETE(Coverage.dispose, entry->to);
}

static const caryll_VectorEntryTypeInfo(otl_GsubMultiEntry) gss_typeinfo = {
    .ctor = NULL, .copyctor = NULL, .dtor = deleteGsubMultiEntry};

void otl_delete_gsub_multi(otl_Subtable *_subtable) {
	if (!_subtable) return;
	subtable_gsub_multi *subtable = &(_subtable->gsub_multi);
	caryll_vecDelete(subtable);
}

static subtable_gsub_multi *otl_new_gsub_multi() {
	subtable_gsub_multi *subtable;
	caryll_vecNew(subtable, gss_typeinfo);
	return subtable;
}

otl_Subtable *otl_read_gsub_multi(font_file_pointer data, uint32_t tableLength, uint32_t offset,
                                  const otfcc_Options *options) {
	subtable_gsub_multi *subtable = otl_new_gsub_multi();
	otl_Coverage *from = NULL;
	checkLength(offset + 6);

	from = Coverage.read(data, tableLength, offset + read_16u(data + offset + 2));
	glyphid_t seqCount = read_16u(data + offset + 4);
	if (seqCount != from->numGlyphs) goto FAIL;
	checkLength(offset + 6 + seqCount * 2);

	for (glyphid_t j = 0; j < seqCount; j++) {
		uint32_t seqOffset = offset + read_16u(data + offset + 6 + j * 2);
		otl_Coverage *cov = Coverage.create();
		glyphid_t n = read_16u(data + seqOffset);
		for (glyphid_t k = 0; k < n; k++) {
			Coverage.push(cov, Handle.fromIndex(read_16u(data + seqOffset + 2 + k * 2)));
		}
		caryll_vecPush(subtable, ((otl_GsubMultiEntry){
		                             .from = Handle.copy(from->glyphs[j]), .to = cov,
		                         }));
	}
	Coverage.dispose(from);
	return (otl_Subtable *)subtable;

FAIL:
	if (from) Coverage.dispose(from);
	otl_delete_gsub_multi((otl_Subtable *)subtable);
	return NULL;
}

json_value *otl_gsub_dump_multi(const otl_Subtable *_subtable) {
	const subtable_gsub_multi *subtable = &(_subtable->gsub_multi);
	json_value *st = json_object_new(subtable->length);
	for (glyphid_t j = 0; j < subtable->length; j++) {
		json_object_push(st, subtable->data[j].from.name, Coverage.dump(subtable->data[j].to));
	}
	return st;
}

otl_Subtable *otl_gsub_parse_multi(const json_value *_subtable, const otfcc_Options *options) {
	subtable_gsub_multi *st = otl_new_gsub_multi();

	for (glyphid_t k = 0; k < _subtable->u.object.length; k++) {
		json_value *_to = _subtable->u.object.values[k].value;
		if (!_to || _to->type != json_array) continue;
		caryll_vecPush(st, ((otl_GsubMultiEntry){
		                       .from = Handle.fromName(sdsnewlen(_subtable->u.object.values[k].name,
		                                                         _subtable->u.object.values[k].name_length)),
		                       .to = Coverage.parse(_to),
		                   }));
	}

	return (otl_Subtable *)st;
}

caryll_Buffer *otfcc_build_gsub_multi_subtable(const otl_Subtable *_subtable) {
	const subtable_gsub_multi *subtable = &(_subtable->gsub_multi);
	otl_Coverage *cov = Coverage.create();
	for (glyphid_t j = 0; j < subtable->length; j++) {
		Coverage.push(cov, Handle.copy(subtable->data[j].from));
	}

	bk_Block *root = bk_new_Block(b16, 1,                                          // format
	                              p16, bk_newBlockFromBuffer(Coverage.build(cov)), // coverage
	                              b16, subtable->length,                           // quantity
	                              bkover);
	for (glyphid_t j = 0; j < subtable->length; j++) {
		bk_Block *b = bk_new_Block(b16, subtable->data[j].to->numGlyphs, bkover);
		for (glyphid_t k = 0; k < subtable->data[j].to->numGlyphs; k++) {
			bk_push(b, b16, subtable->data[j].to->glyphs[k].index, bkover);
		}
		bk_push(root, p16, b, bkover);
	}
	Coverage.dispose(cov);
	return bk_build_Block(root);
}
