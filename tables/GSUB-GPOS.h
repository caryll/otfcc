#ifndef CARYLL_TABLES_GSUB_GPOS_H
#define CARYLL_TABLES_GSUB_GPOS_H
#include "../support/util.h"
#include "../caryll-sfnt.h"

typedef enum {
	otl_unknown = 0,

	gsub_unknown = 0x10,
	gsub_single = 0x11,
	gsub_multiple = 0x12,
	gsub_alternate = 0x13,
	gsub_ligature = 0x14,
	gsub_context = 0x15,
	gsub_chain = 0x16,
	gsub_extension = 0x17,
	gsub_reverse = 0x18,

	gpos_unknown = 0x20,
	gpos_single = 0x21,
	gpos_pair = 0x22,
	gpos_cursive = 0x23,
	gpos_mark_to_base = 0x24,
	gpos_mark_to_ligature = 0x25,
	gpos_mark_to_mark = 0x26,
	gpos_context = 0x27,
	gpos_chain = 0x28,
	gpos_extend = 0x29
} otl_lookup_type;

typedef struct { otl_lookup_type type; } otl_lookup;

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
	otl_language_system *languages;
	uint16_t featureCount;
	otl_feature *features;
	uint16_t lookupCount;
	otl_lookup *lookups;
} table_otl;

void caryll_read_GSUB_GPOS(caryll_packet packet);

#endif
