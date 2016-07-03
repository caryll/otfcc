#ifndef CARYLL_TABLES_CFF_H
#define CARYLL_TABLES_CFF_H

#include "glyf.h"
#include <font/caryll-sfnt.h>
#include <support/util.h>

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
} cff_fontmatrix;

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
	uint32_t forceBold;
	uint32_t languageGroup;
	float expansionFactor;
	float initialRandomSeed;
	float defaultWidthX;
	float nominalWidthX;
} cff_private;

typedef struct _table_CFF table_CFF;

struct _table_CFF {
	bool isCID;
	sds version;
	sds notice;
	sds copyright;
	sds fontName;
	sds fullName;
	sds familyName;
	sds weight;
	uint32_t isFixedPitch;
	float italicAngle;
	float underlinePosition;
	float underlineThickness;
	float uniqueID;
	float fontBBoxTop;
	float fontBBoxBottom;
	float fontBBoxLeft;
	float fontBBoxRight;
	float strokeWidth;
	uint32_t fsType;
	cff_private *privateDict;
	cff_fontmatrix fontMatrix;

	// CID-only operators
	sds cidFontName;
	sds cidRegistry;
	uint32_t cidOrdering;
	float cidSupplement;
	float cidFontVersion;
	float cidFontRevision;
	float cidCount;
	float UIDBase;
	// CID FDArray
	uint16_t fdArrayCount;
	table_CFF *fdArray;
};

// CFF and glyf
typedef struct {
	table_CFF *meta;
	table_glyf *glyphs;
} caryll_cff_parse_result;

table_CFF *caryll_new_cff();
void caryll_delete_CFF(table_CFF *table);
caryll_cff_parse_result *caryll_read_CFF_and_glyf(caryll_packet, uint32_t tag);
void caryll_CFF_to_json(table_CFF *table, json_value *root, caryll_dump_options *dumpopts,
                        const char *tag);
table_CFF *caryll_CFF_from_json(json_value *root, caryll_dump_options *dumpopts);
caryll_buffer *caryll_write_CFF(caryll_cff_parse_result cffAndGlyf);

#endif
