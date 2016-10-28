#ifndef CARYLL_TABLE_OTL_GSUB_REVERSE_H
#define CARYLL_TABLE_OTL_GSUB_REVERSE_H

#include "otl-private.h"

void otl_delete_gsub_reverse(otl_Subtable *_subtable);
otl_Subtable *otl_read_gsub_reverse(const font_file_pointer data, uint32_t tableLength, uint32_t offset,
                                    const otfcc_Options *options);
json_value *otl_gsub_dump_reverse(const otl_Subtable *_subtable);
otl_Subtable *otl_gsub_parse_reverse(const json_value *_subtable, const otfcc_Options *options);
caryll_Buffer *caryll_build_gsub_reverse(const otl_Subtable *_subtable);

#endif
