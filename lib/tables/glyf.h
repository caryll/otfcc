#ifndef CARYLL_TABLES_GLYF_H
#define CARYLL_TABLES_GLYF_H

#include <font/caryll-sfnt.h>
#include <support/glyphorder.h>
#include <support/ttinstr.h>
#include <support/util.h>

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
	glyph_handle glyph;
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
	metric_t advanceWidth;
	metric_t advanceHeight;
	pos_t verticalOrigin;

	// Outline
	// NOTE: SFNT does not support mixed glyphs, but we do.
	uint16_t numberOfContours;
	uint16_t numberOfReferences;
	glyf_Contour *contours;
	glyf_ComponentReference *references;

	// Postscript hints
	uint16_t numberOfStemH;
	uint16_t numberOfStemV;
	uint16_t numberOfHintMasks;
	uint16_t numberOfContourMasks;
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
	fd_handle fdSelect;

	// Stats
	glyf_GlyphStat stat;
} glyf_Glyph;

typedef struct {
	uint16_t numberGlyphs;
	glyf_Glyph **glyphs;
} table_glyf;

typedef enum {
	GLYF_FLAG_ON_CURVE = 1,
	GLYF_FLAG_X_SHORT = (1 << 1),
	GLYF_FLAG_Y_SHORT = (1 << 2),
	GLYF_FLAG_REPEAT = (1 << 3),
	GLYF_FLAG_SAME_X = (1 << 4),
	GLYF_FLAG_SAME_Y = (1 << 5),
	GLYF_FLAG_POSITIVE_X = (1 << 4),
	GLYF_FLAG_POSITIVE_Y = (1 << 5)
} glyf_point_flag;

typedef enum {
	ARG_1_AND_2_ARE_WORDS = (1 << 0),
	ARGS_ARE_XY_VALUES = (1 << 1),
	ROUND_XY_TO_GRID = (1 << 2),
	WE_HAVE_A_SCALE = (1 << 3),
	MORE_COMPONENTS = (1 << 5),
	WE_HAVE_AN_X_AND_Y_SCALE = (1 << 6),
	WE_HAVE_A_TWO_BY_TWO = (1 << 7),
	WE_HAVE_INSTRUCTIONS = (1 << 8),
	USE_MY_METRICS = (1 << 9),
	OVERLAP_COMPOUND = (1 << 10)
} glyf_reference_flag;

glyf_Glyph *table_new_glyf_glyph();
table_glyf *table_read_glyf(caryll_Packet packet, table_head *head, table_maxp *maxp);
void table_delete_glyf(table_glyf *table);
void table_dump_glyf(table_glyf *table, json_value *root, const caryll_Options *options);
table_glyf *table_parse_glyf(json_value *root, glyphorder_Map glyph_order, const caryll_Options *options);

typedef struct {
	caryll_buffer *glyf;
	caryll_buffer *loca;
} glyf_loca_bufpair;

glyf_loca_bufpair table_build_glyf(table_glyf *table, table_head *head, const caryll_Options *options);

#endif
