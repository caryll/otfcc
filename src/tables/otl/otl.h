#ifndef CARYLL_TABLES_OTL_H
#define CARYLL_TABLES_OTL_H
#include "../../support/util.h"
#include "../../caryll-sfnt.h"

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

#include "coverage.h"
#include "classdef.h"

typedef struct {
	int16_t dx;
	int16_t dy;
	int16_t dWidth;
	int16_t dHeight;
} otl_position_value;

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
	otl_coverage **from;
	otl_coverage *to;
} subtable_gsub_ligature;

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

typedef struct {
	uint16_t matchCount;
	uint16_t inputIndex;
	otl_coverage **match;
	otl_coverage *to;
} subtable_gsub_reverse;

// GPOS subtable formats
typedef struct {
	otl_coverage *coverage;
	otl_position_value *values;
} subtable_gpos_single;

typedef struct {
	bool present;
	int16_t x;
	int16_t y;
} otl_anchor;

typedef struct {
	otl_coverage *coverage;
	otl_classdef *first;
	otl_classdef *second;
	otl_position_value **firstValues;
	otl_position_value **secondValues;
} subtable_gpos_pair;

typedef struct {
	otl_coverage *coverage;
	otl_anchor *enter;
	otl_anchor *exit;
} subtable_gpos_cursive;

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
	otl_mark_array *markArray;
	otl_anchor **baseArray;
} subtable_gpos_mark_to_single;

typedef struct {
	uint16_t componentCount;
	otl_anchor **anchors;
} mark_to_ligature_base;

typedef struct {
	otl_coverage *marks;
	otl_coverage *bases;
	uint16_t classCount;
	otl_mark_array *markArray;
	mark_to_ligature_base **ligArray;
} subtable_gpos_mark_to_ligature;

typedef struct {
	uint16_t type;
	otl_subtable *subtable;
} subtable_extend;

typedef union _otl_subtable {
	subtable_gsub_single gsub_single;
	subtable_gsub_multi gsub_multi;
	subtable_gsub_ligature gsub_ligature;
	subtable_chaining chaining;
	subtable_gsub_reverse gsub_reverse;
	subtable_gpos_single gpos_single;
	subtable_gpos_pair gpos_pair;
	subtable_gpos_cursive gpos_cursive;
	subtable_gpos_mark_to_single gpos_mark_to_single;
	subtable_gpos_mark_to_ligature gpos_mark_to_ligature;
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

otl_subtable *caryll_read_otl_subtable(font_file_pointer data, uint32_t tableLength,
                                       uint32_t subtableOffset, otl_lookup_type lookupType);

table_otl *caryll_new_otl();
void caryll_delete_otl(table_otl *table);
table_otl *caryll_read_otl(caryll_packet packet, uint32_t tag);
void caryll_otl_to_json(table_otl *table, json_value *root, caryll_dump_options *dumpopts,
                        const char *tag);
table_otl *caryll_otl_from_json(json_value *root, caryll_dump_options *dumpopts, const char *tag);
caryll_buffer *caryll_write_otl(table_otl *table);

#include "gsub-single.h"
#include "gsub-multi.h"
#include "gsub-ligature.h"
#include "gsub-reverse.h"
#include "gpos-single.h"
#include "gpos-pair.h"
#include "gpos-cursive.h"
#include "gpos-mark-to-single.h"
#include "gpos-mark-to-ligature.h"
#include "chaining.h"
#include "extend.h"

#define checkLength(offset)                                                                        \
	if (tableLength < offset) { goto FAIL; }

#endif
