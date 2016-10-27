#ifndef CARYLL_TABLE_OTL_GSUB_SINGLE_H
#define CARYLL_TABLE_OTL_GSUB_SINGLE_H

#include "otl-private.h"

void otl_delete_gsub_single(otl_Subtable *subtable);
otl_Subtable *otl_read_gsub_single(const font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset);
json_value *otl_gsub_dump_single(const otl_Subtable *_subtable);
otl_Subtable *otl_gsub_parse_single(const json_value *_subtable, const otfcc_Options *options);
caryll_Buffer *caryll_build_gsub_single_subtable(const otl_Subtable *_subtable);

#endif
