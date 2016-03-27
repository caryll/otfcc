#ifndef CARYLL_TABLES_GLYF_H
#define CARYLL_TABLES_GLYF_H

#include "../support/util.h"
#include "../caryll-sfnt.h"
#include "../support/glyphorder.h"

#include "head.h"
#include "maxp.h"

enum GlyphType { SIMPLE, COMPOSITE };

typedef struct {
	float x;
	float y;
	int8_t onCurve;
} glyf_point;

typedef struct {
	uint16_t pointsCount;
	glyf_point *points;
} glyf_contour;

typedef struct {
	glyph_handle glyph;
	// transformation term
	float a;
	float b;
	float c;
	float d;
	float x;
	float y;
	bool useMyMetrics;
	bool overlap;
} glyf_reference;

typedef struct {
	sds name;
	uint16_t numberOfContours;
	uint16_t numberOfReferences;
	uint16_t instructionsLength;
	uint8_t *instructions;
	uint16_t advanceWidth;

	// NOTE: SFNT does not support mixed glyphs, but we do.
	glyf_contour *contours;
	glyf_reference *references;
} glyf_glyph;

typedef struct {
	uint16_t numberGlyphs;
	glyf_glyph **glyphs;
} table_glyf;

#define GLYF_FLAG_ON_CURVE 1
#define GLYF_FLAG_X_SHORT (1 << 1)
#define GLYF_FLAG_Y_SHORT (1 << 2)
#define GLYF_FLAG_REPEAT (1 << 3)
#define GLYF_FLAG_SAME_X (1 << 4)
#define GLYF_FLAG_SAME_Y (1 << 5)
#define GLYF_FLAG_POSITIVE_X (1 << 4)
#define GLYF_FLAG_POSITIVE_Y (1 << 5)

#define ARG_1_AND_2_ARE_WORDS (1 << 0)
#define MORE_COMPONENTS (1 << 5)
#define WE_HAVE_A_SCALE (1 << 3)
#define WE_HAVE_AN_X_AND_Y_SCALE (1 << 6)
#define WE_HAVE_A_TWO_BY_TWO (1 << 7)
#define WE_HAVE_INSTRUCTIONS (1 << 8)
#define USE_MY_METRICS (1 << 9)
#define OVERLAP_COMPOUND (1 << 10)

glyf_glyph *caryll_glyf_new();
table_glyf *caryll_read_glyf(caryll_packet packet, table_head *head, table_maxp *maxp);
void caryll_delete_glyf(table_glyf *table);
void caryll_glyf_to_json(table_glyf *table, json_value *root);
table_glyf *caryll_glyf_from_json(json_value *root, glyph_order_hash glyph_order);
#endif
