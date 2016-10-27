#ifndef CARYLL_TABLE_OTL_CHAINING_H
#define CARYLL_TABLE_OTL_CHAINING_H

#include "otl-private.h"

// This file supports four formats
// GSUB contextual
// GSUB chaining
// GPOS contextual
// GPOS chaining

void otl_delete_chaining(otl_Subtable *_subtable);
otl_Subtable *otl_read_chaining(const font_file_pointer data, uint32_t tableLength, uint32_t offset);
otl_Subtable *otl_read_contextual(const font_file_pointer data, uint32_t tableLength, uint32_t offset);
json_value *otl_dump_chaining(const otl_Subtable *_subtable);
otl_Subtable *otl_parse_chaining(const json_value *_subtable, const otfcc_Options *options);
caryll_Buffer *caryll_build_chaining(const otl_Subtable *_subtable);

tableid_t caryll_classifiedBuildChaining(const otl_Lookup *lookup, OUT caryll_Buffer ***subtableBuffers,
                                         MODIFY size_t *lastOffset);

#endif
