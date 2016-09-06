#ifndef CARYLL_TABLES_OTL_CHAINING_H
#define CARYLL_TABLES_OTL_CHAINING_H

#include "otl.h"

// This file supports four formats
// GSUB contextual
// GSUB chaining
// GPOS contextual
// GPOS chaining

void otl_delete_chaining(otl_Subtable *_subtable);
otl_Subtable *otl_read_chaining(font_file_pointer data, uint32_t tableLength, uint32_t offset);
otl_Subtable *otl_read_contextual(font_file_pointer data, uint32_t tableLength, uint32_t offset);
json_value *otl_dump_chaining(otl_Subtable *_subtable);
otl_Subtable *otl_parse_chaining(json_value *_subtable);
caryll_buffer *caryll_build_chaining(otl_Subtable *_subtable);

#endif
