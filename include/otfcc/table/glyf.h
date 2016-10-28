#ifndef CARYLL_INCLUDE_TABLE_GLYF_H
#define CARYLL_INCLUDE_TABLE_GLYF_H

#include "table-common.h"
#include "head.h"
#include "maxp.h"

enum GlyphType { SIMPLE, COMPOSITE };

typedef struct {
	pos_t x;
	pos_t y;
	int8_t onCurve; // a mask indicates whether a point is on-curve or off-curve
	                // bit 0     : 1 for on-curve, 0 for off-curve. JSON field: "on"
	                // bit 1 - 7 : unused, set to 0
	                // in JSON, they are separated into several boolean fields.
} glyf_Point;

typedef struct {
	uint16_t pointsCount;
	glyf_Point *points;
} glyf_Contour;

// CFF stems and hint masks
typedef struct {
	pos_t position;
	pos_t width;

	uint16_t map;
} glyf_PostscriptStemDef;

typedef struct {
	uint16_t pointsBefore;
	bool maskH[0x100];
	bool maskV[0x100];
} glyf_PostscriptHintMask;

typedef struct {
	otfcc_GlyphHandle glyph;
	// transformation term
	pos_t a;
	pos_t b;
	pos_t c;
	pos_t d;
	pos_t x;
	pos_t y;
	// flags
	bool roundToGrid;
	bool useMyMetrics;
} glyf_ComponentReference;

typedef struct {
	pos_t xMin;
	pos_t xMax;
	pos_t yMin;
	pos_t yMax;
	uint16_t nestDepth;
	uint16_t nPoints;
	uint16_t nContours;
	uint16_t nCompositePoints;
	uint16_t nCompositeContours;
} glyf_GlyphStat;

typedef struct {
	sds name;

	// Metrics
	length_t advanceWidth;
	length_t advanceHeight;
	pos_t verticalOrigin;

	// Outline
	// NOTE: SFNT does not support mixed glyphs, but we do.
	shapeid_t numberOfContours;
	shapeid_t numberOfReferences;
	glyf_Contour *contours;
	glyf_ComponentReference *references;

	// Postscript hints
	shapeid_t numberOfStemH;
	shapeid_t numberOfStemV;
	shapeid_t numberOfHintMasks;
	shapeid_t numberOfContourMasks;
	glyf_PostscriptStemDef *stemH;
	glyf_PostscriptStemDef *stemV;
	glyf_PostscriptHintMask *hintMasks;
	glyf_PostscriptHintMask *contourMasks;

	// TTF instructions
	uint16_t instructionsLength;
	uint8_t *instructions;
	// TTF Screen specific
	uint8_t yPel;

	// CID FDSelect
	otfcc_FDHandle fdSelect;

	// Stats
	glyf_GlyphStat stat;
} glyf_Glyph;

typedef struct {
	glyphid_t numberGlyphs;
	glyf_Glyph **glyphs;
} table_glyf;

glyf_Glyph *otfcc_newGlyf_glyph();
void otfcc_deleteGlyf(table_glyf *table);

table_glyf *otfcc_readGlyf(const otfcc_Packet packet, const otfcc_Options *options, table_head *head, table_maxp *maxp);
void otfcc_dumpGlyf(const table_glyf *table, json_value *root, const otfcc_Options *options, bool hasVerticalMetrics,
                    bool exportFDSelect);
table_glyf *otfcc_parseGlyf(json_value *root, otfcc_GlyphOrder *glyph_order, const otfcc_Options *options);

typedef struct {
	caryll_Buffer *glyf;
	caryll_Buffer *loca;
} glyf_loca_bufpair;

glyf_loca_bufpair otfcc_buildGlyf(const table_glyf *table, table_head *head, const otfcc_Options *options);

#endif
