#ifndef CARYLL_TABLES_CFF_H
#define CARYLL_TABLES_CFF_H

#include <font/caryll-sfnt.h>
#include <support/util.h>

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

typedef struct _table_cff table_cff;

struct _table_cff {
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
	table_cff *fdArray;
};

#endif
