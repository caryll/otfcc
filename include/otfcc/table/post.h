#ifndef CARYLL_INCLUDE_TABLE_POST_H
#define CARYLL_INCLUDE_TABLE_POST_H

#include "table-common.h"

typedef struct {
	// PostScript information
	f16dot16 version;
	f16dot16 italicAngle;
	int16_t underlinePosition;
	int16_t underlineThickness;
	uint32_t isFixedPitch;
	uint32_t minMemType42;
	uint32_t maxMemType42;
	uint32_t minMemType1;
	uint32_t maxMemType1;
	otfcc_GlyphOrder *post_name_map;
} table_post;

table_post *otfcc_newTablepost();
void otfcc_deleteTablepost(MOVE table_post *table);
table_post *otfcc_readTablepost(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpTablepost(const table_post *table, json_value *root, const otfcc_Options *options);
table_post *otfcc_parseTablepost(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildTablepost(const table_post *post, otfcc_GlyphOrder *glyphorder, const otfcc_Options *options);

#endif
