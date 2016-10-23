#ifndef CARYLL_TABLES_OTL_CLASSDEF_H
#define CARYLL_TABLES_OTL_CLASSDEF_H
#include "support/util.h"
#include "font/caryll-sfnt.h"
#include "coverage.h"

typedef struct {
	glyphid_t numGlyphs;
	glyphclass_t maxclass;
	glyph_handle *glyphs;
	glyphclass_t *classes;
} otl_ClassDef;

void otl_delete_ClassDef(otl_ClassDef *cd);
otl_ClassDef *otl_read_ClassDef(const font_file_pointer data, uint32_t tableLength, uint32_t offset);
otl_ClassDef *otl_expand_ClassDef(otl_Coverage *cov, otl_ClassDef *ocd);
json_value *otl_dump_ClassDef(const otl_ClassDef *cd);
otl_ClassDef *otl_parse_ClassDef(const json_value *_cd);
caryll_buffer *otl_build_ClassDef(const otl_ClassDef *cd);

#endif
