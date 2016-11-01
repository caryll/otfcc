#include "gpos-single.h"
#include "gpos-common.h"

static void deleteGposSingleEntry(otl_GposSingleEntry *entry) {
	Handle.dispose(&entry->target);
}

static const caryll_VectorEntryTypeInfo(otl_GposSingleEntry) gss_typeinfo = {
    .ctor = NULL, .copyctor = NULL, .dtor = deleteGposSingleEntry};

void otl_delete_gpos_single(otl_Subtable *_subtable) {
	if (!_subtable) return;
	subtable_gpos_single *subtable = &(_subtable->gpos_single);
	caryll_vecDelete(subtable);
}

subtable_gpos_single *otl_new_gpos_single() {
	subtable_gpos_single *subtable;
	caryll_vecNew(subtable, gss_typeinfo);
	return subtable;
}

otl_Subtable *otl_read_gpos_single(const font_file_pointer data, uint32_t tableLength, uint32_t offset,
                                   const otfcc_Options *options) {
	subtable_gpos_single *subtable = otl_new_gpos_single();
	otl_Coverage *targets = NULL;

	checkLength(offset + 6);

	uint16_t subtableFormat = read_16u(data + offset);
	targets = Coverage.read(data, tableLength, offset + read_16u(data + offset + 2));
	if (!targets || targets->numGlyphs == 0) goto FAIL;

	if (subtableFormat == 1) {
		otl_PositionValue v = read_gpos_value(data, tableLength, offset + 6, read_16u(data + offset + 4));
		for (glyphid_t j = 0; j < targets->numGlyphs; j++) {
			caryll_vecPush(subtable, ((otl_GposSingleEntry){
			                             .target = Handle.copy(targets->glyphs[j]), .value = v,
			                         }));
		}
	} else {
		uint16_t valueFormat = read_16u(data + offset + 4);
		uint16_t valueCount = read_16u(data + offset + 6);
		checkLength(offset + 8 + position_format_length(valueFormat) * valueCount);
		if (valueCount != targets->numGlyphs) goto FAIL;

		for (glyphid_t j = 0; j < targets->numGlyphs; j++) {
			caryll_vecPush(
			    subtable, ((otl_GposSingleEntry){
			                  .target = Handle.copy(targets->glyphs[j]),
			                  .value = read_gpos_value(
			                      data, tableLength, offset + 8 + j * position_format_length(valueFormat), valueFormat),
			              }));
		}
	}
	if (targets) Coverage.dispose(targets);
	return (otl_Subtable *)subtable;

FAIL:
	if (targets) Coverage.dispose(targets);
	otl_delete_gpos_single((otl_Subtable *)subtable);
	return NULL;
}

json_value *otl_gpos_dump_single(const otl_Subtable *_subtable) {
	const subtable_gpos_single *subtable = &(_subtable->gpos_single);
	json_value *st = json_object_new(subtable->length);
	for (glyphid_t j = 0; j < subtable->length; j++) {
		json_object_push(st, subtable->data[j].target.name, gpos_dump_value(subtable->data[j].value));
	}
	return st;
}
otl_Subtable *otl_gpos_parse_single(const json_value *_subtable, const otfcc_Options *options) {
	subtable_gpos_single *subtable = otl_new_gpos_single();
	for (glyphid_t j = 0; j < _subtable->u.object.length; j++) {
		if (_subtable->u.object.values[j].value && _subtable->u.object.values[j].value->type == json_object) {
			sds gname = sdsnewlen(_subtable->u.object.values[j].name, _subtable->u.object.values[j].name_length);
			caryll_vecPush(subtable, ((otl_GposSingleEntry){
			                             .target = Handle.fromName(gname),
			                             .value = gpos_parse_value(_subtable->u.object.values[j].value),
			                         }));
		}
	}
	return (otl_Subtable *)subtable;
}

caryll_Buffer *otfcc_build_gpos_single(const otl_Subtable *_subtable) {
	const subtable_gpos_single *subtable = &(_subtable->gpos_single);
	bool isConst = subtable->length > 0;
	uint16_t format = 0;
	if (subtable->length > 0) {
		for (glyphid_t j = 0; j < subtable->length; j++) {
			isConst = isConst && (subtable->data[j].value.dx == subtable->data[0].value.dx) &&
			          (subtable->data[j].value.dy == subtable->data[0].value.dy) &&
			          (subtable->data[j].value.dWidth == subtable->data[0].value.dWidth) &&
			          (subtable->data[j].value.dHeight == subtable->data[0].value.dHeight);
			format |= required_position_format(subtable->data[j].value);
		}
	}
	otl_Coverage *cov = Coverage.create();
	for (glyphid_t j = 0; j < subtable->length; j++) {
		Coverage.push(cov, Handle.copy(subtable->data[j].target));
	}

	if (isConst) {
		bk_Block *b = (bk_new_Block(b16, 1, // Format
		                            p16,
		                            bk_newBlockFromBuffer(Coverage.build(cov)),              // coverage
		                            b16, format,                                             // format
		                            bkembed, bk_gpos_value(subtable->data[0].value, format), // value
		                            bkover));
		Coverage.dispose(cov);
		return bk_build_Block(b);
	} else {
		bk_Block *b = bk_new_Block(b16, 2,                                          // Format
		                           p16, bk_newBlockFromBuffer(Coverage.build(cov)), // coverage
		                           b16, format,                                     // format
		                           b16, subtable->length,                           // quantity
		                           bkover);
		for (glyphid_t k = 0; k < subtable->length; k++) {
			bk_push(b, bkembed, bk_gpos_value(subtable->data[k].value, format), // value
			        bkover);
		}
		Coverage.dispose(cov);
		return bk_build_Block(b);
	}
}
