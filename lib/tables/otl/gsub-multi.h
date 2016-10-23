#ifndef CARYLL_TABLES_OTL_GSUB_MULTI_H
#define CARYLL_TABLES_OTL_GSUB_MULTI_H

#include "otl.h"

void otl_delete_gsub_multi(otl_Subtable *subtable);
otl_Subtable *otl_read_gsub_multi(const font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset);
json_value *otl_gsub_dump_multi(const otl_Subtable *_subtable);
otl_Subtable *otl_gsub_parse_multi(const json_value *_subtable);
caryll_Buffer *caryll_build_gsub_multi_subtable(const otl_Subtable *_subtable);

#endif
