#ifndef CARYLL_TABLES_cff_H
#define CARYLL_TABLES_cff_H

#include <math.h>
#include <stdarg.h>
#include <support/util.h>
#include <libcff/libcff.h>
#include <libcff/charstring-il.h>
#include <font/caryll-sfnt.h>
#include "glyf.h"

// The result of parsing CFF table contains both CFF metadata and glyph outline.
// To simplify code and storage, the glyph outlines are stored inside glyf table.
// and the CFF table contains CFF metadata only.

typedef struct {
	float a;
	float b;
	float c;
	float d;
	float x;
	float y;
} cff_FontMatrix;

typedef struct {
	uint16_t blueValuesCount;
	float *blueValues;
	uint16_t otherBluesCount;
	float *otherBlues;
	uint16_t familyBluesCount;
	float *familyBlues;
	uint16_t familyOtherBluesCount;
	float *familyOtherBlues;
	float blueScale;
	float blueShift;
	float blueFuzz;
	float stdHW;
	float stdVW;
	uint16_t stemSnapHCount;
	float *stemSnapH;
	uint16_t stemSnapVCount;
	float *stemSnapV;
	bool forceBold;
	uint32_t languageGroup;
	float expansionFactor;
	float initialRandomSeed;
	float defaultWidthX;
	float nominalWidthX;
} cff_PrivateDict;

typedef struct _table_CFF table_CFF;

struct _table_CFF {
	// Name
	sds fontName;

	// General properties
	bool isCID;
	sds version;
	sds notice;
	sds copyright;
	sds fullName;
	sds familyName;
	sds weight;
	bool isFixedPitch;
	float italicAngle;
	float underlinePosition;
	float underlineThickness;
	float fontBBoxTop;
	float fontBBoxBottom;
	float fontBBoxLeft;
	float fontBBoxRight;
	float strokeWidth;
	cff_PrivateDict *privateDict;
	cff_FontMatrix *fontMatrix;

	// CID-only operators
	sds cidRegistry;
	sds cidOrdering;
	uint32_t cidSupplement;
	float cidFontVersion;
	float cidFontRevision;
	uint32_t cidCount;
	uint32_t UIDBase;
	// CID FDArray
	uint16_t fdArrayCount;
	table_CFF **fdArray;
};

// CFF and glyf
typedef struct {
	table_CFF *meta;
	table_glyf *glyphs;
} table_CFFAndGlyf;

table_CFF *table_new_CFF();
void table_delete_CFF(table_CFF *table);
table_CFFAndGlyf table_read_cff_and_glyf(caryll_Packet packet);
void table_dump_cff(table_CFF *table, json_value *root, const caryll_Options *options);
table_CFF *table_parse_cff(json_value *root, const caryll_Options *options);
caryll_buffer *table_build_CFF(table_CFFAndGlyf cffAndGlyf, const caryll_Options *options);

#endif
