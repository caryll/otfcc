#ifndef CARYLL_TABLE_OTL_GPOS_SINGLE_H
#define CARYLL_TABLE_OTL_GPOS_SINGLE_H

#include "otl-private.h"

void otl_delete_gpos_single(otl_Subtable *subtable);
otl_Subtable *otl_read_gpos_single(const font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset,
                                   const otfcc_Options *options);
json_value *otl_gpos_dump_single(const otl_Subtable *_subtable);
otl_Subtable *otl_gpos_parse_single(const json_value *_subtable, const otfcc_Options *options);
caryll_Buffer *otfcc_build_gpos_single(const otl_Subtable *_subtable);

#endif
