#ifndef CARYLL_TABLES_OTL_COVERAGE_H
#define CARYLL_TABLES_OTL_COVERAGE_H
#include "../../support/util.h"
#include "../../caryll-sfnt.h"

typedef struct {
	uint16_t numGlyphs;
	glyph_handle *glyphs;
} otl_coverage;

void caryll_delete_coverage(otl_coverage *coverage);
otl_coverage *caryll_read_coverage(font_file_pointer data, uint32_t tableLength, uint32_t offset);
json_value *caryll_coverage_to_json(otl_coverage *coverage);
otl_coverage *caryll_coverage_from_json(json_value *cov);
caryll_buffer *caryll_write_coverage(otl_coverage *coverage);

#endif
