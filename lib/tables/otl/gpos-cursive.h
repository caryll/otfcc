#ifndef CARYLL_TABLES_OTL_GPOS_CURSIVE_H
#define CARYLL_TABLES_OTL_GPOS_CURSIVE_H

#include "otl.h"

void otl_delete_gpos_cursive(otl_Subtable *subtable);
otl_Subtable *otl_read_gpos_cursive(const font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset);
json_value *otl_gpos_dump_cursive(const otl_Subtable *_subtable);
otl_Subtable *otl_gpos_parse_cursive(const json_value *_subtable);
caryll_buffer *caryll_build_gpos_cursive(const otl_Subtable *_subtable);

#endif
