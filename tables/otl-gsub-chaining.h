#ifndef CARYLL_TABLES_OTL_CHAINING_H
#define CARYLL_TABLES_OTL_CHAINING_H

#include "otl.h"
otl_subtable *caryll_read_gsub_chaining(font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset);
json_value *caryll_gsub_chaining_to_json(otl_subtable *_subtable);

#endif
