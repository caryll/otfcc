#ifndef CARYLL_TABLES_OTL_GPOS_MARK_TO_SINGLE_H
#define CARYLL_TABLES_OTL_GPOS_MARK_TO_SINGLE_H

#include "otl.h"
void caryll_delete_gpos_mark_to_single(otl_lookup *lookup);
otl_subtable *caryll_read_gpos_mark_to_single(font_file_pointer data, uint32_t tableLength,
                                              uint32_t subtableOffset);
json_value *caryll_gpos_mark_to_single_to_json(otl_subtable *st);
otl_subtable *caryll_gpos_mark_to_single_from_json(json_value *_subtable);
caryll_buffer *caryll_write_gpos_mark_to_single(otl_subtable *_subtable);
#endif
