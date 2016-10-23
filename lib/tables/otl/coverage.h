#ifndef CARYLL_TABLES_OTL_COVERAGE_H
#define CARYLL_TABLES_OTL_COVERAGE_H
#include "support/util.h"
#include "font/caryll-sfnt.h"

typedef struct {
	glyphid_t numGlyphs;
	glyph_handle *glyphs;
} otl_Coverage;

void otl_delete_Coverage(MOVE otl_Coverage *coverage);

otl_Coverage *otl_read_Coverage(const font_file_pointer data, uint32_t tableLength, uint32_t offset);
json_value *otl_dump_Coverage(const otl_Coverage *coverage);
otl_Coverage *otl_parse_Coverage(const json_value *cov);
caryll_Buffer *otl_build_Coverage(const otl_Coverage *coverage);

#endif
