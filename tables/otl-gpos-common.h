#ifndef CARYLL_TABLES_OTL_GPOS_COMMON_H
#define CARYLL_TABLES_OTL_GPOS_COMMON_H

#include "otl.h"

// anchors
otl_anchor otl_read_anchor(font_file_pointer data, uint32_t tableLength, uint32_t offset);

// mark arrays
void otl_delete_mark_array(otl_mark_array *array);
otl_mark_array *otl_read_mark_array(font_file_pointer data, uint32_t tableLength, uint32_t offset);

#endif
