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
	otl_type_gsub_chaining = 0x16,
	otl_type_gsub_extend = 0x17,
	otl_type_gsub_reverse = 0x18,

	otl_type_gpos_unknown = 0x20,
	otl_type_gpos_single = 0x21,
	otl_type_gpos_pair = 0x22,
	otl_type_gpos_cursive = 0x23,
	otl_type_gpos_mark_to_base = 0x24,
	otl_type_gpos_mark_to_ligature = 0x25,
	otl_type_gpos_mark_to_mark = 0x26,
	otl_type_gpos_context = 0x27,
	otl_type_gpos_chaining = 0x28,
	otl_type_gpos_extend = 0x29
} otl_lookup_type;

typedef union _otl_subtable otl_subtable;

typedef struct {
	uint16_t numGlyphs;
	glyph_handle *glyphs;
} otl_coverage;

typedef struct {
	uint16_t numGlyphs;
	uint16_t maxclass;
	glyph_handle *glyphs;
	uint16_t *classes;
} otl_classdef;

// GSUB subtable formats
typedef struct {
	otl_coverage *from;
	otl_coverage *to;
} subtable_gsub_single;

typedef struct {
	otl_coverage *from;
	otl_coverage **to;
} subtable_gsub_multi;

typedef struct {
	uint16_t index;
	uint16_t lookupIndex;
	sds lookupName;
} otl_contextual_application;
typedef struct {
	uint16_t matchCount;
	uint16_t inputBegins;
	uint16_t inputEnds;
	otl_coverage **match;
	uint16_t applyCount;
	otl_contextual_application *apply;
} otl_chaining_rule;
typedef struct {
	uint16_t rulesCount;
	otl_chaining_rule **rules;
	
	bool classified;
	otl_classdef *bc;
	otl_classdef *ic;
	otl_classdef *fc;
} subtable_chaining;

// GPOS subtable formats
typedef struct {
	bool present;
	int16_t x;
	int16_t y;
} otl_anchor;

typedef struct {
	uint16_t markClass;
	otl_anchor anchor;
} otl_mark_record;

typedef struct {
	uint16_t markCount;
	otl_mark_record *records;
} otl_mark_array;

typedef struct {
	otl_coverage *marks;
	otl_coverage *bases;
	uint16_t classCount;
	otl_anchor **baseArray;
	otl_mark_array *markArray;
} subtable_gpos_mark_to_single;

typedef struct {
	uint16_t type;
	otl_subtable *subtable;
} subtable_extend;

typedef union _otl_subtable {
	subtable_gsub_single gsub_single;
	subtable_gsub_multi gsub_multi;
	subtable_chaining chaining;
	subtable_gpos_mark_to_single gpos_mark_to_single;
	subtable_extend extend;
} otl_subtable;

typedef struct _otl_lookup {
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

otl_subtable *caryll_read_otl_subtable(font_file_pointer data, uint32_t tableLength, uint32_t subtableOffset,
                                       otl_lookup_type lookupType);

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

// Classdef functions
otl_classdef *caryll_raad_classdef(font_file_pointer data, uint32_t tableLength, uint32_t offset);
caryll_buffer *caryll_write_classdef(otl_classdef *cd);

#include "otl-gsub-single.h"
#include "otl-gsub-multi.h"
#include "otl-gpos-mark-to-single.h"
#include "otl-chaining.h"
#include "otl-extend.h"

#define checkLength(offset)                                                                                            \
	if (tableLength < offset) { goto FAIL; }

#endif
