#ifndef CARYLL_TABLES_OTL_GSUB_LIGATURE_H
#define CARYLL_TABLES_OTL_GSUB_LIGATURE_H

#include "otl.h"

void otl_delete_gsub_ligature(otl_Subtable *subtable);
otl_Subtable *otl_read_gsub_ligature(font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset);
json_value *otl_gsub_dump_ligature(otl_Subtable *_subtable);
otl_Subtable *otl_gsub_parse_ligature(json_value *_subtable);
caryll_buffer *caryll_build_gsub_ligature_subtable(otl_Subtable *_subtable);

#endif
