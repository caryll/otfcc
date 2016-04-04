#ifndef CARYLL_TABLES_OTL_CHAINING_H
#define CARYLL_TABLES_OTL_CHAINING_H

#include "otl.h"

// This file supports four formats
// GSUB contextual
// GSUB chaining
// GPOS contextual
// GPOS chaining

void caryll_delete_chaining(otl_lookup *lookup) ;
void delete_otl_chaining_subtable(otl_subtable *_subtable);
otl_subtable *caryll_read_chaining(font_file_pointer data, uint32_t tableLength, uint32_t offset);
otl_subtable *caryll_read_contextual(font_file_pointer data, uint32_t tableLength, uint32_t offset);
json_value *caryll_chaining_to_json(otl_subtable *_subtable);
otl_lookup *caryll_chaining_from_json(json_value *_lookup, char *_type);
caryll_buffer *caryll_write_chaining(otl_subtable *_subtable);

#endif
