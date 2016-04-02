#include "otl-extend.h"

// Extended tables are special
// We will only deal with reading, and they will be flatten.
otl_subtable *caryll_read_otl_extend(font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset,
                                     otl_lookup_type lookupType) {
	otl_subtable *_subtable;
	NEW(_subtable);
	checkLength(subtableOffset + 8);
	subtable_extend *subtable = &(_subtable->extend);
	subtable->type = read_16u(data + subtableOffset + 2) +
	                 (lookupType == otl_type_gsub_extend ? otl_type_gsub_unknown : lookupType == otl_type_gpos_extend
	                                                                                   ? otl_type_gpos_unknown
	                                                                                   : otl_type_unknown);
	subtable->subtable = caryll_read_otl_subtable(data, tableLength,
	                                              subtableOffset + read_32u(data + subtableOffset + 4), subtable->type);
	goto OK;
FAIL:
	FREE(_subtable);
OK:
	return _subtable;
}
