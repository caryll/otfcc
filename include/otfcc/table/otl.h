#ifndef CARYLL_INCLUDE_TABLE_OTL_H
#define CARYLL_INCLUDE_TABLE_OTL_H

#include "caryll/vector.h"
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
	otfcc_GlyphHandle from;
	otfcc_GlyphHandle to;
} subtable_gsub_single_entry;
typedef caryll_Vector(subtable_gsub_single_entry) subtable_gsub_single;
/*typedef struct {
    OWNING otl_Coverage *from;
    OWNING otl_Coverage *to;
} subtable_gsub_single;
*/
typedef struct {
	OWNING otl_Coverage *from;
	OWNING otl_Coverage **to;
} subtable_gsub_multi;
typedef struct {
	OWNING otl_Coverage **from;
	OWNING otl_Coverage *to;
} subtable_gsub_ligature;

typedef enum {
	otl_chaining_canonical = 0, // The canonical form of chaining contextual substitution, one rule per subtable.
	otl_chaining_poly = 1,      // The multi-rule form, right after reading OTF. N rule per subtable.
	otl_chaining_classified = 2 // The classified intermediate form, for building TTF with compression.
	                            // N rules, has classdefs, and coverage GID interpreted as class number.
} otl_chaining_type;

typedef struct {
	tableid_t index;
	otfcc_LookupHandle lookup;
} otl_ChainLookupApplication;
typedef struct {
	tableid_t matchCount;
	tableid_t inputBegins;
	tableid_t inputEnds;
	OWNING otl_Coverage **match;
	tableid_t applyCount;
	OWNING otl_ChainLookupApplication *apply;
} otl_ChainingRule;
typedef struct {
	otl_chaining_type type;
	union {
		otl_ChainingRule rule; // for type = otl_chaining_canonical
		struct {               // for type = otl_chaining_poly or otl_chaining_classified
			tableid_t rulesCount;
			OWNING otl_ChainingRule **rules;
			OWNING otl_ClassDef *bc;
			OWNING otl_ClassDef *ic;
			OWNING otl_ClassDef *fc;
		};
	};
} subtable_chaining;

typedef struct {
	tableid_t matchCount;
	tableid_t inputIndex;
	OWNING otl_Coverage **match;
	OWNING otl_Coverage *to;
} subtable_gsub_reverse;

// GPOS subtable formats
typedef struct {
	OWNING otl_Coverage *coverage;
	OWNING otl_PositionValue *values;
} subtable_gpos_single;

typedef struct {
	bool present;
	pos_t x;
	pos_t y;
} otl_Anchor;

typedef struct {
	OWNING otl_ClassDef *first;
	OWNING otl_ClassDef *second;
	OWNING otl_PositionValue **firstValues;
	OWNING otl_PositionValue **secondValues;
} subtable_gpos_pair;

typedef struct {
	OWNING otl_Coverage *coverage;
	OWNING otl_Anchor *enter;
	OWNING otl_Anchor *exit;
} subtable_gpos_cursive;

typedef struct {
	glyphclass_t markClass;
	otl_Anchor anchor;
} otl_MarkRecord;

typedef struct {
	glyphclass_t markCount;
	OWNING otl_MarkRecord *records;
} otl_MarkArray;

typedef struct {
	OWNING otl_Coverage *marks;
	OWNING otl_Coverage *bases;
	glyphclass_t classCount;
	OWNING otl_MarkArray *markArray;
	OWNING otl_Anchor **baseArray;
} subtable_gpos_markToSingle;

typedef struct {
	glyphid_t componentCount;
	OWNING otl_Anchor **anchors;
} otl_MarkToLigatureBase;

typedef struct {
	OWNING otl_Coverage *marks;
	OWNING otl_Coverage *bases;
	glyphclass_t classCount;
	OWNING otl_MarkArray *markArray;
	OWNING otl_MarkToLigatureBase **ligArray;
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
	OWNING otl_Subtable **subtables;
} otl_Lookup;

typedef struct {
	sds name;
	tableid_t lookupCount;
	OWNING otl_Lookup **lookups;
} otl_Feature;

typedef struct {
	sds name;
	OWNING otl_Feature *requiredFeature;
	tableid_t featureCount;
	OWNING otl_Feature **features;
} otl_LanguageSystem;

typedef struct {
	sds from;
	sds to;
} otl_LookupAliasingRecord;

typedef struct {
	uint32_t languageCount;
	OWNING otl_LanguageSystem **languages;
	tableid_t featureCount;
	OWNING otl_Feature **features;
	tableid_t lookupCount;
	OWNING otl_Lookup **lookups;
} table_OTL;

#endif
