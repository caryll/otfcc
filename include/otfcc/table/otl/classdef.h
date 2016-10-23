#ifndef CARYLL_INCLUDE_TABLE_OTL_CLASSDEF_H
#define CARYLL_INCLUDE_TABLE_OTL_CLASSDEF_H
#include "../table-common.h"
#include "coverage.h"

typedef struct {
	glyphid_t numGlyphs;
	glyphclass_t maxclass;
	otfcc_GlyphHandle *glyphs;
	glyphclass_t *classes;
} otl_ClassDef;

void otl_delete_ClassDef(otl_ClassDef *cd);
otl_ClassDef *otl_read_ClassDef(const uint8_t *data, uint32_t tableLength, uint32_t offset);
otl_ClassDef *otl_expand_ClassDef(otl_Coverage *cov, otl_ClassDef *ocd);
json_value *otl_dump_ClassDef(const otl_ClassDef *cd);
otl_ClassDef *otl_parse_ClassDef(const json_value *_cd);
caryll_Buffer *otl_build_ClassDef(const otl_ClassDef *cd);

#endif
