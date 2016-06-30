#ifndef CARYLL_TABLES_OTL_CLASSDEF_H
#define CARYLL_TABLES_OTL_CLASSDEF_H
#include <support/util.h>
#include <font/caryll-sfnt.h>
#include "coverage.h"

typedef struct {
	uint16_t numGlyphs;
	uint16_t maxclass;
	glyph_handle *glyphs;
	uint16_t *classes;
} otl_classdef;

void caryll_delete_classdef(otl_classdef *cd);
otl_classdef *caryll_read_classdef(font_file_pointer data, uint32_t tableLength, uint32_t offset);
otl_classdef *caryll_expand_classdef(otl_coverage *cov, otl_classdef *ocd);
json_value *caryll_classdef_to_json(otl_classdef *cd);
otl_classdef *caryll_classdef_from_json(json_value *_cd);
caryll_buffer *caryll_write_classdef(otl_classdef *cd);

#endif
