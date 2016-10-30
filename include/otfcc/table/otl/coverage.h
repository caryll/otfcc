#ifndef CARYLL_INCLUDE_TABLE_OTL_COVERAGE_H
#define CARYLL_INCLUDE_TABLE_OTL_COVERAGE_H
#include "../table-common.h"

typedef struct {
	glyphid_t numGlyphs;
	uint32_t capacity;
	otfcc_GlyphHandle *glyphs;
} otl_Coverage;

otl_Coverage *otl_new_Coverage();
void otl_clear_Coverage(otl_Coverage *coverage, uint32_t n);
void otl_delete_Coverage(MOVE otl_Coverage *coverage);

otl_Coverage *otl_read_Coverage(const uint8_t *data, uint32_t tableLength, uint32_t offset);
json_value *otl_dump_Coverage(const otl_Coverage *coverage);
otl_Coverage *otl_parse_Coverage(const json_value *cov);
caryll_Buffer *otl_build_Coverage(const otl_Coverage *coverage);
void fontop_shrinkCoverage(otl_Coverage *coverage, bool dosort);

void otl_Coverage_push(otl_Coverage *coverage, MOVE otfcc_GlyphHandle h);

#endif
