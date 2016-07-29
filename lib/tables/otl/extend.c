#include "extend.h"

// Extended tables are special
// We will only deal with reading, and they will be flatten.
static otl_subtable *_caryll_read_otl_extend(font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset,
                                             otl_lookup_type BASIS) {
	otl_subtable *_subtable;
	NEW(_subtable);
	checkLength(subtableOffset + 8);
	subtable_extend *subtable = &(_subtable->extend);
	subtable->type = read_16u(data + subtableOffset + 2) + BASIS;
	subtable->subtable = caryll_read_otl_subtable(data, tableLength,
	                                              subtableOffset + read_32u(data + subtableOffset + 4), subtable->type);
	goto OK;
FAIL:
	FREE(_subtable);
OK:
	return _subtable;
}

otl_subtable *caryll_read_otl_gsub_extend(font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset) {
	return _caryll_read_otl_extend(data, tableLength, subtableOffset, otl_type_gsub_unknown);
}
otl_subtable *caryll_read_otl_gpos_extend(font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset) {
	return _caryll_read_otl_extend(data, tableLength, subtableOffset, otl_type_gpos_unknown);
}
