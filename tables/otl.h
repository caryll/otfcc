#ifndef CARYLL_TABLES_OTL_H
#define CARYLL_TABLES_OTL_H
#include "../support/util.h"
#include "../caryll-sfnt.h"

typedef enum {
	otl_type_unknown = 0,

	otl_type_gsub_unknown = 0x10,
	otl_type_gsub_single = 0x11,
	otl_type_gsub_multiple = 0x12,
	otl_type_gsub_alternate = 0x13,
	otl_type_gsub_ligature = 0x14,
	otl_type_gsub_context = 0x15,
	otl_type_gsub_chain = 0x16,
	otl_type_gsub_extension = 0x17,
	otl_type_gsub_reverse = 0x18,

	otl_type_gpos_unknown = 0x20,
	otl_type_gpos_single = 0x21,
	otl_type_gpos_pair = 0x22,
	otl_type_gpos_cursive = 0x23,
	otl_type_gpos_mark_to_base = 0x24,
	otl_type_gpos_mark_to_ligature = 0x25,
	otl_type_gpos_mark_to_mark = 0x26,
	otl_type_gpos_context = 0x27,
	otl_type_gpos_chain = 0x28,
	otl_type_gpos_extend = 0x29
} otl_lookup_type;

typedef struct {
	uint16_t numGlyphs;
	glyph_handle *glyphs;
} otl_coverage;

typedef struct {
	otl_coverage *from;
	otl_coverage *to;
} subtable_gsub_single;

typedef union { 
	subtable_gsub_single gsub_single;
} otl_subtable;

typedef struct {
	sds name;
	otl_lookup_type type;
	uint32_t _offset;
	uint16_t flags;
	uint16_t subtableCount;
	otl_subtable **subtables;
} otl_lookup;

typedef struct {
	sds name;
	uint16_t lookupCount;
	otl_lookup **lookups;
} otl_feature;

typedef struct {
	sds name;
	otl_feature *requiredFeature;
	uint16_t featureCount;
	otl_feature **features;
} otl_language_system;

typedef struct {
	uint32_t languageCount;
	otl_language_system **languages;
	uint16_t featureCount;
	otl_feature **features;
	uint16_t lookupCount;
	otl_lookup **lookups;
} table_otl;

table_otl *caryll_new_otl();
void caryll_delete_otl(table_otl *table);
table_otl *caryll_read_otl(caryll_packet packet, uint32_t tag);
void caryll_otl_to_json(table_otl *table, json_value *root, caryll_dump_options *dumpopts, const char *tag);
table_otl *caryll_otl_from_json(json_value *root, caryll_dump_options *dumpopts, const char *tag);
caryll_buffer *caryll_write_otl(table_otl *table);

// Coverage functions
void caryll_delete_coverage(otl_coverage *coverage);
otl_coverage *caryll_read_coverage(font_file_pointer data, uint32_t tableLength, uint32_t offset);
json_value *caryll_coverage_to_json(otl_coverage *coverage);
otl_coverage *caryll_coverage_from_json(json_value *cov);
caryll_buffer *caryll_write_coverage(otl_coverage *coverage);

#include "otl-gsub-single.h"

#endif
