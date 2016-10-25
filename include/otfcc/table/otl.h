#ifndef CARYLL_INCLUDE_TABLE_OTL_H
#define CARYLL_INCLUDE_TABLE_OTL_H

#include "table-common.h"
#include "otl/coverage.h"
#include "otl/classdef.h"

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
	otl_type_gpos_markToBase = 0x24,
	otl_type_gpos_markToLigature = 0x25,
	otl_type_gpos_markToMark = 0x26,
	otl_type_gpos_context = 0x27,
	otl_type_gpos_chaining = 0x28,
	otl_type_gpos_extend = 0x29
} otl_LookupType;

typedef union _otl_subtable otl_Subtable;

typedef struct {
	pos_t dx;
	pos_t dy;
	pos_t dWidth;
	pos_t dHeight;
} otl_PositionValue;

// GSUB subtable formats
typedef struct {
	otl_Coverage *from;
	otl_Coverage *to;
} subtable_gsub_single;
typedef struct {
	otl_Coverage *from;
	otl_Coverage **to;
} subtable_gsub_multi;
typedef struct {
	otl_Coverage **from;
	otl_Coverage *to;
} subtable_gsub_ligature;

typedef struct {
	tableid_t index;
	otfcc_LookupHandle lookup;
} otl_ChainLookupApplication;
typedef struct {
	tableid_t matchCount;
	tableid_t inputBegins;
	tableid_t inputEnds;
	otl_Coverage **match;
	tableid_t applyCount;
	otl_ChainLookupApplication *apply;
} otl_ChainingRule;
typedef struct {
	tableid_t rulesCount;
	otl_ChainingRule **rules;
	// these fields are for classified intermediate subtables.
	bool classified;
	otl_ClassDef *bc;
	otl_ClassDef *ic;
	otl_ClassDef *fc;
} subtable_chaining;

typedef struct {
	tableid_t matchCount;
	tableid_t inputIndex;
	otl_Coverage **match;
	otl_Coverage *to;
} subtable_gsub_reverse;

// GPOS subtable formats
typedef struct {
	otl_Coverage *coverage;
	otl_PositionValue *values;
} subtable_gpos_single;

typedef struct {
	bool present;
	pos_t x;
	pos_t y;
} otl_Anchor;

typedef struct {
	otl_Coverage *coverage;
	otl_ClassDef *first;
	otl_ClassDef *second;
	otl_PositionValue **firstValues;
	otl_PositionValue **secondValues;
} subtable_gpos_pair;

typedef struct {
	otl_Coverage *coverage;
	otl_Anchor *enter;
	otl_Anchor *exit;
} subtable_gpos_cursive;

typedef struct {
	glyphclass_t markClass;
	otl_Anchor anchor;
} otl_MarkRecord;

typedef struct {
	glyphclass_t markCount;
	otl_MarkRecord *records;
} otl_MarkArray;

typedef struct {
	otl_Coverage *marks;
	otl_Coverage *bases;
	glyphclass_t classCount;
	otl_MarkArray *markArray;
	otl_Anchor **baseArray;
} subtable_gpos_markToSingle;

typedef struct {
	glyphid_t componentCount;
	otl_Anchor **anchors;
} otl_MarkToLigatureBase;

typedef struct {
	otl_Coverage *marks;
	otl_Coverage *bases;
	glyphclass_t classCount;
	otl_MarkArray *markArray;
	otl_MarkToLigatureBase **ligArray;
} subtable_gpos_markToLigature;

typedef struct {
	otl_LookupType type;
	otl_Subtable *subtable;
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
	subtable_gpos_markToSingle gpos_markToSingle;
	subtable_gpos_markToLigature gpos_markToLigature;
	subtable_extend extend;
} otl_Subtable;

typedef struct _otl_lookup {
	sds name;
	otl_LookupType type;
	uint32_t _offset;
	uint16_t flags;
	tableid_t subtableCount;
	otl_Subtable **subtables;
} otl_Lookup;

typedef struct {
	sds name;
	tableid_t lookupCount;
	otl_Lookup **lookups;
} otl_Feature;

typedef struct {
	sds name;
	otl_Feature *requiredFeature;
	tableid_t featureCount;
	otl_Feature **features;
} otl_LanguageSystem;

typedef struct {
	sds from;
	sds to;
} otl_LookupAliasingRecord;

typedef struct {
	uint32_t languageCount;
	otl_LanguageSystem **languages;
	tableid_t featureCount;
	otl_Feature **features;
	tableid_t lookupCount;
	otl_Lookup **lookups;
} table_OTL;

otl_Subtable *table_read_otl_subtable(uint8_t *data, uint32_t tableLength, uint32_t subtableOffset,
                                      otl_LookupType lookupType);

table_OTL *table_new_otl();
void table_delete_otl(MOVE table_OTL *table);
table_OTL *table_read_otl(const caryll_Packet packet, const otfcc_Options *options, const uint32_t tag);
void table_dump_otl(const table_OTL *table, json_value *root, const otfcc_Options *options, const char *tag);
table_OTL *table_parse_otl(const json_value *root, const otfcc_Options *options, const char *tag);
caryll_Buffer *table_build_otl(const table_OTL *table, const otfcc_Options *options, const char *tag);

#endif
