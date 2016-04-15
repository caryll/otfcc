#ifndef CARYLL_TABLES_OTL_GSUB_REVERSE_H
#define CARYLL_TABLES_OTL_GSUB_REVERSE_H

#include "otl.h"

void caryll_delete_gsub_reverse(otl_lookup *lookup);
otl_subtable *caryll_read_gsub_reverse(font_file_pointer data, uint32_t tableLength, uint32_t offset);
json_value *caryll_gsub_reverse_to_json(otl_subtable *_subtable);
otl_subtable *caryll_gsub_reverse_from_json(json_value *_subtable);
caryll_buffer *caryll_write_gsub_reverse(otl_subtable *_subtable);

#endif
