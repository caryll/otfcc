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
	float x;
	float y;
	int8_t onCurve; // a mask indicates whether a point is on-curve or off-curve
	                // bit 0     : 1 for on-curve, 0 for off-curve. JSON field: "on"
	                // bit 1 - 7 : unused, set to 0
	                // in JSON, they are separated into several boolean fields.
} glyf_point;

typedef struct {
	uint16_t pointsCount;
	glyf_point *points;
} glyf_contour;

// CFF stems and hint masks
typedef struct {
	float position;
	float width;
} glyf_postscript_hint_stemdef;

typedef struct {
	uint16_t pointsBefore;
	bool maskH[0x100];
	bool maskV[0x100];
} glyf_postscript_hint_mask;

typedef struct {
	glyph_handle glyph;
	// transformation term
	float a;
	float b;
	float c;
	float d;
	float x;
	float y;
	// flags
	bool roundToGrid;
	bool useMyMetrics;
} glyf_reference;

typedef struct {
	float xMin;
	float xMax;
	float yMin;
	float yMax;
	uint16_t nestDepth;
	uint16_t nPoints;
	uint16_t nContours;
	uint16_t nCompositePoints;
	uint16_t nCompositeContours;
} glyf_glyph_stat;

typedef struct {
	sds name;

	// Metrics
	uint16_t advanceWidth;
	uint16_t advanceHeight;
	float verticalOrigin;

	// Outline
	// NOTE: SFNT does not support mixed glyphs, but we do.
	uint16_t numberOfContours;
	uint16_t numberOfReferences;
	glyf_contour *contours;
	glyf_reference *references;

	// Postscript hints
	uint16_t numberOfStemH;
	uint16_t numberOfStemV;
	uint16_t numberOfHintMasks;
	uint16_t numberOfContourMasks;
	glyf_postscript_hint_stemdef *stemH;
	glyf_postscript_hint_stemdef *stemV;
	glyf_postscript_hint_mask *hintMasks;
	glyf_postscript_hint_mask *contourMasks;

	// TTF instructions
	uint16_t instructionsLength;
	uint8_t *instructions;

	// CID FDSelect
	uint16_t fdSelectIndex;

	// Stats
	glyf_glyph_stat stat;
} glyf_glyph;

typedef struct {
	uint16_t numberGlyphs;
	glyf_glyph **glyphs;
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

glyf_glyph *caryll_new_glyf_glyph();
table_glyf *caryll_read_glyf(caryll_packet packet, table_head *head, table_maxp *maxp);
void caryll_delete_glyf(table_glyf *table);
void caryll_glyf_to_json(table_glyf *table, json_value *root, caryll_dump_options *dumpopts);
table_glyf *caryll_glyf_from_json(json_value *root, glyph_order_hash glyph_order,
                                  caryll_dump_options *dumpopts);
void caryll_write_glyf(table_glyf *table, table_head *head, caryll_buffer *bufglyf,
                       caryll_buffer *bufloca);

#endif
