#ifndef CARYLL_TABLES_OTL_GPOS_PAIR_H
#define CARYLL_TABLES_OTL_GPOS_PAIR_H

#include "otl.h"
void caryll_delete_gpos_pair(otl_subtable *_subtable);
otl_subtable *caryll_read_gpos_pair(font_file_pointer data, uint32_t tableLength, uint32_t offset);
json_value *caryll_gpos_pair_to_json(otl_subtable *_subtable);
otl_subtable *caryll_gpos_pair_from_json(json_value *_subtable);
caryll_buffer *caryll_write_gpos_pair(otl_subtable *_subtable);
#endif
