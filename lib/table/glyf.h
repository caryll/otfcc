#ifndef CARYLL_TABLE_GLYF_H
#define CARYLL_TABLE_GLYF_H

#include "otfcc/table/glyf.h"

glyf_Glyph *otfcc_newGlyf_glyph();
void otfcc_initGlyfContour(glyf_Contour *contour);
void otfcc_deleteGlyf(table_glyf *table);
table_glyf *otfcc_readGlyf(const otfcc_Packet packet, const otfcc_Options *options, table_head *head, table_maxp *maxp);
void otfcc_dumpGlyf(const table_glyf *table, json_value *root, const otfcc_Options *options, bool hasVerticalMetrics,
                    bool exportFDSelect);
table_glyf *otfcc_parseGlyf(const json_value *root, otfcc_GlyphOrder *glyph_order, const otfcc_Options *options);

typedef struct {
	caryll_Buffer *glyf;
	caryll_Buffer *loca;
} table_GlyfAndLocaBuffers;

table_GlyfAndLocaBuffers otfcc_buildGlyf(const table_glyf *table, table_head *head, const otfcc_Options *options);

#endif
