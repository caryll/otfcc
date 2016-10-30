#ifndef CARYLL_TABLE_OTL_EXTEND_H
#define CARYLL_TABLE_OTL_EXTEND_H

#include "otl-private.h"

otl_Subtable *otfcc_readOtl_gsub_extend(font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset,
                                             const otfcc_Options *options);
otl_Subtable *otfcc_readOtl_gpos_extend(font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset,
                                             const otfcc_Options *options);

#endif
