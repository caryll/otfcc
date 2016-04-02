#ifndef CARYLL_TABLES_OTL_EXTEND_H
#define CARYLL_TABLES_OTL_EXTEND_H

#include "otl.h"
otl_subtable *caryll_read_otl_extend(font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset,
                                     otl_lookup_type lookupType);
#endif
