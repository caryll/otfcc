#ifndef CARYLL_TABLES_OTL_GPOS_MARK_TO_BASE_H
#define CARYLL_TABLES_OTL_GPOS_MARK_TO_BASE_H

#include "otl.h"
void caryll_delete_gpos_mark_to_base(otl_lookup *lookup);
void caryll_read_gpos_mark_to_base(font_file_pointer data, uint32_t tableLength, otl_lookup *lookup);
void caryll_gpos_mark_to_base_to_json(otl_lookup *lookup, json_value *dump);
#endif
