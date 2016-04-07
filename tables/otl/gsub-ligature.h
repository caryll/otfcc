#ifndef CARYLL_TABLES_OTL_GSUB_LIGATURE_H
#define CARYLL_TABLES_OTL_GSUB_LIGATURE_H

#include "otl.h"

void caryll_delete_gsub_ligature(otl_lookup *lookup);
otl_subtable *caryll_read_gsub_ligature(font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset);
json_value *caryll_gsub_ligature_to_json(otl_subtable *_subtable);
otl_subtable *caryll_gsub_ligature_from_json(json_value *_subtable);
caryll_buffer *caryll_write_gsub_ligature_subtable(otl_subtable *_subtable);

#endif
