#ifndef CARYLL_TABLES_OTL_GSUB_SINGLE_H
#define CARYLL_TABLES_OTL_GSUB_SINGLE_H

#include "otl.h"

void caryll_delete_gsub_single(otl_lookup *lookup);
otl_subtable *caryll_read_gsub_single(font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset);
void caryll_gsub_single_to_json(otl_lookup *lookup, json_value *dump);
otl_lookup *caryll_gsub_single_from_json(json_value *lookup);
caryll_buffer *caryll_write_gsub_single(otl_lookup *lookup);

caryll_buffer *caryll_write_gsub_single_subtable(otl_subtable *_subtable);

#endif
