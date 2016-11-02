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
typedef caryll_Vector(glyf_Point) glyf_Contour;
typedef caryll_Vector(glyf_Contour) glyf_ContourList;

// CFF stems and hint masks
typedef struct {
	pos_t position;
	pos_t width;
	uint16_t map;
} glyf_PostscriptStemDef;
typedef caryll_Vector(glyf_PostscriptStemDef) glyf_StemDefList;

typedef struct {
	uint16_t pointsBefore;
	uint16_t contoursBefore;
	bool maskH[0x100];
	bool maskV[0x100];
} glyf_PostscriptHintMask;
typedef caryll_Vector(glyf_PostscriptHintMask) glyf_MaskList;

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
typedef caryll_Vector(glyf_ComponentReference) glyf_ReferenceList;

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
	OWNING glyf_ContourList contours;
	OWNING glyf_ReferenceList references;

	// Postscript hints
	OWNING glyf_StemDefList stemH;
	OWNING glyf_StemDefList stemV;
	OWNING glyf_MaskList hintMasks;
	OWNING glyf_MaskList contourMasks;

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

#endif
