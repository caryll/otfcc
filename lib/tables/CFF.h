#ifndef CARYLL_TABLES_cff_H
#define CARYLL_TABLES_cff_H

#include <math.h>
#include <stdarg.h>
#include <support/util.h>
#include <libcff/libcff.h>
#include <libcff/charstring-il.h>
#include <libcff/subr.h>
#include <font/caryll-sfnt.h>
#include "glyf.h"

// The result of parsing CFF table contains both CFF metadata and glyph outline.
// To simplify code and storage, the glyph outlines are stored inside glyf table.
// and the CFF table contains CFF metadata only.

typedef struct {
	double a;
	double b;
	double c;
	double d;
	double x;
	double y;
} cff_FontMatrix;

typedef struct {
	arity_t blueValuesCount;
	double *blueValues;
	arity_t otherBluesCount;
	double *otherBlues;
	arity_t familyBluesCount;
	double *familyBlues;
	arity_t familyOtherBluesCount;
	double *familyOtherBlues;
	double blueScale;
	double blueShift;
	double blueFuzz;
	double stdHW;
	double stdVW;
	arity_t stemSnapHCount;
	double *stemSnapH;
	arity_t stemSnapVCount;
	double *stemSnapV;
	bool forceBold;
	uint32_t languageGroup;
	double expansionFactor;
	double initialRandomSeed;
	double defaultWidthX;
	double nominalWidthX;
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
	double italicAngle;
	double underlinePosition;
	double underlineThickness;
	double fontBBoxTop;
	double fontBBoxBottom;
	double fontBBoxLeft;
	double fontBBoxRight;
	double strokeWidth;
	cff_PrivateDict *privateDict;
	cff_FontMatrix *fontMatrix;

	// CID-only operators
	sds cidRegistry;
	sds cidOrdering;
	uint32_t cidSupplement;
	double cidFontVersion;
	double cidFontRevision;
	uint32_t cidCount;
	uint32_t UIDBase;
	// CID FDArray
	tableid_t fdArrayCount;
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
